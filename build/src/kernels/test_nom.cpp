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
	
	assets::Material * solid_color;
	assets::Material * texture_map;

	unsigned int vertex_attribs;
	
	void render_buffer( RenderStream & rs, assets::Shader * shader, assets::Material * material )
	{
		vs.update();
		
		glm::mat4 modelview;
		glm::mat4 projection = glm::ortho( 0.0f, (float)compositor->width, (float)compositor->height, 0.0f, -0.1f, 256.0f );
		glm::mat4 object_matrix;
		
		rs.add_shader( shader );
		rs.add_uniform_matrix4( shader->get_uniform_location("modelview_matrix"), &modelview );
		rs.add_uniform_matrix4( shader->get_uniform_location("projection_matrix"), &projection );
		rs.add_uniform_matrix4( shader->get_uniform_location("object_matrix"), &object_matrix );
		
		rs.add_material( material, shader );
		
		rs.add_draw_call( vs.vertexbuffer );
		
		rs.run_commands();
		vs.reset();
	}
	
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
		
		
		// setup materials
		solid_color = assets::materials()->allocate_asset();
		if ( solid_color )
		{
			solid_color->allocate_parameters(1);
			assets::Material::Parameter * parameter = &solid_color->parameters[0];
			parameter->type = assets::MP_VEC4;
			parameter->name = "diffusecolor";
			parameter->vecValue = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			assets::materials()->take_ownership( "gui/solid_color", solid_color );
			solid_color->calculate_requirements();
		}

		texture_map = assets::materials()->allocate_asset();
		if ( texture_map )
		{
			texture_map->allocate_parameters(1);
			assets::Material::Parameter * parameter = &texture_map->parameters[0];
			parameter->type = assets::MP_SAMPLER_2D;
			parameter->name = "diffusemap";
			parameter->intValue = assets::textures()->get_default()->Id();
			assets::materials()->take_ownership( "gui/texture_map", texture_map );
			texture_map->calculate_requirements();
		}
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
//		gui::Size size = bounds.size;
//		glm::vec3 start = glm::vec3( bounds.origin.x, bounds.origin.y, 0.0f );
//		glm::vec3 end = start + glm::vec3( size.width, size.height, 0.0f );
//		debugdraw::line( start, end, Color( 255, 0, 255 ) );
//		debugdraw::point( glm::vec3( bounds.origin.x + size.width, bounds.origin.y + size.height, 0.0f ), Color(255, 255, 255) );
		
		gui::ColorRGBA rgba;
		UNPACK_RGBA( color, rgba );
		
		solid_color->parameters[0].vecValue = glm::vec4( (rgba[0] / 255.0f), (rgba[1] / 255.0f), (rgba[2] / 255.0f), (rgba[3] / 255.0f) );
//		debugdraw::box( start, end, Color(rgba[0], rgba[1], rgba[2], rgba[3]), 0.0f );
		
		vs.reset();
		
		RenderStream rs;
		assets::Shader * shader = assets::find_compatible_shader( vertex_attribs + solid_color->requirements );
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

			//v[0].color = v[1].color = v[2].color = v[3].color = Color(rgba[0], rgba[1], rgba[2], rgba[3]);
			
			renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
			vs.append_indices( indices, 6 );
		}
		else
		{
			LOGV( "buffer be full\n" );
		}
		
		this->render_buffer( rs, shader, solid_color );
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

		texture_map->parameters[0].intValue = handle;
		texture_map->parameters[0].texture_unit = 0;
		
		assets::Shader * shader = assets::find_compatible_shader( vertex_attribs + texture_map->requirements );
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

		this->render_buffer( rs, shader, texture_map );
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


struct CustomControl : public gui::Panel
{
	gui::Bounds local_bounds;
	
	CustomControl( Panel * parent ) : Panel(parent) {}
	
	float snap_to( float input, float nearest )
	{
		float hnear = (nearest/2.0f);
		return floor((input + hnear) / nearest) * nearest;
	}
	
	virtual void handle_event( EventArgs & args )
	{
		if ( args.type == gui::Event_CursorMove )
		{
//			LOGV( "cursor: %i %i\n", args.cursor.x, args.cursor.y );
//			LOGV( "delta: %i %i\n", args.delta.x, args.delta.y );
//			LOGV( "local: %i %i\n", args.local.x, args.local.y );
//			
			local_bounds.size.width = snap_to( args.local.x, 16.0f );
			local_bounds.size.height = snap_to( args.local.y, 16.0f );
			
//			LOGV( "size: %g %g\n", local_bounds.size.width, local_bounds.size.height );
		}
//		LOGV( "handle_event: %i\n", args.type );
		

		//Panel::handle_event( args );
	}
	
	virtual void render( Compositor * compositor, Renderer * renderer )
	{
		ColorInt color = PACK_RGBA(0, 255, 0, 255);
		
		local_bounds.origin = this->bounds.origin;
		
		

		if ( this->background != 0 )
		{
			renderer->draw_textured_bounds( this->bounds, this->background );
		}
		
		renderer->draw_bounds( local_bounds, color );
		
	}
};



struct Timeline : public gui::Panel
{
	Timeline( Panel * parent ) : Panel(parent) {}
	
	virtual void handle_event( EventArgs & args )
	{
		if ( args.type == gui::Event_CursorMove )
		{
		}
		
		//Panel::handle_event( args );
	}
	
	virtual void render( Compositor * compositor, Renderer * renderer )
	{
		ColorInt color = PACK_RGBA(255, 0, 255, 255);
		
		renderer->draw_bounds( this->bounds, color );

//		if ( this->background != 0 )
//		{
//			renderer->draw_textured_bounds( this->bounds, this->background );
//		}
	}
};

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

//		gui::Button * b = new gui::Button( compositor );
//		compositor->add_child(b);
//		b->bounds.set( 0, 0, 512, 256 );
//		b->set_background_image( compositor, "textures/mainmenu" );
		
		CustomControl * b2 = new CustomControl( compositor );
		compositor->add_child(b2);
		b2->bounds.set( 50, 200, 128, 128 );
		b2->set_background_image( compositor, "textures/checker2" );
		
		
		Timeline * t = new Timeline( compositor );
		compositor->add_child( t );
		t->bounds.set( 50, 350, 600, 128 );
		
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