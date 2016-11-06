// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include <renderer/renderer.h>
#include <renderer/font.h>

#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/guirenderer.h>
#include <runtime/standaloneresourcecache.h>

#include <platform/platform.h>
#include <platform/window.h>
#include <platform/kernel.h>
#include <platform/input.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>
#include <core/logging.h>
#include <core/profiler.h>
#include <core/mathlib.h>
#include <core/argumentparser.h>

#include <ui/ui.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/button.h>
#include <ui/label.h>
#include <ui/dockingcontainer.h>
#include <ui/menu.h>

#include <sdk/camera.h>
#include <sdk/game_api.h>
#include <sdk/utils.h>

#include <renderer/debug_draw.h>


#include "project.h"

using namespace platform;
using namespace renderer;
using namespace gemini;

bool net_listen_thread = true;
const size_t TOTAL_SENSORS = 3;

glm::quat sensors[TOTAL_SENSORS];

struct bno055_packet_t
{
	uint8_t header;
	uint8_t data[8 * TOTAL_SENSORS];
	uint8_t footer;

	bno055_packet_t()
	{
		header = 0xba;
		footer = 0xff;
	}

	bool is_valid() const
	{
		return (header == 0xba) && (footer == 0xff);
	}

	uint8_t get_header() const
	{
		return header;
	}

	uint8_t get_footer() const
	{
		return footer;
	}

	glm::quat get_orientation(uint32_t sensor_index = 0) const
	{
		int16_t x = 0;
		int16_t y = 0;
		int16_t z = 0;
		int16_t w = 0;

		const uint8_t* buffer = (data + (sensor_index * 8));

		// they are 16-bit LSB
		x = (((uint16_t)buffer[3]) << 8) | ((uint16_t)buffer[2]);
		y = (((uint16_t)buffer[5]) << 8) | ((uint16_t)buffer[4]);
		z = (((uint16_t)buffer[7]) << 8) | ((uint16_t)buffer[6]);
		w = (((uint16_t)buffer[1]) << 8) | ((uint16_t)buffer[0]);

		const double QUANTIZE = (1.0 / 16384.0);

		return glm::quat(w * QUANTIZE, x * QUANTIZE, y * QUANTIZE, z * QUANTIZE);
	}
}; // bno055_packet_t

// Returns true if timeout_msec has passed since target_msec.
// If true, sets target_msec to millis().
bool msec_passed(uint64_t& target_msec, uint32_t timeout_msec)
{
	uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;
	assert(current_milliseconds >= target_msec);
	if ((current_milliseconds - target_msec) > timeout_msec)
	{
		//LOGV("msec_passed: %d ms\n", (current_milliseconds - target_msec));
		target_msec = current_milliseconds;
		return true;
	}

	return false;
} // msec_passed

