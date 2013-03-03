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
#include "typedefs.h"
#include "kernel.hpp"
#include <stdio.h>
#include "renderer.hpp"
#include "audio.hpp"
#include "input.hpp"
#include "log.h"
//#include <squirrel.h>
#include "filesystem.hpp"
#include "hashtable.hpp"

#include "game/menu.hpp"

#include "mathlib.h"
#include "assets.hpp"

const float TEST_SIZE = 200.0f;

// strip the version line from shader source
void strip_shader_version( char * buffer, StackString<32> & version );


void strip_shader_version( char * buffer, StackString<32> & version )
{
	// remove preceding "#version" shader
	char * pos = xstr_str( buffer, "#version" );
	if ( pos )
	{
		char * end = pos;
		while( *end != '\n' )
			++end;
		
		version._length = (end-pos);
		memcpy( &version[0], &buffer[(pos-buffer)], version._length );
		memset( &buffer[(pos-buffer)], ' ', (end-pos) );
	}
} // strip_shader_version




renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines )
{
	renderer::ShaderObject shader_object;
	char * buffer;
	int length = 0;
	buffer = fs::file_to_buffer( shader_path, 0, &length );
	if ( buffer )
	{
		StackString<32> version;
		strip_shader_version( buffer, version );
		if ( version._length == 0 )
		{
			LOGW( "Unable to extract version from shader! Forcing to #version 150.\n" );
			version = "#version 150";
		}
				
		// specify version string first, followed by any defines, then the actual shader source
		if ( preprocessor_defines == 0 )
		{
			preprocessor_defines = "";
		}
		
		shader_object = renderer::driver()->shaderobject_create( type );
		
		renderer::driver()->shaderobject_compile( shader_object, buffer, preprocessor_defines, version() );
	
		DEALLOC(buffer);
	}
	else
	{
		LOGE( "Unable to open shader '%s'\n", shader_path );
	}
	
	return shader_object;
}






void foreach_child( MenuItem * root, foreach_menu_callback callback )
{
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		callback( option );
	}
}

MenuNavigator _menu;

void setup_menu()
{
	MenuItem * root = _menu.root_menu();
	root->name = "Game";

	MenuItem * newgame = root->add_child( "New Game" );
		newgame->add_child( "Easy" );
		newgame->add_child( "Medium" );
		newgame->add_child( "Hard" );
		
	root->add_child( "Load Game" );
	root->add_child( "Options" );
	root->add_child( "Quit" );
}

void print_option( MenuItem * child )
{
	LOGV( "[ %s ]\n", child->name );
}


void print_options( MenuItem * root )
{
	LOGV( "[ %s ]\n", root->name );
	LOGV( "options:\n" );
	
	MenuItemVector::iterator it, end;
	it = root->children.begin();
	end = root->children.end();
	
	for( ; it != end; ++it )
	{
		MenuItem * option = (*it);
		LOGV( "-> %s\n", option->name );
	}
}


using namespace kernel;



