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


#include "configloader.hpp"
#include "camera.hpp"


glm::mat4 objectMatrix;
glm::vec3 light_position = glm::vec3( 0, 2, 0 );

const unsigned int MAX_RENDERER_STREAM_BYTES = 32768;
const unsigned int MAX_RENDERER_STREAM_COMMANDS = 32768;
unsigned int num_commands = 0;
struct RenderState
{
	int type;
	long offset; // offset into parameters
}; // RenderState

struct RenderStream
{
	char buffer[ MAX_RENDERER_STREAM_BYTES ];
	RenderState commands[ MAX_RENDERER_STREAM_COMMANDS ];
	unsigned int num_commands;
	MemoryStream stream;
	
	
	RenderStream()
	{
		num_commands = 0;
		stream.init( buffer, MAX_RENDERER_STREAM_BYTES );
	}
	
	void rewind()
	{
		stream.rewind();
		num_commands = 0;
	}
	
	RenderState * new_render_state()
	{
		if ( num_commands >= (MAX_RENDERER_STREAM_COMMANDS) )
		{
			LOGW( "Too many renderstates! make this dynamically resizable!\n" );
			return 0;
		}
		
		RenderState * rs;
		rs = &commands[ num_commands++ ];
		return rs;
	}
	
	void run_commands()
	{
		stream.rewind();
		RenderState * renderstate;
		renderer::IRenderDriver * driver = renderer::driver();

		for( int state_id = 0; state_id < num_commands; state_id++ )
		{
			renderstate = &commands[ state_id ];
			
			// setup the stream and run the command
			stream.seek( renderstate->offset, 1 );
			driver->run_command( (renderer::DriverCommandType)renderstate->type, stream );
		}
#if 0
		for( int state_id = 0; state_id < num_commands; state_id++ )
		{
			renderstate = &commands[ state_id ];
			
			// run the post (cleanup) command
			stream.seek( renderstate->offset, 1 );
			driver->post_command( (renderer::DriverCommandType)renderstate->type, stream );
		}
#endif

	} // run_commands
	
	void add_command( int type )
	{
		RenderState * state = new_render_state();
		state->type = type;
		state->offset = stream.offset_pointer();
	}
	
	void add_clearcolor( float r, float g, float b, float a )
	{
		add_command( renderer::DC_CLEARCOLOR );
		stream.write( r );
		stream.write( g );
		stream.write( b );
		stream.write( a );
	}
	
	void add_clear( unsigned int bitflags )
	{
		add_command( renderer::DC_CLEAR );
		stream.write( bitflags );
	}
	
	void add_viewport( int x, int y, int width, int height )
	{
		add_command( renderer::DC_VIEWPORT );
		stream.write( x );
		stream.write( y );
		stream.write( width );
		stream.write( height );
	}
	
	void add_uniform1i( int uniform_location, int value )
	{
		add_command( renderer::DC_UNIFORM1i );
		stream.write( uniform_location );
		stream.write( value );
	}
	
	void add_sampler2d( int uniform_location, int texture_unit, int texture_id )
	{
		add_command( renderer::DC_UNIFORM_SAMPLER_2D );
		stream.write( uniform_location );
		stream.write( texture_unit );
		stream.write( texture_id );
	}
	
	void add_state( renderer::DriverState state, int enable )
	{
		add_command( renderer::DC_STATE );
		stream.write( state );
		stream.write( enable );
	}
	
	void add_blendfunc( renderer::RenderBlendType source, renderer::RenderBlendType destination )
	{
		add_command( renderer::DC_BLENDFUNC );
		stream.write( source );
		stream.write( destination );
	}
	
	void add_shader( renderer::ShaderProgram * shader )
	{
		add_command( renderer::DC_SHADER );
		stream.write( shader->object );
	}	
	
	void add_uniform3f( int location, const glm::vec3 * data )
	{
		add_command( renderer::DC_UNIFORM3f );
		stream.write( location );
		stream.write( data );
	}
	
	void add_uniform_matrix4( int location, const glm::mat4 * data )
	{
		add_command( renderer::DC_UNIFORMMATRIX4 );
		stream.write( data );
		stream.write( location );
	}	
	
