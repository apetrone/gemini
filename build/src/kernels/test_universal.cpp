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


#include "assets.hpp"


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


	audio::SoundHandle sound;
	audio::SoundSource source;
	renderer::VertexBuffer * vertex_buffer;
	renderer::ShaderProgram shader_program;
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

		setup_menu();

		foreach_child( _menu.current_menu(), print_option );
		_menu.navigate_to_child( 0 );
		foreach_child( _menu.current_menu(), print_option );
		
		
		_menu.navigate_back();
		foreach_child( _menu.current_menu(), print_option );
				
		_menu.clear_items();
		
#if 0
		assets::Texture * tex = assets::load_texture( "textures/logo" );
		if ( tex )
		{
			LOGV( "loaded texture successfully: %i!\n", tex->texture_id );
		}
		else
		{
			LOGW( "Could not load texture.\n" );
		}
#endif
		
		
		
#if 0
		renderer::VertexStream buffer;
		buffer.desc.add( renderer::VD_FLOAT3 );
		buffer.desc.add( renderer::VD_UNSIGNED_BYTE4 );
		
		struct FontVertexType
		{
			float x, y, z;
			Color color;
		};
		
		buffer.create(sizeof(FontVertexType), 1024, 1024, renderer::DRAW_TRIANGLES );
		buffer.destroy();
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



		renderer::ShaderObject vertex_shader = create_shader_from_file( "shaders/fontshader.vert", renderer::SHADER_VERTEX, 0 );
		renderer::ShaderObject fragment_shader = create_shader_from_file( "shaders/fontshader.frag", renderer::SHADER_FRAGMENT, 0 );

		renderer::ShaderParameters parms;
		this->shader_program = renderer::driver()->shaderprogram_create( parms );
		renderer::driver()->shaderprogram_attach( shader_program, vertex_shader );
		renderer::driver()->shaderprogram_attach( shader_program, fragment_shader );

		parms.set_frag_data_location( "out_Color" );
		parms.alloc_uniforms( 2 );
		parms.uniforms[0].set_key( "projectionMatrix" );
		parms.uniforms[1].set_key( "modelviewMatrix" );
		
		parms.alloc_attributes( 3 );
		parms.attributes[0].set_key( "in_Position" ); parms.attributes[0].second = 0;
		parms.attributes[1].set_key( "in_Color" ); parms.attributes[1].second = 1;
		parms.attributes[2].set_key( "in_tex" ); parms.attributes[2].second = 2;
		

		renderer::driver()->shaderprogram_bind_attributes( shader_program, parms );
		renderer::driver()->shaderprogram_link_and_validate( shader_program );
		
		renderer::driver()->shaderprogram_bind_uniforms( shader_program, parms );

		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );

		return kernel::Success;
	}
	
	virtual void step( kernel::Params & params )
	{
	}

	virtual void tick( kernel::Params & params )
	{
#if 1
		renderer::IRenderDriver * driver = renderer::driver();
		MemoryStream ms;
		char buffer[128] = {0};
		ms.init( buffer, 128 );
		
		// viewport
		ms.rewind();
		ms.write( 0 );
		ms.write( 0 );
		ms.write( params.window_width );
		ms.write( params.window_width );
		ms.rewind();
		driver->run_command( renderer::DC_VIEWPORT, ms );
		
		// set clear color
		ms.rewind();
		ms.write( 0.5f );
		ms.write( 0.0f );
		ms.write( 0.75f );
		ms.write( 1.0f );
		ms.rewind();
		driver->run_command( renderer::DC_CLEARCOLOR, ms );
		
		// color_buffer_bit
		ms.rewind();
		ms.write( 0x00004000 );
		ms.rewind();
		driver->run_command( renderer::DC_CLEAR, ms );
		
		
		driver->shaderprogram_activate( shader_program );
#endif
	}

	virtual void shutdown()
	{
		renderer::driver()->shaderprogram_destroy( shader_program );
	}
};

IMPLEMENT_APPLICATION( TestUniversal );