// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone

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
#include "kernel.hpp"
#include <stdio.h>
#include "renderer.hpp"
#include <slim/xlog.h>
#include "input.hpp"
#include "renderstream.hpp"
#include "debugdraw.hpp"

#include "assets/asset_texture.hpp"
using namespace kernel;

#include <nom/nom.hpp>

#include "mathlib.h"

class CustomStyle : public gui::Style
{
	virtual void render_panel( gui::Panel * panel, gui::Compositor * compositor, gui::Renderer * renderer )
	{
		
	} // render_panel
}; // CustomStyle

class GLRenderer : public gui::Renderer
{
	struct VertexType
	{
		glm::vec3 position;
		Color color;
		glm::vec2 uv;
	};
	
public:
	gui::Compositor * compositor;
	renderer::VertexStream vs;

	GLRenderer() {}
	~GLRenderer() {}

	unsigned int vertex_attribs;
	
	virtual void startup( gui::Compositor * compositor )
	{
		vertex_attribs = 0;
		this->compositor = compositor;
		
		vs.desc.add( renderer::VD_FLOAT3 );
		vs.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		vs.desc.add( renderer::VD_FLOAT2 );
		
		vs.create( 64, 64, renderer::DRAW_INDEXED_TRIANGLES );
		
		assets::ShaderString name("uv0");
		vertex_attribs = assets::find_parameter_mask( name );
		
		name = "colors";
		vertex_attribs |= assets::find_parameter_mask( name );
	}
	
	virtual void shutdown( gui::Compositor * compositor )
	{
		
	}
	
	virtual void begin_frame( gui::Compositor * Compositor )
	{
		RenderStream rs;
		
		rs.add_state( renderer::STATE_BLEND, 1 );
		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		rs.add_state(renderer::STATE_DEPTH_TEST, 0);
		rs.add_state(renderer::STATE_DEPTH_WRITE, 0);
		
		rs.run_commands();
	}
	
	virtual void end_frame()
	{
		RenderStream rs;
		
		rs.add_state( renderer::STATE_BLEND, 0 );
		rs.add_state(renderer::STATE_DEPTH_TEST, 1);
		rs.add_state(renderer::STATE_DEPTH_WRITE, 1);
		rs.run_commands();
	}
	
	virtual void draw_bounds( const gui::Bounds & bounds, gui::ColorInt color )
	{
		gui::Size size = bounds.size;
		glm::vec3 start = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
		glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
//		debugdraw::line( start, end, Color( 255, 0, 255 ) );
//		debugdraw::point( glm::vec3( bounds.origin.x + size.width, bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
		
		gui::ColorRGBA rgba;
		UNPACK_RGBA( color, rgba );
				
		debugdraw::box( start, end, Color(rgba[0], rgba[1], rgba[2], rgba[3]), 0.0f );
	}

	void render_buffer2( RenderStream & rs, assets::Shader * shader, assets::Texture * tex )
	{
		vs.update();
		
		glm::mat4 modelview;
		glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
		glm::mat4 object_matrix;
		
		
		rs.add_shader( shader );
		rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview );
		rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection );
		rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
		
		rs.add_sampler2d( shader->get_uniform_location("diffusemap"), 0, tex->texture_id );
		rs.add_draw_call( vs.vertexbuffer );
		
		rs.run_commands();
		vs.reset();
	}

	virtual void draw_textured_bounds( const gui::Bounds & bounds, const gui::TextureHandle & handle )
	{
		vs.reset();
		
		RenderStream rs;
		assets::Texture * tex = assets::textures()->find_with_id( handle );
		if ( !tex )
		{
			return;
		}

		assets::Shader * shader = assets::find_compatible_shader( vertex_attribs + 32 );
		if ( !shader )
		{
			LOGV( "no shader found\n" );
			return;
		}
	
		
		if ( vs.has_room(4, 6) )
		{
			VertexType * v = (VertexType*)vs.request(4);
			
			gui::Size size = bounds.size;
			v[0].position = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
			v[1].position = v[0].position + glm::vec3( 0.0f, size.height, 0.0f );
			v[2].position = v[0].position + glm::vec3( size.width, size.height, 0.0f );
			v[3].position = v[0].position + glm::vec3( size.width, 0.0f, 0.0f );
			
			// lower left corner is the origin in OpenGL
			v[0].uv = glm::vec2(0, 0);
			v[1].uv = glm::vec2(0, 1);
			v[2].uv = glm::vec2(1, 1);
			v[3].uv = glm::vec2(1, 0);
			
			v[0].color = v[1].color = v[2].color = v[3].color = Color(255, 255, 255, 255);
			
			renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
			vs.append_indices( indices, 6 );
		}

		this->render_buffer2( rs, shader, tex );
	} // draw_textured_bounds
		
	virtual gui::TextureResult texture_create( const char * path, gui::TextureHandle & handle )
	{
		assets::Texture * tex = assets::textures()->load_from_path( path );
		if ( !tex )
		{
			return gui::TextureResult_Failed;
		}
	
		handle = tex->Id();
	
		return gui::TextureResult_Success;
	}
	
	virtual void texture_destroy( const gui::TextureHandle & handle )
	{
		// nothing really to do in our system
	}

	virtual gui::TextureResult texture_info( const gui::TextureHandle & handle, uint32_t & width, uint32_t & height, uint8_t & channels )
	{
		assets::Texture * tex = assets::textures()->find_with_id( handle );
		if ( !tex )
		{
			return gui::TextureResult_Failed;
		}
		
		return gui::TextureResult_Success;
	}

}; // GLRenderer

