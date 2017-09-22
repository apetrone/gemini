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
#include <runtime/animation.h>
#include <runtime/assets.h>
#include <runtime/debug_event.h>
#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/geometry.h>
#include <runtime/guirenderer.h>
#include <runtime/hotloading.h>
#include <runtime/standaloneresourcecache.h>

#include <runtime/mesh.h>
#include <runtime/transform_graph.h>

#include <platform/input.h>
#include <platform/kernel.h>
#include <platform/platform.h>
#include <platform/network.h>
#include <platform/window.h>
#include <platform/directory_monitor.h>

#include <core/hashset.h>
#include <core/stackstring.h>
#include <core/fixedarray.h>
#include <core/logging.h>
#include <core/profiler.h>
#include <core/mathlib.h>
#include <core/argumentparser.h>

#include <ui/button.h>
#include <ui/compositor.h>
#include <ui/graph.h>
#include <ui/label.h>
#include <ui/layout.h>
#include <ui/menu.h>
#include <ui/timeline.h>
#include <ui/ui.h>

#include <rapid/rapid.h>

#include <renderer/debug_draw.h>
#include <renderer/font_library.h>
#include <renderer/renderer.h>
#include <renderer/scene_renderer.h>

#include <sdk/camera.h>
#include <sdk/game_api.h>
#include <sdk/utils.h>


#include "editable_mesh.h"

#include "project.h"
#include "parameter.h"

using namespace platform;
using namespace renderer;
using namespace gemini;

#include <runtime/imocap.h>
#include <runtime/debug_event.h>

#define ENABLE_UI 1
#define DRAW_SENSOR_GRAPHS 0
#define TEST_SPRING_SYSTEM 0
#define TEST_TELEMETRY_SYSTEM 0
#define TEST_TELEMETRY_HOST 0
#define DRAW_IMOCAP 0

#define DRAW_LINES 0
const size_t TOTAL_LINES = 256;

namespace gui
{
	// This uses a render target to present data
	class RenderableSurface : public gui::Panel
	{
	public:
		RenderableSurface(Panel* parent)
			: Panel(parent)
			, render_device(nullptr)
			, target(nullptr)
			, handle(render::WhiteTexture)
		{
			flags |= Flag_CursorEnabled;
			set_name("RenderableSurface");
		}

		void set_device(render2::Device* device) { render_device = device; }
		void set_render_target(render2::RenderTarget* render_target) { target = render_target; }
		render2::RenderTarget* get_render_target() const { return target; }
		void set_texture_handle(int ref) { handle = ref; }

		virtual void render(gui::Compositor* /*compositor*/, gui::Renderer* /*renderer*/, gui::render::CommandList& render_commands) override
		{
			gui::Painter painter(this, render_commands);

			if (on_render_content.is_valid())
			{
				on_render_content(target);
			}

			painter.add_rectangle(geometry[0], geometry[1], geometry[2], geometry[3], handle, gemini::Color::from_rgba(255, 255, 255, 255));
		}

		// invoked when the handler should render its content to the render
		// target.
		gemini::Delegate<void (render2::RenderTarget*)> on_render_content;

		virtual void set_size(const Size& new_size)
		{
			// resize the render target
			LOGV("resize render target %i %i\n", new_size.width, new_size.height);
			gui::Panel::set_size(new_size);
			//https://blog.nelhage.com/2016/12/how-i-test/
			render_device->resize_render_target(target, new_size.width, new_size.height);
		}

	private:
		render2::Device* render_device;
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


// The quiet period required before an asset change is dispatched to subscribers.
const float ASSET_CHANGE_NOTIFICATION_QUIET_SECONDS = 0.250f;
struct ModifiedAssetData
{
	float quiet_time_left;
	MonitorHandle monitor_handle;
};

typedef HashSet<platform::PathString, ModifiedAssetData> PathDelayHashSet;

size_t total_bytes = sizeof(MyVertex) * 4;


class SpringPanel : public gui::Panel
{
public:

	gui::Point tube[4];

	struct Object
	{
		gui::Point position;
		gui::Point velocity;
	};

	float k;
	float x;
	Object box;

	gui::Point target;

	SpringPanel(gui::Panel* parent)
		: gui::Panel(parent)
	{
		set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

		tube[0] = gui::Point(0.0f, 0.0f);
		tube[1] = gui::Point(0.0f, 50.0f);
		tube[2] = gui::Point(50.0f, 50.0f);
		tube[3] = gui::Point(50.0f, 0.0f);


		box.position = gui::Point(0.0f, 0.0f);
		box.velocity = gui::Point(0.0f, 0.0f);
		target = gui::Point(50.0f, 0.0f);
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override;
	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
};


void SpringPanel::update(gui::Compositor* compositor, float delta_seconds)
{
	gui::Panel::update(compositor, delta_seconds);
#if 0
	// Simple harmonic oscillator
	x = box.position.x - target.x;
	k = 15.f;

	const float mass_kgs = .045f;
	//float w = sqrt(k / m);
	//const float frequency = (w / (2 * mathlib::PI));
	//const float period = 1.0f / frequency;
	const float T = (mathlib::PI * 2.0) * (sqrt(mass_kgs / k));
	const float frequency = (1.0f / T);
	LOGV("freq: %2.2fHz\n", frequency);


	box.velocity.x += -k*x * delta_seconds;
	box.position += box.velocity * delta_seconds;
#endif

#if 1
	// Damped harmonic oscillator
	x = box.position.x - target.x;
	k = 0.125f;
	const float mass_kgs = .045f;
	const float c = 0.19f;

	//const float damping_ratio = (c / (2.0 * sqrt(mass_kgs * k)));
	//LOGV("damping_ratio is %2.2f\n", damping_ratio);

	//box.velocity += 0.75f * (-(box.position - target) * delta_seconds);
	box.velocity.x += -k*x -c * box.velocity.x;
	box.position += box.velocity;
#endif
}

void SpringPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
{
	gui::Painter painter(this, render_commands);

	gui::Panel::render(compositor, renderer, render_commands);

	painter.add_rectangle(
		box.position + tube[0],
		box.position + tube[1],
		box.position + tube[2],
		box.position + tube[3],
		gui::render::WhiteTexture,
		gemini::Color(1.0f, 0.5f, 0.0)
	);
}

class NumberPanel : public gui::Label
{
public:

	NumberPanel(gui::Panel* parent)
		: gui::Label(parent)
		, cursor_index(0)
		, is_captured(0)
		, value(nullptr)
		, step(0.01f)
	{
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override;
	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
	virtual void handle_event(gui::EventArgs& args) override;

	float* value;
	float step;
	gui::Point down_position;
	uint8_t is_captured;
	uint32_t cursor_index;
};

void NumberPanel::update(gui::Compositor* compositor, float delta_seconds)
{
	gui::Label::update(compositor, delta_seconds);
}

void NumberPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
{
	gemini::Color outline_color(1.0f, 0.0f, 0.0f);

	gui::Painter painter(this, render_commands);

	//painter.add_line(
	//	geometry[0],
	//	geometry[1],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[1],
	//	geometry[2],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[2],
	//	geometry[3],
	//	outline_color
	//);

	//painter.add_line(
	//	geometry[3],
	//	geometry[0],
	//	outline_color
	//);

	gui::Label::render(compositor, renderer, render_commands);

	gui::Rect bounds;
	renderer->font_measure_string(font_handle, text.c_str(), cursor_index, bounds);

	// draw a cursor
	//const float margin = 2.0f;
	//float horizontal = bounds.size.width + margin;
	//painter.add_line(
	//	gui::Point(horizontal, 0.0f),
	//	gui::Point(horizontal, size.height),
	//	gemini::Color(1.0f, 0.0f, 0.0f),
	//	2
	//);
}

void NumberPanel::handle_event(gui::EventArgs& args)
{
	if (args.focus == this)
	{
	}

	if (args.type == gui::Event_Text)
	{
		//std::string::iterator it = text.begin() + cursor_index;
		//text.insert(it, (char)args.unicode);
		//++cursor_index;
	}

	if (args.type == gui::Event_CursorButtonPressed)
	{
		is_captured = 1;
		down_position = args.local;
		args.handled = 1;
	}
	else if (args.type == gui::Event_CursorButtonReleased)
	{
		is_captured = 0;
		args.handled = 1;
	}

	if (args.type == gui::Event_CursorMove)
	{
		if (is_captured)
		{
			gui::Point delta = args.local - down_position;
			if (value)
			{
				*value += delta.x * step;
				set_text(core::str::format("%2.4f", *value));
				args.handled = 1;
			}
		}
	}

	if (args.type == gui::Event_CursorButtonReleased)
	{
		if (args.cursor_button == gui::CursorButton::Right)
		{
			if (cursor_index < text.length())
			{
				cursor_index += 1;
				args.handled = 1;
			}
		}
		else if (args.cursor_button == gui::CursorButton::Left)
		{
			if (cursor_index > 0)
			{
				cursor_index -= 1;
				args.handled = 1;
			}
		}
	}
}

class ParameterVec3Widget : public gui::Panel
{
public:
	Parameter* target;

