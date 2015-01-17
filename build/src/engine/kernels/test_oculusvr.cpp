// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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
#include <stdio.h>

#include <platform/config.h>
#include <slim/xlog.h>

#include "kernel.h"
#include <renderer/renderstream.h>
#include <renderer/renderer.h>
#include <renderer/debugdraw.h>
#include "input.h"
#include "scene_graph.h"
#include "vr.h"
#include "scenelink.h"
#include "camera.h"


using namespace gemini;

using namespace kernel;



class TestOculusVR : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestOculusVR );

	vr::HeadMountedDevice* device;
	scenegraph::Node* root;
	renderer::SceneLink scenelink;
	Camera camera;
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
			if (event.key == input::KEY_ESCAPE)
			{
				kernel::instance()->set_active(false);
			}
			else if (event.key == input::KEY_SPACE)
			{
				if (device)
				{
					device->dismiss_warning();
				}
			}
			else if (event.key == input::KEY_TAB)
			{
				if (device)
				{
					device->reset_head_pose();
				}
			}
        }
	}

	virtual void event(MouseEvent& event)
	{
		switch(event.subtype)
		{
			case kernel::MouseMoved:
			{
				if (input::state()->mouse().is_down(input::MOUSE_LEFT))
				{
					int lastx, lasty;
					input::state()->mouse().last_mouse_position(lastx, lasty);
					
					camera.move_view(event.mx-lastx, event.my-lasty);
				}
                break;
			}
			default: break;
		}
	}

	virtual void event( SystemEvent & event )
	{
	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		// try to setup VR device and resolution here.
		vr::startup();
		
		int32_t w = 800, h = 600;
		LOGV("total VR devices: %i\n", vr::total_devices());

		device = vr::create_device();
		if (device)
		{
			device->query_display_resolution(w, h);
			LOGV("creating window with resolution: %i x %i\n", w, h);
			params.use_fullscreen = true;
		}
		
		params.window_width = (uint16_t)w;
		params.window_height = (uint16_t)h;
		params.window_title = "TestOculusVR";
		params.swap_buffers = 0;
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		if (device)
		{
			vr::setup_rendering(device, params.render_width, params.render_height);
		}
		
		// setup scene
		root = CREATE(scenegraph::Node);
		root->name = "root";

		camera.perspective(100.0f, params.render_width, params.render_height, 0.1f, 32768.0f);
		camera.set_absolute_position(glm::vec3(0, 0, 5));
		
		return kernel::Application_Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		camera.move_left(input::state()->keyboard().is_down(input::KEY_A));
		camera.move_right(input::state()->keyboard().is_down(input::KEY_D));
		camera.move_forward(input::state()->keyboard().is_down(input::KEY_W));
		camera.move_backward(input::state()->keyboard().is_down(input::KEY_S));
		camera.update_view();
		
		root->update(params.step_interval_seconds);
		debugdraw::update(params.step_interval_seconds);
	}

	virtual void tick(kernel::Params& params)
	{
		if (!device)
		{
			return;
		}
		
		renderer::IRenderDriver * driver = renderer::driver();
		device->begin_frame(driver);
		
		renderer::RenderTarget* rt = device->render_target();
		
		RenderStream rs;
		rs.add_clearcolor(0.0f, 0.5f, 0.5f, 1.0f);
		rs.add_clear(renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER);
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.run_commands();
		rs.rewind();
		
		for (uint32_t eye_index = 0; eye_index < device->total_eyes(); ++eye_index)
		{
			vr::EyePose eye_pose;// = device->eye_pose_at(eye_index);

			int x;
			int y = 0;
			int width, height;

			width = rt->width/2;
			height = rt->height;
			
			if (eye_pose.is_left_eye())
			{
				x = 0;
				y = 0;
			}
			else if (eye_pose.is_right_eye())
			{
				x = width;
				y = 0;
			}

			
			glm::vec3 light_position(0, 4, 0);
			glm::vec3 camera_position = camera.pos;
			glm::vec3 render_pos = eye_pose.offset + camera_position + eye_pose.translation;

			camera.set_absolute_position(render_pos);
			camera.update_view();

			renderer::ConstantBuffer cb;
			cb.modelview_matrix = &camera.matCam;
			cb.projection_matrix = &camera.matProj;
			cb.viewer_direction = &camera.view;
			cb.viewer_position = &render_pos;
			cb.light_position = &light_position;
					
			rs.add_viewport(x, y, width, height);
			
			// render nodes
			rs.run_commands();
			scenelink.draw(root, cb);
			
			camera.set_absolute_position(camera_position);
		}

		device->end_frame(driver);
	}

	virtual void shutdown( kernel::Params & params )
	{
		vr::destroy_device(device);
		vr::shutdown();
		
		
		DESTROY(Node, root);
	}
};

IMPLEMENT_APPLICATION( TestOculusVR );