class TestUniversal : public kernel::IApplication,
	public IEventListener<KeyboardEvent>,
	public IEventListener<MouseEvent>,
	public IEventListener<SystemEvent>,
	public IEventListener<TouchEvent>
{
	struct FontVertexType
	{
		float x, y, z;
		Color color;
		float u, v;
	};

	audio::SoundHandle sound;
	audio::SoundSource source;
	renderer::ShaderProgram shader_program;
	renderer::VertexStream vb;
	assets::Texture * tex;
	
	float alpha;
	int alpha_delta;
	
public:
	DECLARE_APPLICATION( TestUniversal );
	
	virtual void event( kernel::TouchEvent & event )
	{
		if ( event.subtype == kernel::TouchBegin )
		{
			fprintf( stdout, "Touch Event Began at %i, %i\n", event.x, event.y );
		}
		else if ( event.subtype == kernel::TouchMoved )
		{
			fprintf( stdout, "Touch Event Moved at %i, %i\n", event.x, event.y );
		}
		else if ( event.subtype == kernel::TouchEnd )
		{
			fprintf( stdout, "Touch Event Ended at %i, %i\n", event.x, event.y );
		}
	}
	
	virtual void event( KeyboardEvent & event )
	{
        if ( event.is_down )
        {
            fprintf( stdout, "key %i pressed\n", event.key );
			 
			if ( event.key == input::KEY_Q )
			{
//				audio::stop( source );
			}
        }
        else
        {
            fprintf( stdout, "key %i released\n", event.key );
        }
	}

	virtual void event( MouseEvent & event )
	{
        switch( event.subtype )
        {
            case kernel::MouseMoved:
                break;
            case kernel::MouseButton:
                if ( event.is_down )
                {
                    fprintf( stdout, "mouse button %i is pressed\n", event.button );
                }
                else
                {
                    fprintf( stdout, "mouse button %i is released\n", event.button );
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
		params.window_title = "TestUniversal";
//		HSQUIRRELVM vm = sq_open(1024);
//		sq_close( vm );
		return kernel::Success;
	}

	virtual kernel::ApplicationResult startup( kernel::Params & params )
	{
//		sound = audio::create_sound( "sounds/powerup" );
//		source = audio::play( sound );
#if 0
		setup_menu();

		foreach_child( _menu.current_menu(), print_option );
		_menu.navigate_to_child( 0 );
		foreach_child( _menu.current_menu(), print_option );
		
		
		_menu.navigate_back();
		foreach_child( _menu.current_menu(), print_option );
				
		_menu.clear_items();
#endif
#if 1
		tex = assets::load_texture( "textures/default" );
		if ( tex )
		{
			LOGV( "loaded texture successfully: %i!\n", tex->texture_id );
		}
		else
		{
			LOGW( "Could not load texture.\n" );
		}
#endif

		
		alpha = 0;
		alpha_delta = 1;
		
		renderer::ShaderParameters parms;
		this->shader_program = renderer::driver()->shaderprogram_create( parms );
		
		renderer::ShaderObject vertex_shader = create_shader_from_file( "shaders/fontshader.vert", renderer::SHADER_VERTEX, 0 );
		renderer::ShaderObject fragment_shader = create_shader_from_file( "shaders/fontshader.frag", renderer::SHADER_FRAGMENT, 0 );
		
		renderer::driver()->shaderprogram_attach( shader_program, vertex_shader );
		renderer::driver()->shaderprogram_attach( shader_program, fragment_shader );
		
		parms.set_frag_data_location( "out_color" );
		parms.alloc_uniforms( 3 );
		parms.uniforms[0].set_key( "projection_matrix" );
		parms.uniforms[1].set_key( "modelview_matrix" );
		parms.uniforms[2].set_key( "diffusemap" );

		parms.alloc_attributes( 3 );
		parms.attributes[0].set_key( "in_cosition" ); parms.attributes[0].second = 0;
		parms.attributes[1].set_key( "in_color" ); parms.attributes[1].second = 1;
		parms.attributes[2].set_key( "in_uv" ); parms.attributes[2].second = 2;
		
		
		renderer::driver()->shaderprogram_bind_attributes( shader_program, parms );
		renderer::driver()->shaderprogram_link_and_validate( shader_program );
		
		renderer::driver()->shaderprogram_activate( shader_program );
		renderer::driver()->shaderprogram_bind_uniforms( shader_program, parms );
		
		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );
		
#if 1
		vb.reset();
		vb.desc.add( renderer::VD_FLOAT3 );
		vb.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		vb.desc.add( renderer::VD_FLOAT2 );
		

		
		vb.create(sizeof(FontVertexType), 512, 512, renderer::DRAW_TRIANGLES );

		FontVertexType * v = (FontVertexType*)vb.request( 4 );
		if ( v )
		{
			FontVertexType * vert = &v[0];
			vert->x = 0;
			vert->y = 0;
			vert->z = 0;
			vert->color.set( 255, 255, 255 );
			vert->u = 0;
			vert->v = 0;
			
			vert = &v[1];
			vert->x = 0;
			vert->y = TEST_SIZE;
			vert->z = 0;
			vert->color.set( 255, 255, 255 );
			vert->u = 0;
			vert->v = 1;
			
			vert = &v[2];
			vert->x = TEST_SIZE;
			vert->y = TEST_SIZE;
			vert->z = 0;
			vert->color.set( 255, 255, 255 );
			vert->u = 1;
			vert->v = 1;

			vert = &v[3];
			vert->x = TEST_SIZE;
			vert->y = 0;
			vert->z = 0;
			vert->color.set( 255, 255, 255 );
			vert->u = 1;
			vert->v = 0;

		}
		
		renderer::IndexType indices[] = { 0, 1, 2, 2, 3, 0 };
		vb.append_indices( indices, 6 );

		vb.update();
#endif


#if 0
		HashTable<int> t;
		
		t.set( "hello", 3 );
		t.set( "poopy", 32 );
		t.set( "mario", 16 );
		t.set( "daft", 8 );
		t.set( "punk", 88 );
		t.set( "luigi", 13 );
		t.set( "something/heregoes/nothing", 11 );
		t.set( "ipad", 122 );
		
		t.set( "something/heregoes/nothing2", 131 );
		
		if ( t.contains("hello") )
		{
			LOGV( "t contains 'hello'!\n" );
			LOGV( "'hello' value is: %i\n", t.get("hello") );
		}
		else
		{
			LOGV( "t does not contain 'hello'\n" );
		}
#endif




		return kernel::Success;
	}
	
	virtual void step( kernel::Params & params )
	{
		alpha += ((float)alpha_delta) * 0.025f;
		
		if ( alpha >= 1.0 || alpha <= 0 )
		{
			if ( alpha >= 1.0 )
			{
				alpha = 1.0f;
			}
			else
			{
				alpha = 0;
			}
			alpha_delta = -alpha_delta;
		}
	}

	virtual void tick( kernel::Params & params )
	{
#if 1
		renderer::IRenderDriver * driver = renderer::driver();
		MemoryStream ms;
		char buffer[512] = {0};
		ms.init( buffer, 512 );
		


		// set clear color
		ms.rewind();
		ms.write( 0.25f );
		ms.write( 0.25f );
		ms.write( 0.25f );
		ms.write( 1.0f );
		ms.rewind();
		driver->run_command( renderer::DC_CLEARCOLOR, ms );
	
		// color_buffer_bit
		ms.rewind();
		ms.write( 0x00004000 );
		ms.rewind();
		driver->run_command( renderer::DC_CLEAR, ms );
					
		// viewport
		ms.rewind();
		ms.write( 0 );
		ms.write( 0 );
		ms.write( (int)params.window_width );
		ms.write( (int)params.window_height );
		ms.rewind();
		driver->run_command( renderer::DC_VIEWPORT, ms );
			
		glm::mat4 modelview;
		modelview = glm::translate( modelview, glm::vec3( (params.window_width/2.0f)-(TEST_SIZE/2.0f), (params.window_height/2.0f)-(TEST_SIZE/2.0f), 0 ) );
		glm::mat4 projection = glm::ortho( 0.0f, (float)params.window_width, (float)params.window_height, 0.0f, -0.5f, 255.0f );
		
		ms.rewind();
		
		// activate the shader
		ms.write( shader_program.object );
		
		// set up uniforms
		ms.write( &modelview );
		ms.write( 0 );
		
		ms.write( &projection );
		ms.write( 4 );
		
		ms.write( 0 );
		ms.write( tex->texture_id );
		ms.write( 8 );
		
		
		ms.write( renderer::STATE_BLEND );
		ms.write( 1 );
		
		ms.write( renderer::BLEND_SRC_ALPHA );
		ms.write( renderer::BLEND_ONE_MINUS_SRC_ALPHA );
		
		ms.rewind();
		driver->run_command( renderer::DC_SHADER, ms );
		driver->run_command( renderer::DC_UNIFORMMATRIX4, ms );
		driver->run_command( renderer::DC_UNIFORMMATRIX4, ms );
		driver->run_command( renderer::DC_UNIFORM_SAMPLER_2D, ms );
		driver->run_command( renderer::DC_STATE, ms );
		driver->run_command( renderer::DC_BLENDFUNC, ms );

		for( int i = 0; i < 4; ++i )
		{
			FontVertexType * vert = (FontVertexType*)vb[i];
			vert->color.a = alpha * 255.0;
		}

		vb.update();
		
		
		
		
		
		vb.draw_elements();
//		vb.reset();
		driver->shaderprogram_deactivate( shader_program );
#endif
	}

	virtual void shutdown( kernel::Params & params )
	{
		renderer::driver()->shaderprogram_destroy( shader_program );
		
		vb.destroy();
	}
};

IMPLEMENT_APPLICATION( TestUniversal );