	gui::Label* labels[3];
	NumberPanel* panels[3];

	ParameterVec3Widget(gui::Panel* parent, Parameter* parameter)
		: gui::Panel(parent)
		, target(parameter)
	{
		set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

		gui::HorizontalLayout* layout = new gui::HorizontalLayout();



		labels[0] = new gui::Label(this);
		labels[0]->set_background_color(gemini::Color(1.0f, 0.0f, 0.0f));
		labels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		labels[0]->set_font("debug", 24);
		labels[0]->set_text("X:");
		layout->add_panel(labels[0]);
		panels[0] = new NumberPanel(this);
		panels[0]->set_font("debug", 16);
		panels[0]->set_text("0.0000");
		panels[0]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[0]);

		labels[1] = new gui::Label(this);
		labels[1]->set_background_color(gemini::Color(0.0f, 1.0f, 0.0f));
		labels[1]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[1]->set_font("debug", 16);
		labels[1]->set_text("Y:");
		layout->add_panel(labels[1]);
		panels[1] = new NumberPanel(this);
		panels[1]->set_font("debug", 16);
		panels[1]->set_text("0.0000");
		panels[1]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[1]);

		labels[2] = new gui::Label(this);
		labels[2]->set_background_color(gemini::Color(0.0f, 0.0f, 1.0f));
		labels[2]->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
		labels[2]->set_font("debug", 16);
		labels[2]->set_text("Z:");
		layout->add_panel(labels[2]);
		panels[2] = new NumberPanel(this);
		panels[2]->set_font("debug", 16);
		panels[2]->set_text("0.0000");
		panels[2]->set_foreground_color(gemini::Color(0.0f, 0.0f, 0.0f));
		layout->add_panel(panels[2]);


		set_layout(layout);
		parameter_changed();
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override;
	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;

	void parameter_changed()
	{
		glm::vec3* vector3 = reinterpret_cast<glm::vec3*>(target->value);
		panels[0]->value = &vector3->x;
		panels[1]->value = &vector3->y;
		panels[2]->value = &vector3->z;

		panels[0]->set_text(core::str::format("%2.4f", *panels[0]->value));
		panels[1]->set_text(core::str::format("%2.4f", *panels[1]->value));
		panels[2]->set_text(core::str::format("%2.4f", *panels[2]->value));
	}
};

void ParameterVec3Widget::update(gui::Compositor* compositor, float delta_seconds)
{
	gui::Panel::update(compositor, delta_seconds);
}

void ParameterVec3Widget::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
{
	gui::Panel::render(compositor, renderer, render_commands);
}




#if TEST_TELEMETRY_SYSTEM
class TelemetryPanel : public gui::Panel
{
public:

	gui::Point tube[4];

	telemetry_viewer* viewer;

	gui::Point last_position;
	int32_t selected_frame;
	uint32_t adaptive_max_scale;

	float bar_width;
	float min_bar_width;

	class TelemetryInfo : public gui::Panel
	{
		gui::Label* profile_block;
		gui::Label* variable_block;

	public:
		TelemetryInfo(gui::Panel* parent)
			: gui::Panel(parent)
		{
			profile_block = new gui::Label(this);
			profile_block->set_size(250, 100);
			profile_block->set_font("debug", 16);
			profile_block->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
			profile_block->set_foreground_color(gemini::Color(0.0f, 1.0f, 0.0f));
			profile_block->set_name("profile_block");

			variable_block = new gui::Label(this);
			variable_block->set_size(250, 100);
			variable_block->set_origin(0, 100);
			variable_block->set_font("debug", 16);
			variable_block->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
			variable_block->set_foreground_color(gemini::Color(0.0f, 1.0f, 1.0f));
			variable_block->set_name("variable_block");
		}

		void set_profile_block(const char* text)
		{
			profile_block->set_text(text);
		}

		void set_variable_block(const char* text)
		{
			variable_block->set_text(text);
		}

		virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override
		{
			render_geometry(render_commands, background_color);
			//render_capture_rect(render_commands);
			render_background(render_commands);
			render_children(compositor, renderer, render_commands);
		}
	};


	TelemetryInfo* info_panel;


	TelemetryPanel(gui::Panel* parent, telemetry_viewer* telemetry_viewer)
		: gui::Panel(parent)
		, viewer(telemetry_viewer)
	{
		set_background_color(gemini::Color(0.5f, 0.5f, 0.5f));

		tube[0] = gui::Point(0.0f, 0.0f);
		tube[1] = gui::Point(0.0f, 50.0f);
		tube[2] = gui::Point(50.0f, 50.0f);
		tube[3] = gui::Point(50.0f, 0.0f);

		flags |= gui::Panel::Flag_CanMove;
		selected_frame = -1;

		bar_width = 6.0f;

		info_panel = new TelemetryInfo(this);
		info_panel->set_size(250, 200);
		info_panel->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.5f));
		adaptive_max_scale = 100;
	}

	virtual void update(gui::Compositor* compositor, float delta_seconds) override;
	virtual void render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands) override;
	virtual void handle_event(gui::EventArgs& args) override;
};

void TelemetryPanel::update(gui::Compositor* compositor, float delta_seconds)
{
	min_bar_width = (get_client_size().width / static_cast<float>(TELEMETRY_MAX_VIEWER_FRAMES));
	gui::Panel::update(compositor, delta_seconds);
}

void TelemetryPanel::render(gui::Compositor* compositor, gui::Renderer* renderer, gui::render::CommandList& render_commands)
{
	gui::Painter painter(this, render_commands);

	render_geometry(render_commands, background_color);
	render_capture_rect(render_commands);
	render_background(render_commands);

	uint32_t width = get_client_size().width;
	float rect_width = bar_width;

	const gemini::Color current(1.0f, 1.0f, 1.0f);
	const gemini::Color normal(1.0f, 0.0f, 0.0f);
	const gemini::Color selected(1.0f, 0.5f, 0.0f);

	float client_height = get_client_size().height;
	float panel_height = get_size().height;

	const glm::mat3& tx = get_transform(0);
	uint32_t visible_frames = (width / bar_width);

	uint32_t max_scale_current_frame = adaptive_max_scale;
	adaptive_max_scale = 0;

	for (size_t index = 0; index < visible_frames; ++index)
	{
		gui::Point origin = gui::Point((index * rect_width), capture_rect.height());

		// just grab and graph the first record
		debug_record_t* record = &viewer->frames[index].records[0];

		if (viewer->frames[index].total_cycles > adaptive_max_scale)
		{
			adaptive_max_scale = viewer->frames[index].total_cycles;
		}

		float scale = viewer->frames[index].total_cycles / static_cast<float>(max_scale_current_frame);
		float rect_height = scale * static_cast<float>(client_height);

		tube[0] = gui::Point(0.0f, client_height - rect_height);
		tube[1] = gui::Point(0.0f, client_height);
		tube[2] = gui::Point(rect_width, client_height);
		tube[3] = gui::Point(rect_width, client_height - rect_height);

		gemini::Color current_color = normal;
		if (selected_frame != -1 && index == selected_frame)
		{
			current_color = selected;
		}
		else if (index == viewer->current_index)
		{
			current_color = current;
		}

		painter.add_rectangle(
			origin + tube[0],
			origin + tube[1],
			origin + tube[2],
			origin + tube[3],
			gui::render::WhiteTexture,
			current_color
		);
	}

	render_children(compositor, renderer, render_commands);
}