void sensor_thread(platform::Thread* thread)
{
	net_socket* sock = static_cast<net_socket*>(thread->user_data);
	LOGV("launched network listen thread.\n");

	const size_t PACKET_SIZE = sizeof(bno055_packet_t);
	LOGV("packet size is %i bytes\n", PACKET_SIZE);

	const size_t MAX_PACKET_DATA = 4 * PACKET_SIZE;
	char buffer[MAX_PACKET_DATA];
	size_t current_index = 0;

	// Waiting for a client to connect. Reply to broadcasts.
	const uint8_t STATE_WAITING = 1;

	// Waiting for the client to accept handshake.
	const uint8_t STATE_HANDSHAKE = 2;

	// Streaming with a client.
	const uint8_t STATE_STREAMING = 3;

	const uint32_t HANDSHAKE_VALUE = 1985;

	uint8_t current_state = STATE_WAITING;

	uint64_t last_client_ping_msec = 0;

	// Time to wait until sending a 'ping' back to the client to force
	// a keep-alive.
	const uint64_t CLIENT_PING_DELAY_MSEC = 1000;
	net_address client_address;

	const uint64_t CLIENT_TIMEOUT_MSEC = 3000;
	uint64_t last_client_contact_msec = 0;

	const uint32_t DISCONNECT_VALUE = 2005;


	while (net_listen_thread)
	{
		net_address source;

		uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;

		if (current_state == STATE_STREAMING)
		{
			if (msec_passed(last_client_contact_msec, CLIENT_TIMEOUT_MSEC))
			{
				LOGV("No communication from client in %i msec\n", CLIENT_TIMEOUT_MSEC);
				LOGV("Assuming the client is dead. Waiting for connections...\n");
				// Assume the client is dead.
				current_state = STATE_WAITING;
				continue;
			}

			if (msec_passed(last_client_ping_msec, CLIENT_PING_DELAY_MSEC))
			{
				uint32_t ping_value = 2000;
				net_socket_sendto(*sock, &client_address, (const char*)&ping_value, sizeof(uint32_t));
			}
		}

		timeval zero_timeval;
		zero_timeval.tv_sec = 0;
		zero_timeval.tv_usec = 1;

		fd_set receive;
		FD_ZERO(&receive);
		FD_SET(*sock, &receive);

		fd_set transmit;
		FD_ZERO(&transmit);
		FD_SET(*sock, &transmit);

		int select_result = select((*sock) + 1, &receive, &transmit, nullptr, &zero_timeval);
		assert(select_result >= 0);

		if (FD_ISSET(*sock, &receive))
		{
			int32_t bytes_available = net_socket_recvfrom(*sock, &source, buffer, PACKET_SIZE);
			if (bytes_available > 0)
			{
				LOGV("-> received %i bytes...\n", bytes_available);
				last_client_contact_msec = current_milliseconds;
				if (current_state == STATE_STREAMING)
				{
					bno055_packet_t* packet = reinterpret_cast<bno055_packet_t*>(buffer);
					if (packet->is_valid())
					{
						for (size_t index = 0; index < TOTAL_SENSORS; ++index)
						{
							glm::quat q = packet->get_orientation(index);
							//LOGV("q[%i]: %2.2f, %2.2f, %2.2f, %2.2f\n", index, q.x, q.y, q.z, q.w);
							sensors[index] = q;
						}
					}
					else
					{
						LOGV("Received invalid packet! (header = %x, footer = %x)\n", packet->get_header(), packet->get_footer());
					}
				}
				else if (current_state == STATE_WAITING)
				{
					uint32_t request = (*reinterpret_cast<uint32_t*>(buffer));
					if (request == 1983)
					{
						char ip[22] = { 0 };
						net_address_host(&source, ip, 22);
						uint16_t port = net_address_port(&source);
						LOGV("Mocap client at %s:%i; initiating handshake (%i)...\n", ip, port, HANDSHAKE_VALUE);
						net_socket_sendto(*sock, &source, (const char*)&HANDSHAKE_VALUE, sizeof(uint32_t));
						current_state = STATE_HANDSHAKE;
					}
					else
					{
						LOGW("Request value is invalid.\n");
					}
				}
				else if (current_state == STATE_HANDSHAKE)
				{
					LOGV("received data while handshaking: %i\n", bytes_available);
					uint32_t request = (*reinterpret_cast<uint32_t*>(buffer));
					if (request == HANDSHAKE_VALUE)
					{
						char ip[22] = { 0 };
						net_address_host(&source, ip, 22);
						uint16_t port = net_address_port(&source);
						LOGV("Connected with mocap client at %s:%i.\n", ip, port);
						uint32_t response = 65535;
						net_socket_sendto(*sock, &source, (const char*)&response, sizeof(uint32_t));

						net_address_set(&client_address, ip, port);
						last_client_contact_msec = platform::microseconds() * MillisecondsPerMicrosecond;

						current_state = STATE_STREAMING;
					}
					else
					{
						LOGV("handshake did not match! (received: %i, expected: %i)\n", request, HANDSHAKE_VALUE);
						current_state = STATE_WAITING;
					}
				}
			}
		}
	}



	if (current_state == STATE_STREAMING)
	{
		// send disconnect
		int32_t value = DISCONNECT_VALUE;
		net_socket_sendto(*sock, &client_address, (const char*)&value, sizeof(uint32_t));
		thread_sleep(500);
	}
}



namespace gui
{
	class TimelineScrubber : public Panel
	{
	public:
		TimelineScrubber(Panel* parent)
			: Panel(parent)
		{
			flags &= ~Flag_CursorEnabled;
			set_name("TimelineScrubber");
		}

		virtual void render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands) override
		{
			// TODO: we should get this from the style
			gemini::Color scrubber_highlight = gemini::Color::from_rgba(255, 128, 0, 32);
			gemini::Color scrubber_outline = gemini::Color::from_rgba(255, 128, 0, 192);

			// draw the main highlight fill
			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, scrubber_highlight);