	void add_draw_call( renderer::VertexBuffer * vertexbuffer )
	{
		assert( vertexbuffer != 0 );
		add_command( renderer::DC_DRAWCALL );
		renderer::driver()->setup_drawcall( vertexbuffer, this->stream );
	}
}; // RenderStream






const float TEST_SIZE = 10.0f;

renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines )
{
	renderer::ShaderObject shader_object;
	char * buffer;
	int length = 0;
	buffer = fs::file_to_buffer( shader_path, 0, &length );
	if ( buffer )
	{
		StackString<32> version;
		util::strip_shader_version( buffer, version );
		if ( version._length == 0 )
		{
			LOGW( "Unable to extract version from shader! Forcing to #version 150.\n" );
			version = "#version 150";
		}
		version.append( "\n" );
				
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
	assets::Mesh * mesh;
	
	float alpha;
	int alpha_delta;
	Camera camera;
	
	struct GeneralParameters
	{
		glm::mat4 * modelview_matrix;
		glm::mat4 * projection_project;
		glm::mat4 * object_matrix;
		
		unsigned int global_params;
		glm::vec3 * camera_position;
	};
	
	void stream_geometry( RenderStream & rs, assets::Geometry * geo, GeneralParameters & gp )
	{
		assert( geo != 0 );
		assets::Material * material = assets::material_by_id( geo->material_id );
		assert( material != 0 );
//		LOGV( "material: %i\n", material->Id() );
		assets::Shader * shader = assets::find_compatible_shader( geo->attributes + material->requirements + (1 << gp.global_params) );
		assert( shader != 0 );

		if ( !shader )
		{
			LOGE( "Unable to find shader!\n" );
			return;
		}
		
//		LOGV( "binding shader: %i\n", shader->id );
		rs.add_shader( shader );
		
		if ( gp.global_params > 0 )
		{
			rs.add_uniform3f( shader->get_uniform_location("lightPosition"), &light_position );
			rs.add_uniform3f( shader->get_uniform_location("cameraPosition"), gp.camera_position );
		}
		
		rs.add_uniform_matrix4( shader->get_uniform_location("modelviewMatrix"), gp.modelview_matrix );
		rs.add_uniform_matrix4( shader->get_uniform_location("projectionMatrix"), gp.projection_project );
		rs.add_uniform_matrix4( shader->get_uniform_location("objectMatrix"), gp.object_matrix );
				
		// setup shader parameters
		assets::Material::Parameter * parameter;
		for( int p = 0; p < material->num_parameters; ++p )
		{
			parameter = &material->parameters[ p ];
			int renderstate = assets::material_parameter_type_to_render_state( parameter->type );
			int uniform_location = shader->get_uniform_location( parameter->name() );

			// this needs to be converted to a table of function pointers...
			if ( renderstate == renderer::DC_UNIFORM1i )
			{
				rs.add_uniform1i( uniform_location, parameter->intValue );
			}
			else if ( renderstate == renderer::DC_UNIFORM3f || renderstate == renderer::DC_UNIFORM4f )
			{
				rs.add_uniform3f( uniform_location, (glm::vec3*)&parameter->vecValue );
			}
			else if ( renderstate == renderer::DC_UNIFORM_SAMPLER_2D )
			{
				rs.add_sampler2d( uniform_location, parameter->texture_unit, parameter->intValue );
			}
			else if ( renderstate == renderer::DC_UNIFORM_SAMPLER_CUBE )
			{
				// ...
			}

		}
		
		rs.add_draw_call( geo->vertexbuffer );
	} // stream_geometry
	
	
	
	RenderStream rs;
	assets::Geometry geo;
	
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
			{
				if ( input::state()->mouse().is_down( input::MOUSE_LEFT ) )
				{
					int lastx, lasty;
					input::state()->mouse().last_mouse_position( lastx, lasty );
					
					camera.move_view( event.mx-lastx, event.my-lasty );
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

		camera.perspective( 60, params.window_width, params.window_height, 0.1f, 512.0f );
		camera.set_absolute_position( glm::vec3( 0, 1, 5 ) );
		
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
		
		vb.create(512, 512, renderer::DRAW_INDEXED_TRIANGLES );

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

#if 1
		geo.vertex_count = 4;
		geo.index_count = 6;
		geo.vertices = CREATE_ARRAY( glm::vec3, 4 );
		geo.indices = CREATE_ARRAY( renderer::IndexType, 6 );
		geo.colors = CREATE_ARRAY( Color, 4 );
		geo.uvs = CREATE_ARRAY( renderer::UV, 4 );
		
		for( size_t i = 0; i < geo.vertex_count; i++)
		{
			FontVertexType * vert = (FontVertexType*)vb[i];
			glm::vec3 & pos = geo.vertices[ i ];
			pos.x = vert->x;
			pos.y = vert->y;
			pos.z = vert->z;
			
			Color & color = geo.colors[ i ];
			color = vert->color;
			
			renderer::UV & uv = geo.uvs[ i ];
			uv.u = vert->u;
			uv.v = vert->v;
		}
		memcpy( geo.indices, indices, sizeof(renderer::IndexType) * geo.index_count );


		geo.render_setup();
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

#if 0
		// test material loading
		assets::Material * material = assets::load_material( "materials/barrel" );
		if ( material )
		{
			LOGV( "loaded material '%s'\n", material->name() );
		}
#endif

		// test mesh loading
		mesh = assets::load_mesh( "models/plasma3" );
		if ( mesh )
		{
			LOGV( "loaded mesh '%s'\n", mesh->path() );
			mesh->prepare_geometry();
		}
		else
		{
			LOGW( "unable to load mesh.\n" );
		}

		return kernel::Success;
	}

	
	virtual void step( kernel::Params & params )
	{
		if ( input::state()->keyboard().is_down( input::KEY_W ) )
		{
			camera.move_forward( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_S ) )
		{
			camera.move_backward( params.step_interval_seconds );
		}
		
		if ( input::state()->keyboard().is_down( input::KEY_A ) )
		{
			camera.move_left( params.step_interval_seconds );
		}
		else if ( input::state()->keyboard().is_down( input::KEY_D ) )
		{
			camera.move_right( params.step_interval_seconds );
		}
	
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
		
		light_position.y = 2 + sinf( alpha ) * 8;
		
#if 1
		// update geometry (this still needs to be added in the command queue)
		for( int i = 0; i < 4; ++i )
		{
			FontVertexType * vert = (FontVertexType*)vb[i];
			vert->color.a = alpha * 255.0;
		}
		vb.update();
#endif

	}

	virtual void tick( kernel::Params & params )
	{
		rs.rewind();
		rs.add_clearcolor( 0.25, 0.25, 0.25, 1.0f );
		rs.add_clear( 0x00004000 | 0x00000100 );
		rs.add_viewport( 0, 0, (int)params.window_width, (int)params.window_height );
		

		
		


		rs.add_state( renderer::STATE_DEPTH_TEST, 1 );

//		rs.add_state( renderer::STATE_BLEND, 1 );
//		rs.add_blendfunc( renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA );
	
		GeneralParameters gp;
		gp.global_params = 3;
		gp.camera_position = &camera.pos;
		gp.modelview_matrix = &camera.matCam;
		gp.projection_project = &camera.matProj;
		gp.object_matrix = &objectMatrix;
		if ( 1 && mesh )
		{
			for( unsigned int geo_id = 0; geo_id < mesh->total_geometry; ++geo_id )
			{
				
				assets::Geometry * g = &mesh->geometry[ geo_id ];
				stream_geometry( rs, g, gp );
			}
		}
		else
		{
			rs.add_shader( &shader_program );
			glm::mat4 modelview;
			modelview = glm::translate( modelview, glm::vec3( (params.window_width/2.0f)-(TEST_SIZE/2.0f), (params.window_height/2.0f)-(TEST_SIZE/2.0f), 0 ) );
			glm::mat4 projection = glm::ortho( 0.0f, (float)params.window_width, (float)params.window_height, 0.0f, -0.5f, 255.0f );
			rs.add_uniform_matrix4( 0, &camera.matCam );
			rs.add_uniform_matrix4( 4, &camera.matProj );
			rs.add_sampler2d( 8, 0, tex->texture_id );
//			rs.add_draw_call( vb.vertexbuffer );
//			stream_geometry( rs, &geo );
			rs.add_draw_call( geo.vertexbuffer );
		}
		
		rs.run_commands();
	}

	virtual void shutdown( kernel::Params & params )
	{
		renderer::driver()->shaderprogram_destroy( shader_program );
		
		vb.destroy();
	}
};

IMPLEMENT_APPLICATION( TestUniversal );