void TelemetryPanel::handle_event(gui::EventArgs& args)
{
	last_position = args.local;

	// Allow the cursor to still drag by the title bar.
	// TODO: Determine WHERE the capture was made. If it wasn't made in
	// the capture rect, the inherited Panel shouldn't handle it. This panel should.
	Panel::handle_event(args);
	if (args.handled || point_in_capture_rect(args.local))
	{
		return;
	}

	if (args.type == gui::Event_CursorDrag || args.type == gui::Event_CursorButtonPressed)
	{
		if (args.type == gui::Event_CursorButtonPressed)
		{
			args.compositor->set_capture(this, args.cursor_button);
		}

		int next_frame = (((int)args.local.x) - 1) / bar_width;

		selected_frame = next_frame;

		LOGV("selected frame: %i\n", next_frame);
		if (selected_frame >= 0 && selected_frame < TELEMETRY_MAX_VIEWER_FRAMES)
		{
			info_panel->set_visible(true);
			float x_offset = args.cursor.x;
			float x_origin = get_origin().x;

			if ((x_origin + x_offset + info_panel->get_size().width) > args.compositor->get_size().width)
			{
				x_offset = (args.compositor->get_size().width - info_panel->get_size().width - x_origin);
			}
			gui::Point info_panel_offset = args.compositor->compositor_to_local(gui::Point(x_offset, 0.0f));

			info_panel->set_origin(info_panel_offset.x, capture_rect.height());

			String profile_block;
			for (size_t index = 0; index < TELEMETRY_MAX_RECORDS_PER_FRAME; ++index)
			{
				debug_record_t* record = &viewer->frames[selected_frame].records[index];
				if (record->cycles > 0)
				{
					profile_block += core::str::format(
						"[%s:%i - %s] Cycles: %i\n",
						record->filename,
						record->line_number,
						record->function,
						record->cycles);
				}
			}
			//profile_block += core::str::format("Total Cycles: %i", viewer->frames[selected_frame].total_cycles);
			info_panel->set_profile_block(profile_block.c_str());

			String variable_block;
			for (size_t index = 0; index < TELEMETRY_MAX_VARIABLES; ++index)
			{
				debug_var_t* variable = &viewer->frames[selected_frame].variables[index];
				if (variable->name[0] > 0)
				{
					if (variable->type == DEBUG_RECORD_TYPE_FLOAT)
					{
						float* value = reinterpret_cast<float*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": %2.2f\n", index, variable->name, *value);
					}
					else if (variable->type == DEBUG_RECORD_TYPE_FLOAT2)
					{
						glm::vec2* value = reinterpret_cast<glm::vec2*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f]\n", index, variable->name, value->x, value->y);
					}
					else if (variable->type == DEBUG_RECORD_TYPE_FLOAT3)
					{
						glm::vec3* value = reinterpret_cast<glm::vec3*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f, %2.2f]\n", index, variable->name, value->x, value->y, value->z);
					}
					else if (variable->type == DEBUG_RECORD_TYPE_FLOAT4)
					{
						glm::vec4* value = reinterpret_cast<glm::vec4*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": [%2.2f, %2.2f, %2.2f, %2.2f]\n", index, variable->name, value->x, value->y, value->z, value->w);
					}
					else if (variable->type == DEBUG_RECORD_TYPE_UINT32)
					{
						uint32_t* value = reinterpret_cast<uint32_t*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": %u\n", index, variable->name, *value);
					}
					else if (variable->type == DEBUG_RECORD_TYPE_INT32)
					{
						int32_t* value = reinterpret_cast<int32_t*>(variable->data);
						variable_block += core::str::format("[%i] \"%s\": %i\n", index, variable->name, *value);
					}
				}
			}
			info_panel->set_variable_block(variable_block.c_str());
		}
		else
		{
			info_panel->set_visible(false);
		}

		args.handled = true;
	}
	else if (args.type == gui::Event_CursorScroll)
	{
		bar_width += args.wheel * 2.0f;
		LOGV("bar width is %2.2f\n", bar_width);
		if (bar_width < min_bar_width)
		{
			bar_width = min_bar_width;
		}
	}
}
#endif

struct EditorEnvironment
{
	Project* project;

	uint32_t open_last_project_on_start;
	gemini::string last_project;

	EditorEnvironment()
		: project(nullptr)
		, open_last_project_on_start(0)
	{

	}
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

	render2::Pipeline* surface_pipeline;
	glm::mat4 surface_modelview;
	glm::mat4 surface_projection;

	glm::mat4 joint_offsets[IMOCAP_TOTAL_SENSORS];

	MyVertex vertex_data[4];

	EditorEnvironment environment;

//	GLsync fence;

	RenderScene* render_scene;
	TransformNode* transform_graph;

	gui::Compositor* compositor;
	gui::Label* log_window;
	GUIRenderer* gui_renderer;
	gui::Panel* main_panel;
	SpringPanel* spring_panel;
	gui::Timeline* timeline;
	ParameterVec3Widget* vec3_widget;
	Parameter test_parameter;
	glm::vec3 test_vec3;
#if TEST_TELEMETRY_SYSTEM
	TelemetryPanel* telemetry_panel;
#endif
	::renderer::StandaloneResourceCache* resource_cache;
	render2::RenderTarget* render_target;
	render2::Texture* texture;
	AssetProcessingPanel* asset_processor;

	gui::Graph* graphs[IMOCAP_TOTAL_SENSORS];
	gui::Label* status;

	Array<imocap::mocap_frame_t> mocap_frames;
	size_t current_mocap_frame;
	PathString current_mocap_filename;

	telemetry_viewer tel_viewer;

	float value;

	core::logging::Handler log_handler;

	EditableMesh single_mesh;

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
	bool app_in_focus;

	glm::vec3 position_test;
	glm::vec3 velocity_test;

	imocap::MocapDevice* mocap_device;

	gemini::MonitorHandle monitor_handles[5];

	gemini::Allocator asset_allocator;
	MonitorDelegate monitor_delegate;
	gemini::Allocator sensor_allocator;
	gemini::Allocator render_allocator;
	gemini::Allocator debugdraw_allocator;
	gemini::Allocator default_allocator;

	Array<core::StackString<32>> mesh_animations;
	uint32_t current_mesh_animation;
	uint32_t animated_mesh;
	uint32_t second_mesh;
	uint32_t attachment_mesh;
	uint32_t enable_animation;
	uint32_t mesh_animation_index;

	glm::vec3* lines;
	size_t current_line_index;

	NotificationServer notify_server;
	NotificationClient notify_client;
	PathDelayHashSet* queued_asset_changes;

	EntityRenderState entity_render_state;

public:
	EditorKernel()
		: active(true)
		, compositor(nullptr)
		, gui_renderer(nullptr)
		, resource_cache(nullptr)
		, render_target(nullptr)
		, texture(nullptr)
		, value(0.0f)
		, is_recording_frames(false)
		, is_playing_frames(false)
		, app_in_focus(true)
		, mocap_device(nullptr)
		, lines(nullptr)
		, status(nullptr)
		, current_line_index(0)
		, mocap_frames(sensor_allocator, 0)
		, mesh_animations(default_allocator)
		, current_mesh_animation(0)
		, animated_mesh(0)
		, second_mesh(0)
		, enable_animation(1)
		, mesh_animation_index(0)
		, single_mesh(default_allocator)
		, test_parameter("Test Value", &test_vec3)
	{
		yaw = 0.0f;
		pitch = 0.0f;

		left_down = right_down = forward_down = backward_down = false;

		last_time = 0;

		should_move_view = false;

		asset_processor = nullptr;

		timeline = nullptr;

		memset(monitor_handles, 0, sizeof(MonitorHandle) * 5);
	}

	virtual ~EditorKernel()
	{
	}

	virtual bool is_active() const { return active; }
	virtual void set_active(bool isactive) { active = isactive; }

