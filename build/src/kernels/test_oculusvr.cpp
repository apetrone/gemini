// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#include <stdio.h>

#include <gemini/config.h>
#include <slim/xlog.h>

#include "kernel.h"
#include "renderer/renderer.h"
#include "input.h"

#include "scene_graph.h"

#include "vr.h"
#include "debugdraw.h"


#include "renderer/renderstream.h"
#include "renderer/scenelink.h"

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