			// draw the outline
			render_commands.add_line(geometry[0], geometry[1], scrubber_outline);
			render_commands.add_line(geometry[1], geometry[2], scrubber_outline);
			render_commands.add_line(geometry[2], geometry[3], scrubber_outline);
			render_commands.add_line(geometry[3], geometry[0], scrubber_outline);
		}
	};

	class Timeline : public Panel
	{
	public:
		Timeline(Panel* parent)
			: Panel(parent)
			, left_margin(0)
			, current_frame(0)
			, frame_width_pixels(0.0f)
		{
			flags |= Flag_CursorEnabled;
			set_name("Timeline");

			scrubber = new TimelineScrubber(this);
			scrubber->set_origin(0.0f, 0.0f);
		}

		gemini::Delegate<void (size_t)> on_scrubber_changed;

		virtual void handle_event(EventArgs& args) override
		{
			last_position = args.local;
			if (args.type == Event_CursorDrag || args.type == Event_CursorButtonPressed)
			{
				// snap to the closest point
				size_t last_frame = current_frame;

				int next_frame = (((int)args.local.x) - 1) / frame_width_pixels;

				if (next_frame < 0)
					current_frame = 0;
				else if (next_frame > (int)total_frames)
					current_frame = total_frames;
				else
					current_frame = next_frame;

				if ( current_frame <= 0 )
				{
					current_frame = 0;
				}
				else if ( current_frame > total_frames-1 )
				{
					current_frame = total_frames-1;
				}

				if ((last_frame != current_frame) && on_scrubber_changed.is_valid())
				{
					on_scrubber_changed(current_frame);
				}
			}

			args.handled = true;
		} // handle_event

		virtual void update(gui::Compositor* compositor, float delta_seconds) override
		{
			// assuming a horizontal timeline
			if (frame_width_pixels == 0)
			{
				// recompute the distance here
				frame_width_pixels = (size.width / (float)total_frames);
			}

			// should be updated before rendering
			assert(frame_width_pixels > 0);

			scrubber->set_size(frame_width_pixels, size.height);
			scrubber->set_origin((current_frame * frame_width_pixels), 0.0f);

			Panel::update(compositor, delta_seconds);
		} // update

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			// TODO: should get this from the style
			const gemini::Color frame_color = gemini::Color::from_rgba(96, 96, 96, 255);

			// draw the background
			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], gui::render::WhiteTexture, gemini::Color::from_rgba(64, 64, 64, 255));

			// add a top rule line to separate this panel
			render_commands.add_line(geometry[0], geometry[3], gemini::Color::from_rgba(0, 0, 0, 255), 1.0f);

			// center the individual frames
			Rect block;
			block.set(left_margin + 2.0f, 1.0, (frame_width_pixels - 4.0f), size.height - 2.0f);

			for (size_t index = 0; index < total_frames; ++index)
			{
				// draw frame ticks until we reach the end of the panel
				if (block.origin.x + block.size.width >= (origin.x + size.width))
				{
					break;
				}

				Point points[4];
				points[0] = transform_point(local_transform, block.origin);
				points[1] = transform_point(local_transform, Point(block.origin.x, block.origin.y + block.size.height));
				points[2] = transform_point(local_transform, Point(block.origin.x + block.size.width, block.origin.y + block.size.height));
				points[3] = transform_point(local_transform, Point(block.origin.x + block.size.width, block.origin.y));
				render_commands.add_rectangle(points[0], points[1], points[2], points[3], gui::render::WhiteTexture, frame_color);

				block.origin.x += frame_width_pixels;
			}

			render_children(compositor, renderer, render_commands);
		} // render

		void set_frame_range(int lower_frame_limit, int upper_frame_limit)
		{
			lower_limit = lower_frame_limit;
			upper_limit = upper_frame_limit;

			assert(upper_limit > lower_limit);
			total_frames = (upper_limit - lower_limit);

			// force a recalculate on the next render call
			frame_width_pixels = 0;
		} // set_frame_range

		void set_frame(size_t frame)
		{
			current_frame = frame;
		}

	private:
		size_t left_margin;
		size_t current_frame;
		size_t total_frames;

		// frame limits
		int lower_limit;
		int upper_limit;

		// width of a clickable 'frame'
		float frame_width_pixels;

		Point last_position;

		TimelineScrubber* scrubber;
	}; // Timeline


	// This uses a render target to present data
	class RenderableSurface : public gui::Panel
	{
	public:
		RenderableSurface(Panel* parent)
			: Panel(parent)
			, target(nullptr)
			, handle(render::WhiteTexture)
		{
			flags |= Flag_CursorEnabled;
			set_name("RenderableSurface");
		}

		void set_render_target(render2::RenderTarget* render_target) { target = render_target; }
		render2::RenderTarget* get_render_target() const { return target; }
		void set_texture_handle(int ref) { handle = ref; }

		virtual void render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands) override
		{
			if (on_render_content.is_valid())
			{
				on_render_content(target);
			}

			render_commands.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], handle, gemini::Color::from_rgba(255, 255, 255, 255));
		}

		// invoked when the handler should render its content to the render
		// target.
		gemini::Delegate<void (render2::RenderTarget*)> on_render_content;

	private:
		render2::RenderTarget* target;
		int handle;
	}; // RenderableSurface
} // namespace gui

struct MyVertex
{
	float position[3];
	float color[4];

	void set_position(float x, float y, float z)
	{
		position[0] = x;
		position[1] = y;
		position[2] = z;
	}

	void set_color(float red, float green, float blue, float alpha)
	{
		color[0] = red;
		color[1] = green;
		color[2] = blue;
		color[3] = alpha;
	}
};


void log_window_logger_message(core::logging::Handler* handler, const char* message, const char* /*filename*/, const char* /*function*/, int /*line*/, int /*type*/)
{
	gui::Label* logwindow = static_cast<gui::Label*>(handler->userdata);
	logwindow->append_text(message);
	logwindow->scroll_to_bottom();
}

int log_window_logger_open(core::logging::Handler* /*handler*/)
{
	return 1;
}

void log_window_logger_close(core::logging::Handler* /*handler*/)
{
}

// Asset Processing
#if 0
// 1. asset source (currently opened project)
// 2. destination folder: <asset_source>/../builds/<platform>/

// tools for use by asset processor.
// libsox/libogg: audio format conversions
// pvrtexlib: PowerVR texture compression
// nvtt / libsquish: texture compression
// models/animation: custom code
// packaging: custom code
#endif

class AssetProcessingPanel : public gui::Panel
{
public:

	AssetProcessingPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		flags |= gui::Panel::Flag_CanMove;
	}
}; // AssetProcessingPanel







glm::quat transform_sensor_rotation(const glm::quat& q)
{
	// this bit of code converts the coordinate system of the BNO055
	// to that of gemini.
	glm::quat flipped(q.w, q.x, -q.z, -q.y);
	glm::quat y = glm::quat(glm::vec3(0, mathlib::degrees_to_radians(180), 0));
	return glm::inverse(y * flipped);
}

struct mocap_frame_t
{
	uint32_t frame_index;
	glm::quat poses[TOTAL_SENSORS];
};

class EditorKernel : public kernel::IKernel,
public kernel::IEventListener<kernel::KeyboardEvent>,
public kernel::IEventListener<kernel::MouseEvent>,
public kernel::IEventListener<kernel::SystemEvent>
{
private:
	bool active;
	platform::window::NativeWindow* main_window;

	render2::Device* device;
	render2::Buffer* vertex_buffer;

	render2::Pipeline* pipeline;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

	glm::mat4 joint_offets[TOTAL_SENSORS];
	glm::quat zeroed_orientations[TOTAL_SENSORS];
//	GLsync fence;

	gui::Compositor* compositor;
	gui::Label* log_window;
	GUIRenderer* gui_renderer;
	::renderer::StandaloneResourceCache resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;
	gui::DockingContainer* container;
	AssetProcessingPanel* asset_processor;


	Array<mocap_frame_t> mocap_frames;
	size_t current_mocap_frame;
	PathString current_mocap_filename;

	float value;

	core::logging::Handler log_handler;

	Camera camera;

	float yaw;
	float pitch;

	bool left_down;
	bool right_down;
	bool forward_down;
	bool backward_down;

	uint64_t last_time;

	bool should_move_view;

	bool is_recording_frames;
	bool is_playing_frames;


	net_socket data_socket;
	platform::Thread* sensor_thread_handle;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
		, render_target(nullptr)
		, texture(nullptr)
		, value(0.0f)
		, container(nullptr)
		, is_recording_frames(false)
		, is_playing_frames(false)
	{
		yaw = 0.0f;
		pitch = 0.0f;

		left_down = right_down = forward_down = backward_down = false;

		last_time = 0;

		should_move_view = false;
		sensor_thread_handle = nullptr;
		data_socket = -1;

		asset_processor = nullptr;
	}

	virtual ~EditorKernel() {}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == gemini::BUTTON_ESCAPE && event.is_down)
		{
			set_active(false);
		}
		//else
		//{
		//	LOGV("key is_down: '%s', name: '%s', modifiers: %zu\n", event.is_down ? "Yes" : "No", gemini::key_name(event.key), event.modifiers);
		//}

		if (event.key == BUTTON_A)
			left_down = event.is_down;

		if (event.key == BUTTON_D)
			right_down = event.is_down;

		if (event.key == BUTTON_W)
			forward_down = event.is_down;

		if (event.key == BUTTON_S)
			backward_down = event.is_down;

		if (event.key == BUTTON_SPACE)
		{
			LOGV("freezing rotations\n");
			for (size_t index = 0; index < TOTAL_SENSORS; ++index)
			{
				zeroed_orientations[index] = transform_sensor_rotation(sensors[index]);
			}
		}
	}


	virtual void event(kernel::SystemEvent& event)
	{
		if (event.subtype == kernel::WindowResized)
		{
			assert(device);
			device->backbuffer_resized(event.render_width, event.render_height);

			assert(compositor);
			compositor->resize(event.window_width, event.window_height);
		}
		else if (event.subtype == kernel::WindowClosed)
		{
			LOGV("Window was closed!\n");
			set_active(false);
		}
	}

	virtual void event(kernel::MouseEvent& event)
	{
		if (compositor)
		{
			static gui::CursorButton::Type input_to_gui[] = {
				gui::CursorButton::None,
				gui::CursorButton::Left,
				gui::CursorButton::Right,
				gui::CursorButton::Middle,
				gui::CursorButton::Mouse4,
				gui::CursorButton::Mouse5
			};

			if (event.subtype == kernel::MouseMoved)
			{
				bool handled = compositor->cursor_move_absolute(event.mx, event.my);

				if (!handled && should_move_view)
				{
					if (event.dx != 0 || event.dy != 0)
					{
						const float sensitivity = .10f;
						camera.move_view(event.dx, event.dy);
					}
				}
			}
			else if (event.subtype == kernel::MouseButton)
			{
				if (event.is_down)
				{
					if (event.button == MouseButton::MOUSE_LEFT)
					{
						should_move_view = true;
					}

					platform::window::set_mouse_tracking(true);
				}
				else
				{
					if (event.button == MouseButton::MOUSE_LEFT)
					{
						should_move_view = false;
					}
					platform::window::set_mouse_tracking(false);
				}
				compositor->cursor_button(input_to_gui[event.button], event.is_down);
			}
			else if (event.subtype == kernel::MouseWheelMoved)
			{
				compositor->cursor_scroll(event.wheel_direction);
			}
		}
	}

	void test_open_dialog()
	{
		Array<PathString> paths;
		uint32_t flags = platform::OpenDialogFlags::CanChooseDirectories;

		if (platform::show_open_dialog("Choose Game Directory", flags, paths).succeeded())
		{
			LOGV("target path is: %s\n", paths[0]());
		}
	}

	// menu handlers

	void on_file_new(void)
	{
	}

	void on_file_open()
	{
	}

	void on_file_quit(void)
	{
		set_active(false);
	}

	void on_record_start(void)
	{
		if (is_recording_frames == false)
		{
			current_mocap_filename.clear();
			Array<PlatformExtensionDescription> extensions;
			extensions.push_back(PlatformExtensionDescription("Sensor Stream", "sensor"));

			platform::Result save_result = platform::show_save_dialog(
				"Save Sensor Stream",
				0, /* Flags */
				extensions,
				"sensor",
				current_mocap_filename
			);

			if (save_result.succeeded())
			{
				LOGV("Save stream to: %s\n", current_mocap_filename());
				mocap_frames.clear();
				current_mocap_frame = 0;
				is_recording_frames = true;
			}
		}
	}

	void on_record_stop(void)
	{
		if (is_recording_frames)
		{
			is_recording_frames = false;

			platform::File handle = platform::fs_open(current_mocap_filename(), FileMode_Write);
			if (handle.is_open())
			{
				// 1. Total Sensors (poses)
				platform::fs_write(handle, &TOTAL_SENSORS, sizeof(TOTAL_SENSORS), 1);

				// 2. Total frames.
				const size_t total_frames = mocap_frames.size();
				platform::fs_write(handle, &total_frames, sizeof(size_t), 1);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					mocap_frame_t& frame = mocap_frames[index];

					for (size_t pose = 0; pose < TOTAL_SENSORS; ++pose)
					{
						const glm::quat& rotation = frame.poses[pose];
						platform::fs_write(handle, (const void*)&rotation, sizeof(glm::quat), 1);
					}
				}

				platform::fs_close(handle);
			}

			LOGV("Recorded %i frames\n", mocap_frames.size());
			current_mocap_filename.clear();
			current_mocap_frame = 0;
			mocap_frames.clear();
		}
	}

	void on_playback_start(void)
	{
		Array<PathString> paths;
		platform::Result open_result = platform::show_open_dialog("Choose Sensor Stream", 0, paths);
		if (open_result.succeeded())
		{
			LOGV("loading sensor stream '%s'\n", paths[0]());
			platform::File handle = platform::fs_open(paths[0](), FileMode_Read);
			if (handle.is_open())
			{
				// 1. Total Sensors (poses)
				size_t total_sensors = 0;
				platform::fs_read(handle, &total_sensors, sizeof(size_t), 1);

				// If you hit this, the file loaded differs in the number of sensors
				// it contains vs the number of sensors this now supports.
				assert(total_sensors == TOTAL_SENSORS);

				// 2. Read Total Frames
				size_t total_frames = 0;
				platform::fs_read(handle, &total_frames, sizeof(size_t), 1);

				LOGV("found %i total frames...\n", total_frames);
				mocap_frames.resize(total_frames);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					mocap_frame_t& frame = mocap_frames[index];
					frame.frame_index = index;
					for (size_t pose = 0; pose < total_sensors; ++pose)
					{
						glm::quat& rotation = frame.poses[pose];
						platform::fs_read(handle, (void*)&rotation, sizeof(glm::quat), 1);
					}
				}
			}

			// Set state to playback.
			current_mocap_frame = 0;
			is_playing_frames = true;
			is_recording_frames = false;

			LOGV("start playback of %i frames...\n", mocap_frames.size());
		}
	}

	void on_playback_stop(void)
	{
		is_playing_frames = false;
		LOGV("stopped playback.\n");
	}


	void on_window_toggle_asset_processor(void)
	{
		assert(asset_processor);
		asset_processor->set_visible(!asset_processor->is_visible());
	}


	void timeline_scrubber_changed(size_t current_frame)
	{
		value = (current_frame / 30.0f);
	}

	void render_main_content(render2::RenderTarget* render_target)
	{
		render2::Pass render_pass;
		render_pass.color(0.0f, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;
		render_pass.target = render_target;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(pipeline);

		const bool test_triangle = 1;
		if (test_triangle)
		{
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, 3);
			device->queue_buffers(queue, 1);
		}
		device->destroy_serializer(serializer);
	}

	virtual kernel::Error startup()
	{
		// parse command line values
		std::vector<std::string> arguments;
		core::argparse::ArgumentParser parser;
		core::StackString<MAX_PATH_SIZE> content_path;

		runtime_load_arguments(arguments, parser);

		core::argparse::VariableMap vm;
		const char* docstring = R"(
Usage:
	--assets=<content_path>

Options:
	-h, --help  Show this help screen
	--version  Display the version number
	--assets=<content_path>  The path to load content from
	)";

		if (parser.parse(docstring, arguments, vm, "1.0.0-alpha"))
		{
			std::string path = vm["--assets"];
			content_path = platform::make_absolute_path(path.c_str());
		}
		else
		{
			return kernel::CoreFailed;
		}


		std::function<void(const char*)> custom_path_setup = [&](const char* application_data_path)
		{
			core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
			PathString root_path = platform::get_program_directory();

			// the root path is the current binary path
			filesystem->root_directory(root_path);

			// the content directory is where we'll find our assets
			filesystem->content_directory(content_path);

			// load engine settings (from content path)
			//load_config(config);

			// the application path can be specified in the config (per-game basis)
			//const platform::PathString application_path = platform::get_user_application_directory(config.application_directory.c_str());
			filesystem->user_application_directory(application_data_path);
		};

		gemini::runtime_startup("arcfusion.net/orion", custom_path_setup);

		// create a platform window
		{
			platform::window::startup(platform::window::RenderingBackend_Default);

			platform::window::Parameters params;

			bool enable_fullscreen = false;
			if (enable_fullscreen)
			{
				params.enable_fullscreen = enable_fullscreen;
				params.frame = platform::window::screen_frame(0);
			}
			else
			{
				params.frame = platform::window::centered_window_frame(0, 800, 600);
			}

			params.window_title = "orion";
			main_window = platform::window::create(params);

			// set perspective on camera
			camera.perspective(60.0f, (int)params.frame.width, (int)params.frame.height, 0.01f, 1024.0f);
			camera.set_position(glm::vec3(0.0f, 5.0f, 10.0f));
			camera.set_type(Camera::FIRST_PERSON);
			camera.update_view();
		}



		// old renderer initialize
		{
//			renderer::RenderSettings render_settings;
//			render_settings.gamma_correct = true;

//			renderer::startup(renderer::OpenGL, render_settings);

			// clear errors
//			gl.CheckError("before render startup");

//			fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}

		platform::window::Frame window_frame;

		// initialize the renderer
		{
			using namespace render2;
			RenderParameters params;

			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["depth_size"] = "24";
//			params["multisample"] = "4";

			// set opengl specific options
			params["rendering_backend"] = "opengl";
//			params["opengl.major"] = "3";
//			params["opengl.minor"] = "2";
//			params["opengl.profile"] = "core";
//			params["opengl.share_context"] = "true";

//			for (RenderParameters::Iterator it = params.begin(); it != params.end(); ++it)
//			{
//				const param_string& key = it.key();
//				const param_string& value = it.value();
//				LOGV("'%s' -> '%s'\n", key(), value());
//			}

			device = create_device(params);

			window_frame = platform::window::get_frame(main_window);
			device->init(window_frame.width, window_frame.height);

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = device->create_shader("vertexcolor");
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
			pipeline = device->create_pipeline(desc);

			size_t total_bytes = sizeof(MyVertex) * 6;
			vertex_buffer = device->create_vertex_buffer(total_bytes);

#if 1
			MyVertex* vertex = reinterpret_cast<MyVertex*>(device->buffer_lock(vertex_buffer));

//			MyVertex vertex[4];

			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

			vertex[2].set_position(window_frame.width/2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_unlock(vertex_buffer);
#endif
		}

		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}

		font::startup(device);

		// initialize debug draw
		debugdraw::startup(device);

#if 0
		// load the gui
		{
			// gui layout

			core::filesystem::IFileSystem* fs = core::filesystem::instance();

			platform::File handle = platform::fs_open("ui/main.ui", platform::FileMode_Read);
			if (handle.is_open())
			{
//				core::DataStream* stream = fs->memory_from_file(handle);

				// create the gui elements from a file
//				compositor->create_layout_from_memory(stream->get_data(), stream->get_data_size());

				platform::fs_close(handle);
			}

		}
#else
		{
			// in lieu of the above working; manually setup the gui...
			gui_renderer = MEMORY_NEW(GUIRenderer, core::memory::global_allocator())(resource_cache);
			gui_renderer->set_device(device);

			assert(window_frame.width > 0);
			assert(window_frame.height > 0);
			compositor = new gui::Compositor(window_frame.width, window_frame.height, &resource_cache, gui_renderer);
			compositor->set_name("compositor");


#if 0
			gui::Timeline* timeline = new gui::Timeline(compositor);
			timeline->set_bounds(0, 550, 800, 50);
			timeline->set_frame_range(0, 30);
			timeline->on_scrubber_changed.bind<EditorKernel, &EditorKernel::timeline_scrubber_changed>(this);
			timeline->set_frame(0);
#endif

#if 0
			// try and use a docking container for fun.
			container = new gui::DockingContainer(compositor);
			container->set_name("docking_container");
			container->set_dimensions(0.6f, 0.6f);



	#if 1
			gui::Panel* tp = new gui::Panel(compositor);
			tp->set_dimensions(0.1f, 0.1f);
			tp->set_origin(700, 0.0f);
			tp->set_background_color(gemini::Color(1.0f, 0.5f, 0.0f));
			tp->set_flags(tp->get_flags() | gui::Panel::Flag_CanMove);
			tp->set_name("draggable_test_panel");

			tp = new gui::Panel(compositor);
			tp->set_dimensions(0.1f, 0.1f);
			tp->set_origin(700, 200);
			tp->set_background_color(gemini::Color(0.5f, 0.5f, 0.7f));
			tp->set_flags(tp->get_flags() | gui::Panel::Flag_CanMove);
			tp->set_name("draggable_test_panel");
	#endif
#endif

#if 1
			asset_processor = new AssetProcessingPanel(compositor);
			asset_processor->set_origin(0.0f, 25.0f);
			asset_processor->set_size(400, 100);
			asset_processor->set_background_color(gemini::Color(0.25f, 0.25f, 0.25f));
			//asset_processor->set_visible(false);
#endif




// add a menu
#if 1
			gui::MenuBar* menubar = new gui::MenuBar(compositor);
			{
				gui::Menu* filemenu = new gui::Menu("File", menubar);

				filemenu->add_item("New Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_new, this));
				filemenu->add_item("Open Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_open, this));
				filemenu->add_separator();
				filemenu->add_item("Quit", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_quit, this));
				menubar->add_menu(filemenu);


				gui::Menu* record = new gui::Menu("Sensor", menubar);
				record->add_item("Save New Stream...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_record_start, this));
				record->add_item("Stop Recording", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_record_stop, this));
				record->add_item("Open Stream...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_playback_start, this));
				record->add_item("Stop Playback", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_playback_stop, this));
				menubar->add_menu(record);


				gui::Menu* windowmenu = new gui::Menu("Window", menubar);
				windowmenu->add_item("Show Asset Processor", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_window_toggle_asset_processor, this));
				menubar->add_menu(windowmenu);
			}
#endif

#if 0
			gui::RenderableSurface* surface = new gui::RenderableSurface(compositor);
			surface->set_bounds(0, 0, 512, 512);
			surface->on_render_content.bind<EditorKernel, &EditorKernel::render_main_content>(this);

			image::Image checker_pattern;
			checker_pattern.create(512, 512, 3);
			checker_pattern.fill(gemini::Color::from_rgba(0, 25, 25, 255));
			texture = device->create_texture(checker_pattern);

			int handle = resource_cache.track_texture(texture);

			// TODO: sort out this interface!
			render_target = device->create_render_target(texture);
			surface->set_render_target(render_target);
			surface->set_texture_handle(handle);
#endif

#if 0
			// TODO: This needs more work as it can still get swamped and
			// latency increases dramatically.
			log_window = new gui::Label(compositor);
			log_window->set_origin(0.0f, 450);
			log_window->set_dimensions(1.0f, 0.25f);
			log_window->set_font("fonts/debug.ttf", 16);
			log_window->set_name("log_window");
			log_window->set_foreground_color(gemini::Color(0.85f, 0.85f, 0.85f));
			log_window->set_background_color(gemini::Color(0.10f, 0.10f, 0.10f));
			uint32_t current_flags = log_window->get_flags();
			log_window->set_flags(gui::Panel::Flag_CursorEnabled | gui::Panel::Flag_CanMove | current_flags);

			// install a log handler
			log_handler.open = log_window_logger_open;
			log_handler.close = log_window_logger_close;
			log_handler.message = log_window_logger_message;
			log_handler.userdata = (void*)log_window;
			core::logging::instance()->add_handler(&log_handler);

			LOGV("log initialized.\n");
#endif


		}
#endif

		// inertial motion capture
		// imc library.
		// header (on connect)
		// version
		// # sensors

		// each packet contains data for N sensors
		// plus a 4-byte sequence id.

		// 1. visualization 3d scene
		// 2. record sensor data
		// 3. play back sensor data




		kernel::parameters().step_interval_seconds = (1.0f/50.0f);
		int32_t startup_result = net_startup();
		assert(startup_result == 0);

		data_socket = net_socket_open(net_socket_type::UDP);
		assert(net_socket_is_valid(data_socket));

		net_address addr;
		net_address_init(&addr);
		net_address_set(&addr, "0.0.0.0", 27015);

		int32_t bind_result = net_socket_bind(data_socket, &addr);
		assert(bind_result == 0);

		net_listen_thread = true;
		sensor_thread_handle = platform::thread_create(sensor_thread, &data_socket);

		return kernel::NoError;
	}



	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		platform::update(kernel::parameters().framedelta_milliseconds);

		static float value = 0.0f;
		static float multiplifer = 1.0f;

		value += 0.01f * multiplifer;
		value = glm::clamp(value, 0.0f, 1.0f);
		if (value == 0.0f || value == 1.0f)
			multiplifer *= -1;


		{
			kernel::Parameters& params = kernel::parameters();
			const float movement_factor = 30.0f;

			if (left_down)
				camera.move_left(movement_factor * params.framedelta_seconds);
			if (right_down)
				camera.move_right(movement_factor * params.framedelta_seconds);
			if (forward_down)
				camera.move_forward(movement_factor * params.framedelta_seconds);
			if (backward_down)
				camera.move_backward(movement_factor * params.framedelta_seconds);

			camera.update_view();
		}


		int32_t yoffset = 130;
		debugdraw::text(20, yoffset, "Left Click + Drag: Rotate Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset+16, "WASD: Move Camera", gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset+32, "Space: Calibrate / Freeze Rotations", gemini::Color(1.0f, 1.0f, 1.0f));

		glm::quat local_rotations[TOTAL_SENSORS];

		// We need to adjust the coordinate frame from the sensor to the engine.
		local_rotations[0] = glm::inverse(zeroed_orientations[0]) * transform_sensor_rotation(sensors[0]);
		local_rotations[1] = glm::inverse(local_rotations[0]) * glm::inverse(zeroed_orientations[1]) * transform_sensor_rotation(sensors[1]);
		local_rotations[2] = glm::inverse(local_rotations[1]) * glm::inverse(zeroed_orientations[2]) * transform_sensor_rotation(sensors[2]);

		//local_rotations[1] = sensors[1] * glm::inverse(sensors[0]);

		// temp: setup joint offsets
		joint_offets[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		joint_offets[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));
		joint_offets[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));


		glm::mat4 world_poses[TOTAL_SENSORS];
		glm::vec3 last_origin;

		glm::mat4 parent_pose;

		mocap_frame_t mocap_frame;

		if (is_playing_frames)
		{
			memcpy(local_rotations, mocap_frames[current_mocap_frame].poses, sizeof(glm::quat) * TOTAL_SENSORS);
			current_mocap_frame++;

			if (current_mocap_frame > (mocap_frames.size() - 1))
			{
				is_playing_frames = false;
				LOGV("reached end of playback buffer at %i frames.\n", current_mocap_frame);
			}
		}


		for (size_t index = 0; index < TOTAL_SENSORS; ++index)
		{
			glm::mat4 m = glm::toMat4(local_rotations[index]);
			glm::mat4 local_pose = (joint_offets[index] * m);
			glm::mat4 parent_pose;
			if (index > 0)
			{
				parent_pose = world_poses[(index-1)];
			}

			glm::mat4& world_pose = world_poses[index];
			world_pose = parent_pose * local_pose;

			debugdraw::axes(world_pose, 0.1f);

			glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));
			if (index > 0)
			{
				debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
			}
			last_origin = origin;

			debugdraw::sphere(origin, Color::from_rgba(255, 0, 0, 255), 0.025f);
			//mocap_frame.poses[index] = local_rotations[index];
		}


		if (is_recording_frames)
		{
			memcpy(mocap_frame.poses, local_rotations, sizeof(glm::quat) * TOTAL_SENSORS);
			mocap_frame.frame_index = current_mocap_frame;
			current_mocap_frame++;

			mocap_frames.push_back(mocap_frame);
		}
		//debugdraw::axes(glm::mat4(1.0f), 1.0f);

		debugdraw::update(kernel::parameters().framedelta_seconds);

		if (compositor)
		{
			compositor->tick(static_cast<float>(kernel::parameters().step_interval_seconds));
		}

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

		modelview_matrix = glm::mat4(1.0f);
		projection_matrix = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);
		//const glm::vec3& p = camera.get_position();
		//LOGV("p: %2.2f, %2.2f, %2.2f\n", p.x, p.y, p.z);

		modelview_matrix = camera.get_modelview();
		projection_matrix = camera.get_projection();
		//pipeline->constants().set("modelview_matrix", &modelview_matrix);
		//pipeline->constants().set("projection_matrix", &projection_matrix);

		value = 0.35f;

		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(pipeline);
		//serializer->vertex_buffer(vertex_buffer);
		//serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);

		platform::window::activate_context(main_window);

		if (compositor)
		{
			compositor->draw();
		}

		debugdraw::render(modelview_matrix, projection_matrix, window_frame.width, window_frame.height);

		device->submit();

		platform::window::swap_buffers(main_window);