	virtual void event(kernel::KeyboardEvent& event)
	{
		if (event.key == gemini::BUTTON_ESCAPE && event.is_down)
		{
			set_active(false);
		}

		if (compositor && compositor->get_focus())
		{
			if (event.is_text)
			{
				compositor->text_event(event.unicode);
			}
			else
			{
				compositor->key_event(event.is_down, event.key, 0);
			}
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

		if (event.is_down)
		{
			if (event.key == BUTTON_SPACE)
			{
				LOGV("freezing rotations\n");
				imocap::zero_rotations(mocap_device);

				position_test = velocity_test = glm::vec3(0.0f, 0.0f, 0.0f);
			}

			if (event.key == BUTTON_F2)
			{
				runtime_unload_rapid();
				LOGV("unloaded rapid interface\n");

			}
			else if (event.key == BUTTON_F3)
			{
				runtime_load_rapid();
				LOGV("loading rapid interface\n");
			}

			if (event.key == BUTTON_F4)
			{
				LOGV("take a snapshot\n");
				render2::Image image(render_allocator);
				image.create(render_target->width, render_target->height, 4);
				device->render_target_read_pixels(render_target, image);
				image::save_image_to_file(image, "test.png");
			}

			if (event.key == BUTTON_F5)
			{
				++current_mesh_animation;
				if (current_mesh_animation >= mesh_animations.size())
				{
					current_mesh_animation = 0;
				}

				LOGV("Playing animation: \"%s\"\n", mesh_animations[current_mesh_animation]());
				mesh_animation_index = render_scene_animation_play(render_scene, animated_mesh, mesh_animations[current_mesh_animation](), 0);
				render_scene_animation_play(render_scene, animated_mesh, "look_right", 1);

				//render_scene_animation_play(render_scene, second_mesh, "idle", 0);

				update_timeline_frames();
			}
			else if (event.key == BUTTON_SPACE)
			{
				enable_animation = !enable_animation;
				LOGV("Animation is now %s\n", enable_animation ? "ON" : "OFF");
			}
			else if (event.key == BUTTON_MINUS)
			{
				kernel::parameters().simulation_time_scale -= 0.1f;
				kernel::parameters().simulation_time_scale = glm::clamp(kernel::parameters().simulation_time_scale, 0.0f, 3.0f);
				LOGV("set timescale to %2.2f\n", kernel::parameters().simulation_time_scale);
			}
			else if (event.key == BUTTON_PLUS)
			{
				kernel::parameters().simulation_time_scale += 0.1f;
				kernel::parameters().simulation_time_scale = glm::clamp(kernel::parameters().simulation_time_scale, 0.0f, 3.0f);
				LOGV("set timescale to %2.2f\n", kernel::parameters().simulation_time_scale);
			}
		}
	}

	void update_timeline_frames()
	{
		// update the timeline with the new animation
		if (timeline && animated_mesh != 0)
		{
			uint32_t total_frames = render_scene_animation_total_frames(render_scene, animated_mesh, 0);
			timeline->set_frame(0);
			timeline->set_frame_range(0, total_frames);
		}
	}

	void on_asset_reload(uint32_t channel, void* data, uint32_t data_size)
	{
		platform::PathString path = (const char*)data;
		path.recompute_size();
		assert(data_size == path.size());

		if (channel == 1)
		{
			path = path.basename();
			path = path.remove_extension();
			LOGV("reloading SHADER asset: %s\n", path());
			shader_load(path(), true);
		}
		else if (channel == 2)
		{
			path = path.remove_extension();
			LOGV("reloading TEXTURE asset: %s\n", path());
		}
		else if (channel == 3)
		{
			path = path.remove_extension();
			LOGV("reload material: %s\n", path());
			material_load(path(), true);
		}
		else if (channel == 4)
		{
			path = path.remove_extension();
			LOGV("reload model: %s\n", path());
		}
		else if (channel == 5)
		{
			path = path.basename();
			LOGV("reload config: %s\n", path());
		}
	}

	void on_file_updated(MonitorHandle monitor_handle, MonitorAction action, const platform::PathString& path)
	{
		if (action == MonitorAction::Modified)
		{
			ModifiedAssetData mod;
			mod.quiet_time_left = ASSET_CHANGE_NOTIFICATION_QUIET_SECONDS;
			mod.monitor_handle = monitor_handle;

			// make this path a relative path to the project root.
			if (environment.project)
			{
#if 0
				PathString project_root = environment.project->get_root_path().c_str();
				project_root.normalize(PATH_SEPARATOR);
				PathString asset_root = gemini::runtime_platform_asset_root(project_root);
				PathString relative_path;
				core::filesystem::relative_path_from_absolute(relative_path, path(), asset_root);
#endif
				(*queued_asset_changes)[path] = mod;
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

			main_panel->set_size(event.window_width, event.window_height-24);
			camera.perspective(60.0f, (int)event.window_width, (int)event.window_height, 0.01f, 1024.0f);
		}
		else if (event.subtype == kernel::WindowClosed)
		{
			LOGV("Window was closed!\n");
			set_active(false);
		}

		if (event.window == main_window)
		{
			if (event.subtype == kernel::WindowRestored || event.subtype == kernel::WindowGainFocus)
			{
				app_in_focus = true;
			}
			else if (event.subtype == kernel::WindowMinimized || event.subtype == kernel::WindowLostFocus)
			{
				app_in_focus = false;
			}
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
				bool handled = compositor->cursor_button(input_to_gui[event.button], event.is_down);
				if (!handled)
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
				}
			}
			else if (event.subtype == kernel::MouseWheelMoved)
			{
				compositor->cursor_scroll(event.wheel_direction);
			}
		}
	}

	void test_open_dialog()
	{
		gemini::Allocator allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		Array<PathString> paths(allocator);
		uint32_t flags = platform::OpenDialogFlags::CanChooseDirectories;

		if (platform::show_open_dialog("Choose Game Directory", flags, paths).succeeded())
		{
			LOGV("target path is: %s\n", paths[0]());
		}
	}

	// menu handlers

	void on_file_new(void)
	{
		uint32_t open_flags = platform::OpenDialogFlags::CanChooseDirectories | platform::OpenDialogFlags::CanCreateDirectories;

		gemini::Allocator temp = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		Array<PathString> paths(temp);
		platform::Result result = platform::show_open_dialog("Choose project root", open_flags, paths);
		if (result.succeeded())
		{
			assert(environment.project == nullptr);

			Project* project = Project::create_project();
			environment.project = project;

			project->set_name("TestProject");
			project->camera_position = camera.get_position();
			project->camera_yaw = camera.get_yaw();
			project->camera_pitch = camera.get_pitch();
			platform::PathString project_path = paths[0];
			project->set_root_path(project_path());
			project_path.append(PATH_SEPARATOR_STRING);
			project_path.append("project.conf");

			project->save_project_as(project_path());

			LOGV("saved path: %s\n", paths[0]());
		}
	}


	uint32_t on_test_click(platform::PlatformDialogEvent& event)
	{
		if (event.type == OpenDialogEventType::OkClicked)
		{
			// does a project file exist at path?
			platform::PathString project_path = event.filename;
			project_path.append(PATH_SEPARATOR_STRING);
			project_path.append("project.conf");
			if (!core::filesystem::instance()->file_exists(project_path(), false))
			{
				// TODO: Alert the user that this directory doesn't contain
				// a project file.
				LOGW("file '%s' does not exist\n", project_path());
				return 1;
			}
		}

		return 0;
	}

	void on_file_open()
	{
		uint32_t open_flags = platform::OpenDialogFlags::CanChooseDirectories;

		gemini::Allocator temp = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		Array<PathString> paths(temp);
		platform::open_dialog_event_handler delegate;
		delegate.bind<EditorKernel, &EditorKernel::on_test_click>(this);
		platform::Result result = platform::show_open_dialog("Choose project", open_flags, paths, delegate);
		if (result.succeeded())
		{
			platform::PathString project_path = paths[0];

			open_project_at_root(project_path);

			//camera.set_position(environment.project->camera_position);
			//camera.set_yaw(environment.project->camera_yaw);
			//camera.set_pitch(environment.project->camera_pitch);
		}
	}

	void open_project_at_root(const PathString& project_root)
	{
		PathString project_path = project_root;

		PathString asset_root = gemini::runtime_platform_asset_root(project_root);


		// Add the project path to the list of virtual search paths.
		core::filesystem::instance()->virtual_add_root(asset_root());

		project_path.append(PATH_SEPARATOR_STRING);
		project_path.append("project.conf");

		environment.project = Project::open_project(project_path());

		if (monitor_handles[0] != 0)
		{
			// release the old directory monitors before re-adding.
			for (size_t index = 0; index < 5; ++index)
			{
				directory_monitor_remove(monitor_handles[index]);
			}
		}

		// hook up the asset monitor delegate to this new project directory.
		platform::PathString watch_path = asset_root;
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("shaders");
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("150");
		watch_path.normalize(PATH_SEPARATOR);
		monitor_handles[0] = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = asset_root;
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("textures");
		watch_path.normalize(PATH_SEPARATOR);
		monitor_handles[1] = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = asset_root;
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("materials");
		watch_path.normalize(PATH_SEPARATOR);
		monitor_handles[2] = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = asset_root;
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("models");
		watch_path.normalize(PATH_SEPARATOR);
		monitor_handles[3] = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = asset_root;
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("conf");
		watch_path.normalize(PATH_SEPARATOR);
		monitor_handles[4] = directory_monitor_add(watch_path(), monitor_delegate);


		// EXPERIMENTAL ZONE
		{
			// load a single mesh for testing
			gemini::AssetHandle mesh_handle = mesh_load("models/test_model/test_model");
			gemini::Mesh* mesh_instance = mesh_from_handle(mesh_handle);
			single_mesh.load_from(default_allocator, mesh_instance);
		}

	} // open_project_at_root

	void on_file_save_project()
	{
		// If you hit this, there's no opened project.
		assert(environment.project);

		if (environment.project)
		{
			environment.project->camera_position = camera.get_position();
			environment.project->camera_yaw = camera.get_yaw();
			environment.project->camera_pitch = camera.get_pitch();
			environment.project->save_project();
		}
	}

	void on_file_save_project_as()
	{
	}

	void on_file_quit(void)
	{
		set_active(false);
	}

	void on_project_build(void)
	{
		if (environment.project)
		{
			const String& path = environment.project->get_root_path();
			LOGV("project root path is: %s\n", path.c_str());

			// 1. Define source / destination folders.
			//    platform specific versions, including shader types, etc.
			// 2. Determine what processes to perform.
			//	  Copy, Convert, Compress, etc.

			// copy: conf, fonts, materials, shaders
			// copy now, convert later: models
			// convert: sounds, textures

			// create a file watcher for the project root.
		}
		else
		{
			LOGV("BUILD: No project loaded.\n");
		}
	}