namespace gui
{
	void * default_gui_malloc( size_t bytes )
	{
		return malloc(bytes);
	} // default_gui_malloc
	
	void default_gui_free( void * p )
	{
		free( p );
	} // default_gui_free



	
	void set_custom_allocator( gui_malloc malloc_fn, gui_free free_fn ) {}
	
	gui_malloc _gmalloc = default_gui_malloc;
	gui_free _gfree = default_gui_free;
	
	Compositor * create_compositor( uint16_t width, uint16_t height )
	{
		void * p = (Compositor*)_gmalloc(sizeof(Compositor));
		Compositor * c = new (p) Compositor( width, height );
		return c;
	} // create_compositor
	
	void destroy_compositor( Compositor * c )
	{
		c->~Compositor();
		_gfree( c );
	} // destroy_compositor
	

}; // gui

using namespace gui;


class TestNom : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>
{
public:
	DECLARE_APPLICATION( TestNom );

	CustomStyle style;
	GLRenderer renderer;
	
	gui::Compositor * compositor;
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
			if ( event.key == input::KEY_ESCAPE )
			{
				kernel::instance()->set_active( false );
			}
            fprintf( stdout, "key %i pressed\n", event.key );
        }
        else
        {

            fprintf( stdout, "key %i released\n", event.key );
        }
		
		if ( compositor )
		{
			compositor->key_event( event.key, event.is_down, event.unicode );
		}
		
	}

	virtual void event( MouseEvent & event )
	{
        switch( event.subtype )
        {
            case kernel::MouseMoved:
			{
				if ( compositor )
				{
					compositor->cursor_move_absolute( event.mx, event.my );
				}
                break;
			}
            case kernel::MouseButton:
                if ( event.is_down )
                {
                    fprintf( stdout, "mouse button %i is pressed\n", event.button );
                }
                else
                {
                    fprintf( stdout, "mouse button %i is released\n", event.button );
                }
				
				if ( compositor )
				{
					uint32_t button = 0;
					
					// convert  input button to nom button
					
					compositor->cursor_button( button, event.is_down );
				}
                break;
                
            case kernel::MouseWheelMoved:
                if ( event.wheel_direction > 0 )
                {
                    fprintf( stdout, "mouse wheel toward screen\n" );
                }
                else
                {
                    fprintf( stdout, "mouse wheel away from screen\n" );
                }
                break;
            default:
                fprintf( stdout, "mouse event received!\n" );
                break;
        }

	}

	virtual void event( SystemEvent & event )
	{
		switch( event.subtype )
		{
			case kernel::WindowGainFocus:
				fprintf( stdout, "window gained focus\n" );
				break;
				
			case kernel::WindowLostFocus:
				fprintf( stdout, "window lost focus\n" );
				break;
				
			case kernel::WindowResized:
				fprintf( stdout, "resize event: %i x %i\n", event.window_width, event.window_height );
				break;
				
			default: break;
		}

	}
	
	virtual kernel::ApplicationResult config( kernel::Params & params )
	{
		params.window_width = 800;
		params.window_height = 600;
		params.window_title = "TestNom";
		
		return kernel::Application_Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
		compositor = gui::create_compositor( params.render_width, params.render_height );
		compositor->set_style( &this->style );
		compositor->set_renderer( &this->renderer );

		gui::Button * b = new gui::Button( compositor );
		compositor->add_child(b);
		b->bounds.set( 0, 0, 512, 256 );
		b->set_background_image( compositor, "textures/mainmenu" );
		
		gui::Button * b2 = new gui::Button( compositor );
		compositor->add_child(b2);
		b2->bounds.set( 50, 200, 100, 100 );
		b2->set_background_image( compositor, "textures/checker2" );
		
		debugdraw::startup(128);

		
		return kernel::Application_Success;
	}

	virtual void step( kernel::Params & params )
	{
		if ( compositor )
		{
			compositor->update( params.step_interval_seconds );
		}
		
		debugdraw::update( params.step_interval_seconds );
	}


	virtual void tick( kernel::Params & params )
	{
		RenderStream rs;

		rs.add_clear( renderer::CLEAR_COLOR_BUFFER | renderer::CLEAR_DEPTH_BUFFER );
		rs.add_viewport( 0, 0, params.render_width, params.render_height );
		rs.add_clearcolor( 0.1, 0.1, 0.1, 1.0f );
		rs.run_commands();
		
		if ( compositor )
		{
			compositor->render();
		}
		
		glm::mat4 modelview;
		glm::mat4 projection = glm::ortho( 0.0f, (float)params.render_width, (float)params.render_height, 0.0f, 0.0f, 128.0f );
		
		debugdraw::render( modelview, projection, params.render_width, params.render_height );
	}

	virtual void shutdown( kernel::Params & params )
	{
		gui::destroy_compositor( compositor );
		
		debugdraw::shutdown();
	}
};

IMPLEMENT_APPLICATION( TestNom );