#if defined(GEMINI_ENABLE_PROFILER)
		gemini::profiler::report();
		gemini::profiler::reset();
#endif

//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);

		kernel::Parameters& params = kernel::parameters();
		params.current_frame++;


		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * SecondsPerMillisecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;
		last_time = current_time;
	}


	virtual void shutdown()
	{
		net_listen_thread = false;

		platform::thread_join(sensor_thread_handle, 1000);
		platform::thread_destroy(sensor_thread_handle);

		net_shutdown();
		debugdraw::shutdown();

		// remove the log handler
		core::logging::instance()->remove_handler(&log_handler);

		device->destroy_render_target(render_target);
		device->destroy_texture(texture);

		// compositor will cleanup children
		delete compositor;

		// shutdown the gui
		MEMORY_DELETE(gui_renderer, core::memory::global_allocator());

		// explicitly clear the resource cache or else the allocator will
		// detect leaks.
		resource_cache.clear();

		font::shutdown();

		// shutdown the render device
		device->destroy_buffer(vertex_buffer);

		device->destroy_pipeline(pipeline);

		destroy_device(device);

//		glDeleteSync(fence);

		platform::window::destroy(main_window);
		platform::window::shutdown();

		gemini::runtime_shutdown();
	}

};

PLATFORM_MAIN
{
	PLATFORM_IMPLEMENT_PARAMETERS();
	PLATFORM_RETURN(platform::run_application(new EditorKernel()));
}