	void on_record_start(void)
	{
		if (is_recording_frames == false)
		{
			current_mocap_filename.clear();
			Array<PlatformExtensionDescription> extensions(sensor_allocator);
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
				uint32_t total_sensors = IMOCAP_TOTAL_SENSORS;
				platform::fs_write(handle, &total_sensors, sizeof(uint32_t), 1);

				// 2. Total frames.
				const size_t total_frames = mocap_frames.size();
				platform::fs_write(handle, &total_frames, sizeof(size_t), 1);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					imocap::mocap_frame_t& frame = mocap_frames[index];

					for (size_t pose = 0; pose < IMOCAP_TOTAL_SENSORS; ++pose)
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
		Array<PathString> paths(sensor_allocator);
		platform::Result open_result = platform::show_open_dialog("Choose Sensor Stream", OpenDialogFlags::CanChooseFiles, paths);
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
				assert(total_sensors == IMOCAP_TOTAL_SENSORS);

				// 2. Read Total Frames
				size_t total_frames = 0;
				platform::fs_read(handle, &total_frames, sizeof(size_t), 1);

				LOGV("found %i total frames...\n", total_frames);
				mocap_frames.resize(total_frames);

				// 3. All frames.
				for (size_t index = 0; index < total_frames; ++index)
				{
					imocap::mocap_frame_t& frame = mocap_frames[index];
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


	void load_preferences(const PathString& preferences_file)
	{
		environment.last_project = string_create(default_allocator, "x:/games/vrpowergrid");
		environment.open_last_project_on_start = 1;
	}

	void save_preferences(const PathString& preferences_file)
	{
		//gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
		//core::util::ResizableMemoryStream stream(allocator);

		//uint32_t name_length = name.length();
		//stream.write(&name_length, sizeof(uint32_t));
		//stream.write(&name[0], name_length);

		//stream.write(&camera_position, sizeof(glm::vec3));
		//stream.write(&camera_yaw, sizeof(float));
		//stream.write(&camera_pitch, sizeof(float));

		platform::File handle = platform::fs_open(preferences_file(), platform::FileMode_Write);
		if (!handle.is_open())
		{
			LOGW("Unable to open file: %s\n", preferences_file.c_str());
			return;// platform::Result::failure("Couldn't open file");
		}

		//platform::fs_write(handle, stream.get_data(), 1, stream.get_data_size());
		platform::fs_close(handle);
	}

	void timeline_scrubber_changed(size_t current_frame)
	{
		int lower, upper;
		timeline->get_frame_range(lower, upper);
		value = mathlib::PI * 2.0 * (current_frame / static_cast<float>(upper));

		render_scene->light_position_world.x = cosf(value);
		render_scene->light_position_world.y = 2.0f;
		render_scene->light_position_world.z = sinf(value);

		// disable automatic animation advance if the user is scrubbing.
		enable_animation = 0;

		render_scene_animation_set_frame(render_scene, animated_mesh, current_frame, 0);
	}

	void render_main_content(render2::RenderTarget* render_target)
	{
#if 0
		render2::Pass render_pass;
		render_pass.color(0.0f, 0.0f, 0.0, 0.0f);
		render_pass.clear_color = false;
		render_pass.clear_depth = false;
		render_pass.depth_test = false;
		render_pass.target = render_target;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(surface_pipeline);

		const bool test_triangle = 0;
		if (test_triangle)
		{
			serializer->vertex_buffer(vertex_buffer);
			serializer->draw(0, 3);
		}
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		render_scene->camera_position_world = camera.get_position();
		render_scene->camera_view_direction = camera.get_view();
		render_scene_draw(render_scene, device, camera.get_modelview(), camera.get_projection(), render_target);

		debugdraw::render(camera.get_modelview(), camera.get_projection(), render_target->width, render_target->height, render_target);
	}

	virtual kernel::Error startup()
	{
		{
			gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
			ArgumentParser parser;
			argparse_create(allocator, &parser);

			gemini::string asset_path;
			argparse_arg(&parser, "The path to load content from", "--assets", nullptr, &asset_path);

			Array<gemini::string> arguments(allocator);
			runtime_load_arguments(allocator, arguments);
			if (argparse_parse(&parser, arguments) != 0)
			{
				argparse_destroy(allocator, &parser);
				runtime_destroy_arguments(allocator, arguments);
				return kernel::CoreFailed;
			}

			gemini::runtime_startup("arcfusion.net/orion", platform::make_absolute_path(asset_path.c_str()));

			argparse_destroy(allocator, &parser);
			runtime_destroy_arguments(allocator, arguments);
		}

		default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

		// try to load editor preferences...
		{
			core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
			PathString user_preferences_file = filesystem->user_application_directory();

			user_preferences_file.append(PATH_SEPARATOR_STRING);
			user_preferences_file.append("editor.preferences");

			if (filesystem->file_exists(user_preferences_file(), false))
			{
				load_preferences(user_preferences_file);
			}
			else
			{
				save_preferences(user_preferences_file);
			}
		}

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
			//camera.set_position(glm::vec3(0.0f, 5.0f, 10.0f));

			// test for rendering mocap suit
			//camera.set_position(glm::vec3(0.69f, 0.55f, 0.45f));
			//camera.set_yaw(-78.15f);
			//camera.set_pitch(31.65f);

			// test for rendering cubes
			//camera.set_position(glm::vec3(-2.10f, 1.24f, 1.10f));
			//camera.set_yaw(71.70f);
			//camera.set_pitch(12.45f);

			// test for rendering cube_rig2
			camera.set_position(glm::vec3(-1.51, 3.67f, 1.21f));
			camera.set_yaw(52.35f);
			camera.set_pitch(48.90f);

			camera.set_type(Camera::FIRST_PERSON);
			camera.update_view();
		}

		platform::window::Frame window_frame;

		asset_allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);
		assert(directory_monitor_startup(asset_allocator) == 0);

		queued_asset_changes = MEMORY2_NEW(asset_allocator, PathDelayHashSet)(asset_allocator);

		int32_t startup_result = net_startup();
		assert(startup_result == 0);

		// Determine how to handle hotloading for the assets used by orion.
		// We mainly want to catch assets for a specific project and forward
		// those notifications to the engine.
		monitor_delegate.bind<EditorKernel, &EditorKernel::on_file_updated>(this);
#if 0
		monitor_delegate.bind<EditorKernel, &EditorKernel::on_file_updated>(this);

		platform::PathString watch_path = core::filesystem::instance()->content_directory();
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("shaders");
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("150");

		monitor_zero = directory_monitor_add(watch_path(), monitor_delegate);

		watch_path = core::filesystem::instance()->content_directory();
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("textures");
		monitor_one = directory_monitor_add(watch_path(), monitor_delegate);
		on
		watch_path = core::filesystem::instance()->content_directory();
		watch_path.append(PATH_SEPARATOR_STRING);
		watch_path.append("materials");
		monitor_materials = directory_monitor_add(watch_path(), monitor_delegate);
#endif
		// initialize the renderer
		{
			render_allocator = memory_allocator_default(MEMORY_ZONE_RENDERER);

			NotifyMessageDelegate on_message_notify;
			on_message_notify.bind<EditorKernel, &EditorKernel::on_asset_reload>(this);
			hotloading::startup(render_allocator, on_message_notify);

			debugdraw_allocator = memory_allocator_default(MEMORY_ZONE_DEBUGDRAW);

			using namespace render2;
			RenderParameters params(render_allocator);

			// set some options
			params["vsync"] = "true";
			params["double_buffer"] = "true";
			params["gamma_correct"] = "true";
			params["depth_size"] = "24";
//			params["multisample"] = "4";

			// set opengl specific options
			//params["rendering_backend"] = "opengl";


			device = create_device(render_allocator, params);
			assert(device != nullptr);

			assets::startup(device, true);
			render_scene_startup(device, render_allocator);

			window_frame = platform::window::get_frame(main_window);
			device->init(window_frame.width, window_frame.height);

			// setup shaders
			render2::PipelineDescriptor desc;
			desc.shader = shader_load("vertexcolor");
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_color", render2::VD_FLOAT, 4);
			desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
			pipeline = device->create_pipeline(desc);
			pipeline->constants().set("modelview_matrix", &modelview_matrix);
			pipeline->constants().set("projection_matrix", &projection_matrix);

			{
				render2::PipelineDescriptor desc;
				desc.shader = shader_load("vertexcolor");
				render2::VertexDescriptor& vertex_format = desc.vertex_description;
				vertex_format.add("in_position", render2::VD_FLOAT, 3);
				vertex_format.add("in_color", render2::VD_FLOAT, 4);
				desc.input_layout = device->create_input_layout(vertex_format, desc.shader);
				surface_pipeline = device->create_pipeline(desc);
				surface_pipeline->constants().set("modelview_matrix", &surface_modelview);
				surface_pipeline->constants().set("projection_matrix", &surface_projection);
			}

			vertex_buffer = device->create_vertex_buffer(total_bytes);


			// Buffer Lock and Buffer Unlock are perhaps future features.
			// This does not work as of yet.
#if 0
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
#else
			MyVertex* vertex = vertex_data;
			vertex[0].set_position(0, window_frame.height, 0);
			vertex[0].set_color(1.0f, 0.0f, 0.0f, 1.0f);

			vertex[1].set_position(window_frame.width, window_frame.height, 0);
			vertex[1].set_color(0.0f, 1.0f, 0.0f, 1.0f);

			vertex[2].set_position(window_frame.width / 2.0f, 0, 0);
			vertex[2].set_color(0.0f, 0.0f, 1.0f, 1.0f);

			vertex[3].set_position(0, 0, 0);
			vertex[3].set_color(0.0f, 1.0f, 1.0f, 1.0f);

			device->buffer_upload(vertex_buffer, vertex, total_bytes);
#endif
		}

		// setup editor assets / content paths
		{
//			fs->add_virtual_path("editor/assets");
		}


		render_scene = render_scene_create(render_allocator, device);
		transform_graph = transform_graph_create_node(render_allocator, "root");

		animation::startup(asset_allocator);

		// initialize debug draw
		debugdraw::startup(debugdraw_allocator, device);

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
			resource_cache = MEMORY2_NEW(render_allocator, StandaloneResourceCache)(render_allocator);

			gui_renderer = MEMORY2_NEW(render_allocator, GUIRenderer)(render_allocator, *resource_cache);
			gui_renderer->set_device(device);

			gui::set_allocator(render_allocator);

			assert(window_frame.width > 0);
			assert(window_frame.height > 0);
			compositor = new gui::Compositor(window_frame.width, window_frame.height, resource_cache, gui_renderer);
			compositor->set_name("compositor");

#if ENABLE_UI
			main_panel = new gui::Panel(compositor);
			main_panel->set_name("main_panel");
			main_panel->set_origin(0, 24);
			main_panel->set_size(window_frame.width, window_frame.height-24);


			gui::HorizontalLayout* horizontal_layout = new gui::HorizontalLayout();
			gui::VerticalLayout* center_layout = new gui::VerticalLayout();
			main_panel->set_layout(horizontal_layout);
			horizontal_layout->add_layout(center_layout);
#endif

#if TEST_SPRING_SYSTEM
			spring_panel = new SpringPanel(compositor);
			spring_panel->set_origin(100, 100);
			spring_panel->set_size(400, 400);
			spring_panel->set_name("spring_panel");
#endif


			const char dev_font[] = "debug";
			const size_t dev_font_size = 16;
#if DRAW_SENSOR_GRAPHS
			// Create a graph for each sensor

			uint32_t origin = 24;

			for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
			{
				gui::Graph* graph = new gui::Graph(compositor);
				graph->set_origin(window_frame.width - 250, origin);
				graph->set_size(250, 100);
				origin += 108;
				graph->set_maximum_size(gui::Sizes(250, 100));
				graph->set_font(dev_font, dev_font_size);
				graph->set_background_color(gemini::Color::from_rgba(60, 60, 60, 255));
				graph->set_foreground_color(gemini::Color::from_rgba(255, 255, 255, 255));
				graph->create_samples(100, 3);
				graph->configure_channel(0, gemini::Color::from_rgba(255, 0, 0, 255));
				graph->configure_channel(1, gemini::Color::from_rgba(0, 255, 0, 255));
				graph->configure_channel(2, gemini::Color::from_rgba(0, 0, 255, 255));
				graph->set_range(-0.5f, 0.5f);
				graph->enable_baseline(true, 0.0f, gemini::Color::from_rgba(64, 64, 64, 255));

				graphs[index] = graph;
			}
#endif

			//status = new gui::Label(compositor);
			//status->set_origin(10, 100);
			//status->set_size(150, 75);
			//status->set_font(dev_font, dev_font_size);
			//status->set_text("");
			//status->set_foreground_color(gemini::Color(1.0f, 1.0f, 1.0f));
			//status->set_background_color(gemini::Color(0.0f, 0.0f, 0.0f, 0.0f));

#if 1
			asset_processor = new AssetProcessingPanel(compositor);
			asset_processor->set_origin(0.0f, 25.0f);
			asset_processor->set_size(400, 100);
			asset_processor->set_background_color(gemini::Color(0.25f, 0.25f, 0.25f));
			asset_processor->set_visible(false);
#endif

// add a menu
#if 1
			gui::MenuBar* menubar = new gui::MenuBar(compositor);
			{
				gui::Menu* filemenu = new gui::Menu("File", menubar);

				filemenu->add_item("New Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_new, this));
				filemenu->add_item("Open Project...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_open, this));
				filemenu->add_separator();
				filemenu->add_item("Save Project", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_save_project, this));
				filemenu->add_item("Save Project As...", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_save_project_as, this));
				filemenu->add_separator();
				filemenu->add_item("Quit", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_file_quit, this));
				menubar->add_menu(filemenu);

				gui::Menu* project = new gui::Menu("Project", menubar);
				project->add_item("Build", MAKE_MEMBER_DELEGATE(void(), EditorKernel, &EditorKernel::on_project_build, this));
				menubar->add_menu(project);

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

#if ENABLE_UI
			gui::RenderableSurface* surface = new gui::RenderableSurface(main_panel);
			surface->on_render_content.bind<EditorKernel, &EditorKernel::render_main_content>(this);

			image::Image checker_pattern(render_allocator);
			checker_pattern.create(512, 512, 3);
			checker_pattern.fill(gemini::Color::from_rgba(0, 25, 25, 255));
			texture = device->create_texture(checker_pattern);

			int handle = resource_cache->track_texture(texture);

			// TODO: sort out this interface!
			render_target = device->create_render_target(texture);
			surface->set_device(device);
			surface->set_render_target(render_target);
			surface->set_texture_handle(handle);

			center_layout->add_panel(surface);
#endif

#if ENABLE_UI
			timeline = new gui::Timeline(main_panel);
			timeline->set_frame_range(0, 30);
			timeline->on_scrubber_changed.bind<EditorKernel, &EditorKernel::timeline_scrubber_changed>(this);
			timeline->set_frame(0);
			timeline->set_maximum_size(gui::Size(0, 60));

			center_layout->add_panel(timeline);

			vec3_widget = new ParameterVec3Widget(main_panel, &test_parameter);
			vec3_widget->set_maximum_size(gui::Size(1024, 200));

			horizontal_layout->add_panel(vec3_widget);
#endif




#if 0
			// TODO: This needs more work as it can still get swamped and
			// latency increases dramatically.
			log_window = new gui::Label(compositor);
			log_window->set_origin(0.0f, 450);
			log_window->set_size(500, 250);
			log_window->set_font("debug", 16);
			log_window->set_name("log_window");
			log_window->set_foreground_color(gemini::Color(0.85f, 0.85f, 0.85f));
			log_window->set_background_color(gemini::Color(0.10f, 0.10f, 0.10f));
			uint32_t current_flags = log_window->get_flags();
			log_window->set_flags(gui::Panel::Flag_CursorEnabled | gui::Panel::Flag_CanMove | current_flags);

			// install a log handler
			log_handler.open = log_window_logger_open;
			log_handler.close = log_window_logger_close;
			log_handler.message = log_window_logger_message;
			log_handler.userdata = (void*)log_windlabow;
			core::logging::instance()->add_handler(&log_handler);

			LOGV("log initialized.\n");
#endif


		}
#endif



		kernel::parameters().step_interval_seconds = (1.0f/50.0f);

		telemetry_viewer_create(&tel_viewer, 120, "0.0.0.0", TELEMETRY_VIEWER_PORT);

#if TEST_TELEMETRY_SYSTEM
		telemetry_panel = new TelemetryPanel(compositor, &tel_viewer);
		telemetry_panel->set_origin(100, 100);
		telemetry_panel->set_size(600, 200);
		telemetry_panel->set_name("telemetry_panel");
#endif

#if TEST_TELEMETRY_HOST
		telemetry_host_startup("127.0.0.1", TELEMETRY_VIEWER_PORT);
#endif


		sensor_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);
		imocap::startup(sensor_allocator);

		mocap_device = imocap::device_create();

		notify_server_create(&notify_server);
		//notify_client_create(&notify_client);

		//NotifyMessageDelegate channel_delegate;
		//channel_delegate.bind<EditorKernel, &EditorKernel::on_asset_reload>(this);
		//notify_client_subscribe(&notify_client, 1, channel_delegate);
		//notify_client_subscribe(&notify_client, 2, channel_delegate);
		//notify_client_subscribe(&notify_client, 3, channel_delegate);


		lines = new glm::vec3[TOTAL_LINES];

		last_time = platform::microseconds();

		// At this point the editor is ready to go.
		if (environment.open_last_project_on_start && !environment.last_project.empty())
		{
			open_project_at_root(environment.last_project.c_str());
		}



		//test_load_model("models/cube_rig2/cube_rig2");
		//test_load_model("models/spiderbot/spiderbot");

		//test_load_model("models/test_model/test_model");

		//test_load_model("models/test_character/test_character");
		TransformNode* node0 = nullptr;
		TransformNode* node1 = nullptr;
		TransformNode* attachment = nullptr;

		node0 = test_load_model("test_model", "models/test_model/test_model", 0, &animated_mesh);
		//node1 = test_load_model("enemy", "models/test_enemy/test_enemy", 1, &second_mesh);

		attachment = test_load_model("attachment", "models/test_attachment/test_attachment", 30, &attachment_mesh);

		TransformNode* attachment_point = transform_graph_find_child(node0, "weapon");
		assert(attachment_point);
		//AnimatedMeshComponent* component = render_scene_get_animated_component(render_scene, second_mesh);

		//Mesh* mesh = mesh_from_handle(component->mesh_handle);
		//ModelAttachment* modelattachment = mesh->attachments_by_name["weapon"];
		//modelattachment->

		test_parameter.value = &attachment_point->position;

		//test_parameter.value = &node1->position;
		vec3_widget->parameter_changed();

		render_scene_animation_play(render_scene, animated_mesh, "idle", 0);


		transform_graph_set_parent(attachment, attachment_point);


		if (node1)
		{
			node1->position = glm::vec3(2.0f, 0.0f, 0.0f);
			render_scene_animation_play(render_scene, second_mesh, "idle", 0);
		}

		//test_load_model("models/test_attachment/test_attachment");

#if 0
		AssetHandle test_mesh = mesh_load("models/vault");
		//AssetHandle plane_rig = mesh_load("models/plane_rig/plane");
		AssetHandle animated_mesh;
		animated_mesh = mesh_load("models/cube_rig/cube_rig");
		//animated_mesh = mesh_load("models/chest_rig/chest_rig");
		//animated_mesh = mesh_load("models/isocarbon_rig/isocarbon_rig");

		glm::mat4 transform(1.0f);

		const uint32_t TOTAL_STATIC_MESHES = 1;

		for (size_t index = 0; index < TOTAL_STATIC_MESHES; ++index)
		{
			uint16_t entity_index = index;
			render_scene_add_static_mesh(render_scene, test_mesh, entity_index, transform);
			entity_render_state.model_matrix[entity_index] = transform;
			transform = glm::translate(transform, glm::vec3(1.5f, 0.0f, 0.0f));
		}

		transform = glm::rotate(transform, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		for (size_t index = 0; index < 4; ++index)
		{
			uint16_t entity_index = TOTAL_STATIC_MESHES + index;
			uint32_t component_id = render_scene_add_animated_mesh(render_scene, animated_mesh, entity_index, transform);
			if (index == 2)
			{
				render_scene_animation_play(render_scene, component_id, "wiggle"); a
			}
			else
			{
				render_scene_animation_play(render_scene, component_id, "idle");re
			}

			entity_render_state.model_matrix[entity_index] = transform;
			transform = glm::translate(transform, glm::vec3(-3.0f, 0.0f, 0.0f));
		}
#endif


		return kernel::NoError;
	}

	TransformNode* test_load_model(const char* name, const char* model_path, uint32_t entity_index, uint32_t* component_handle)
	{
		AssetHandle skeleton_mesh = mesh_load(model_path);

		uint32_t model_component_handle;

		TransformNode* transform_node = nullptr;

		Mesh* mesh = mesh_from_handle(skeleton_mesh);
		if (mesh)
		{
			if (!mesh->skeleton.empty())
			{
				// load an animated mesh
				transform_node = transform_graph_create_hierarchy(render_allocator, mesh->skeleton, mesh->attachments, name);
				transform_node->entity_index = entity_index;
				//transform_node->orientation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				transform_graph_set_parent(transform_node, transform_graph);
				model_component_handle = render_scene_add_animated_mesh(render_scene, skeleton_mesh, transform_node->transform_index);
				animation_link_transform_and_component(transform_node, render_scene_get_animated_component(render_scene, model_component_handle));

				if (entity_index == 0)
				{
					HashSet<core::StackString<32>, uint32_t>::Iterator iter = mesh->sequence_index_by_name.begin();
					for (; iter != mesh->sequence_index_by_name.end(); ++iter)
					{
						LOGV("Found animation: %s\n", iter.key()());
						mesh_animations.push_back(iter.key());
					}

					if (!mesh_animations.empty())
					{
						// Start playing the first animation if there are animations.
						mesh_animation_index = render_scene_animation_play(render_scene, model_component_handle, mesh_animations[0](), 0);
						update_timeline_frames();
					}
				}
			}
			else
			{
				enable_animation = false;

				// load a static mesh
				transform_node = transform_graph_create_node(render_allocator, "test");
				transform_node->entity_index = entity_index;
				transform_graph_set_parent(transform_node, transform_graph);
				model_component_handle = render_scene_add_static_mesh(render_scene, skeleton_mesh, transform_node->transform_index);
			}
		}

		*component_handle = model_component_handle;
		return transform_node;
	}

	void tick_queued_asset_changes(PathDelayHashSet& hashset, float tick_seconds)
	{
		uint32_t handle_to_channel[] = {
			1,
			2,
			3,
			4,
			5
		};
		PathDelayHashSet::Iterator it = hashset.begin();
		for (; it != hashset.end(); ++it)
		{
			ModifiedAssetData& data = it.value();
			assert(data.monitor_handle > 0 && data.monitor_handle < 6);

			data.quiet_time_left -= tick_seconds;
			if (data.quiet_time_left <= 0.0f)
			{
				NotifyMessage message;
				platform::PathString asset_path = it.key();
				notify_message_string(message, asset_path);
				message.channel = handle_to_channel[data.monitor_handle - 1];
				notify_server_publish(&notify_server, &message);

				it = hashset.remove(it);
				continue;
			}
		}
	} // tick_queued_asset_changes

	virtual void tick()
	{
		uint64_t current_time = platform::microseconds();
		platform::update(kernel::parameters().framedelta_milliseconds);

		static float accumulator = 0.0f;

		accumulator += kernel::parameters().framedelta_seconds;
		while (accumulator >= kernel::parameters().step_interval_seconds)
		{
			accumulator -= kernel::parameters().step_interval_seconds;
		}

		kernel::parameters().step_alpha = glm::clamp(static_cast<float>(accumulator / kernel::parameters().step_interval_seconds), 0.0f, 1.0f);

		telemetry_viewer_tick(&tel_viewer, kernel::parameters().framedelta_seconds);

		// while i debug network stuff; don't do this...
		//if (!app_in_focus)
		//{
		//	return;
		//}

		directory_monitor_update();
		notify_server_tick(&notify_server);
		hotloading::tick();
		//notify_client_tick(&notify_client);
		tick_queued_asset_changes(*queued_asset_changes, kernel::parameters().framedelta_seconds);

		if (enable_animation)
		{
			// TODO: update the timeline
			uint32_t current_frame = render_scene_animation_current_frame(render_scene, animated_mesh, 0);
			if (timeline)
			{
				timeline->set_frame(current_frame);
			}

			animation::update(kernel::parameters().simulation_delta_seconds);
		}


		{
			// Copy frame state for each entity into their respective transform nodes.
			//transform_graph_copy_frame_state(transform_graph, &frame_state);

			//transform_graph_print(transform_graph);


			// Update transform nodes with current animation poses
			animation_update_transform_nodes();

			// get the latest world matrices from the transform graph
			glm::mat4 world_matrices[256];
			transform_graph_transform(transform_graph, world_matrices);

			// copy transforms from nodes to animated components
			animation_update_components();

			// update world matrices for scene rendering
			render_scene_update(render_scene, world_matrices);
		}

		// See if we need to poke the animated mesh.
		if (animated_mesh != 0)
		{
			if (render_scene_animation_finished(render_scene, animated_mesh, 0))
			{
				if (enable_animation)
				{
					mesh_animation_index = render_scene_animation_play(render_scene, animated_mesh, mesh_animations[current_mesh_animation](), 0);
				}
			}

			if (second_mesh != 0)
			{
				if (render_scene_animation_finished(render_scene, second_mesh, 0))
				{
					render_scene_animation_play(render_scene, second_mesh, "idle", 0);
				}
			}

			//if (render_scene_animation_finished(render_scene, animated_mesh, 1))
			//{
			//	if (enable_animation)
			//	{
			//		render_scene_animation_play(render_scene, animated_mesh, "look_right", 1);
			//	}
			//}
		}



		static float value = 0.0f;
		static float multiplifer = 1.0f;

		if (status)
		{
			if (runtime_rapid())
			{
				status->set_text("PLUGIN LOADED");
			}
			else
			{
				status->set_text("PLUGIN NOT LOADED");
			}
		}

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
		debugdraw::text(20, yoffset+48, core::str::format("Camera: %2.2f, %2.2f, %2.2f [%2.2f, %2.2f]", camera.get_position().x, camera.get_position().y, camera.get_position().z, camera.get_yaw(), camera.get_pitch()), gemini::Color(1.0f, 1.0f, 1.0f));
		debugdraw::text(20, yoffset + 64, core::str::format("debug bytes/sec: %i\n", tel_viewer.bytes_per_second), gemini::Color(1.0f, 1.0f, 1.0f));

#if DRAW_IMOCAP
		glm::quat local_rotations[IMOCAP_TOTAL_SENSORS];

		// We need to adjust the coordinate frame from the sensor to the engine.
		local_rotations[0] = imocap::device_sensor_local_orientation(mocap_device, 0);
		local_rotations[1] = imocap::device_sensor_local_orientation(mocap_device, 1);
		local_rotations[2] = imocap::device_sensor_local_orientation(mocap_device, 2);

		// It is incorrect to cancel out the parent rotation entirely for children.
		// This results in the child not inheriting the parent's coordinate space.

		// For child rotations, we need to cancel out the offset parent rotation
		// from the local child rotation.

		// temp: setup joint offsets
		joint_offsets[0] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		joint_offsets[1] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));
		joint_offsets[2] = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.254f));


		glm::mat4 world_poses[IMOCAP_TOTAL_SENSORS];

		glm::mat4 to_bind_pose[IMOCAP_TOTAL_SENSORS];
		glm::mat4 inverse_bind_pose[IMOCAP_TOTAL_SENSORS];

		glm::vec3 last_origin;

		glm::mat4 parent_pose;

		imocap::mocap_frame_t mocap_frame;

		if (is_playing_frames)
		{
			memcpy(local_rotations, mocap_frames[current_mocap_frame].poses, sizeof(glm::quat) * IMOCAP_TOTAL_SENSORS);
			current_mocap_frame++;

			if (current_mocap_frame > (mocap_frames.size() - 1))
			{
				is_playing_frames = false;
				LOGV("reached end of playback buffer at %i frames.\n", current_mocap_frame);
			}
		}

		RapidInterface* rapid = runtime_rapid();
		for (size_t index = 0; index < IMOCAP_TOTAL_SENSORS; ++index)
		{
			glm::mat4& world_pose = world_poses[index];
			if (rapid)
			{
				rapid->compute_pose(world_pose, world_poses, local_rotations, joint_offsets, index);
			}

			debugdraw::axes(world_pose, 0.1f);

			glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));
			if (index > 0)
			{
				debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
			}
			last_origin = origin;

			debugdraw::sphere(origin, Color::from_rgba(255, 0, 0, 255), 0.025f);
		}

#endif

#if DRAW_LINES
		// draw all lines
		glm::vec3 last_line = lines[0];
		for (size_t index = 0; index < TOTAL_LINES / 2; index += 2)
		{
			debugdraw::line(last_line, lines[index * 2 + 1], gemini::Color(1.0f, 1.0f, 1.0f));

			assert((index * 2 + 1) < TOTAL_LINES);
			last_line = lines[index * 2 + 1];
		}
#endif

#if DRAW_IMOCAP
		if (is_recording_frames)
		{
			memcpy(mocap_frame.poses, local_rotations, sizeof(glm::quat) * IMOCAP_TOTAL_SENSORS);
			mocap_frame.frame_index = current_mocap_frame;
			current_mocap_frame++;

			mocap_frames.push_back(mocap_frame);
		}
#endif
		//debugdraw::axes(glm::mat4(1.0f), 1.0f);

		platform::window::Frame window_frame = platform::window::get_frame(main_window);

#if 0 // draw obb
		glm::vec3 vertices[] = {
			glm::vec3(-2.0f, 3.0f, 1.0f),
			glm::vec3(-2.5f, 1.25f, 1.0f),
			glm::vec3(2.0f, 3.0f, -1.0f),
			glm::vec3(2.0f, 1.0f, -1.0f)
		};

		OrientedBoundingBox box;
		compute_oriented_bounding_box_by_points(box, vertices, 4);

		//glm::mat3 tr = glm::toMat3(glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
		//debugdraw::oriented_box(tr, glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(2.0f, 1.0f, 1.0f), gemini::Color(1.0f, 0.0f, 0.0f));

		debugdraw::oriented_box(box.rotation, box.center, box.positive_extents, gemini::Color(1.0f, 0.0f, 0.0f));
		debugdraw::axes(glm::mat4(box.rotation), 1.0f, 0.0f);
#endif
		static float the_time = 0.0f;
		render_scene->light_position_world.x = cosf(the_time);
		render_scene->light_position_world.y = 2.0f;
		render_scene->light_position_world.z = sinf(the_time);
		the_time += 0.01f;

		// draw the position of the light
		debugdraw::sphere(render_scene->light_position_world, Color(1.0f, 1.0f, 1.0f), 0.5f, 0.0f);

		debugdraw::update(kernel::parameters().framedelta_seconds);

#if TEST_TELEMETRY_HOST
		TELEMETRY_VARIABLE("light_position_world", render_scene->light_position_world);
		TELEMETRY_VARIABLE("time", the_time);
#endif

		debugdraw::axes(glm::mat4(1.0f), 1.0f);

		if (compositor)
		{
			compositor->tick(static_cast<float>(kernel::parameters().step_interval_seconds));
		}

		surface_modelview = glm::mat4(1.0f);
		surface_projection = glm::ortho(0.0f, window_frame.width, window_frame.height, 0.0f, -1.0f, 1.0f);
		//const glm::vec3& p = camera.get_position();
		//LOGV("p: %2.2f, %2.2f, %2.2f\n", p.x, p.y, p.z);

		modelview_matrix = camera.get_modelview();
		projection_matrix = camera.get_projection();



#if 0
		value = 0.35f;
		render2::Pass render_pass;
		render_pass.target = device->default_render_target();
		render_pass.color(value, value, value, 1.0f);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = false;
		render_pass.depth_write = true;

		render2::CommandQueue* queue = device->create_queue(render_pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		serializer->pipeline(surface_pipeline);
		serializer->vertex_buffer(vertex_buffer);
		serializer->draw(0, 3);
		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
#endif
		platform::window::activate_context(main_window);

		//render_scene_draw(render_scene, device, modelview_matrix, projection_matrix);

		//debugdraw::render(camera.get_modelview(), camera.get_projection(), render_target->width, render_target->height, device->default_render_target());


		if (compositor)
		{
#if TEST_TELEMETRY_HOST
			TELEMETRY_BLOCK(gui_draw);
#endif
			compositor->draw();
		}

		{
#if TEST_TELEMETRY_HOST
			TELEMETRY_BLOCK(device_draw);
#endif
			device->submit();
			platform::window::swap_buffers(main_window);
		}

#if defined(GEMINI_ENABLE_PROFILER)
		gemini::profiler::report();
		gemini::profiler::reset();
#endif

		//		glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000);

		kernel::Parameters& params = kernel::parameters();
		params.current_frame++;

		// calculate delta ticks in milliseconds
		params.framedelta_milliseconds = (current_time - last_time) * MillisecondsPerMicrosecond;

		params.simulation_delta_seconds = (params.framedelta_milliseconds * params.simulation_time_scale) * SecondsPerMillisecond;

		// cache the value in seconds
		params.framedelta_seconds = params.framedelta_milliseconds * SecondsPerMillisecond;
		last_time = current_time;

#if TEST_TELEMETRY_HOST
		telemetry_host_submit_frame();
#endif

		//params.step_alpha = accumulator / params.step_interval_seconds;
		//if (params.step_alpha >= 1.0f)
		//{
		//	params.step_alpha -= 1.0f;
		//}

		//memory_leak_report(false);
	}


	virtual void shutdown()
	{
		single_mesh.purge(default_allocator);

		delete [] lines;
		lines = nullptr;

		imocap::device_destroy(mocap_device);
		mocap_device = nullptr;
		imocap::shutdown();

		directory_monitor_shutdown();

		//notify_client_destroy(&notify_client);
		notify_server_destroy(&notify_server);

		// must be shut down before the animations; as they're referenced.
		transform_graph_destroy_node(render_allocator, transform_graph);
		render_scene_destroy(render_scene, device);
		render_scene_shutdown();

		mesh_animations.clear();


		if (environment.project)
		{
			delete environment.project;
		}

		if (environment.last_project.size() > 0)
		{
			string_destroy(default_allocator, environment.last_project);
		}

#if TEST_TELEMETRY_HOST
		telemetry_host_shutdown();
#endif
		telemetry_viewer_destroy(&tel_viewer);

		animation::shutdown();

		MEMORY2_DELETE(asset_allocator, queued_asset_changes);
		queued_asset_changes = nullptr;

		net_shutdown();
		debugdraw::shutdown();

		// remove the log handler
		core::logging::instance()->remove_handler(&log_handler);

		device->destroy_render_target(render_target);
		device->destroy_texture(texture);

		// compositor will cleanup children
		delete compositor;

		// shutdown the gui
		MEMORY2_DELETE(render_allocator, gui_renderer);

		// explicitly clear the resource cache or else the allocator will
		// detect leaks.
		resource_cache->clear();
		MEMORY2_DELETE(render_allocator, resource_cache);


		assets::shutdown();

		// shutdown the render device
		device->destroy_buffer(vertex_buffer);

		device->destroy_pipeline(pipeline);
		device->destroy_pipeline(surface_pipeline);

		destroy_device(render_allocator, device);

//		glDeleteSync(fence);

		hotloading::shutdown();

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
