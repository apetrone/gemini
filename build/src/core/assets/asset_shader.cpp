// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

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
#include <gemini/util/stackstring.h>

#include "assets.h"
#include "renderer.h"
#include "assets/asset_shader.h"
#include "filesystem.h"

namespace assets
{
	// -------------------------------------------------------------
	// Shader
	
	int Shader::get_uniform_location( const char * name )
	{
		//		LOGV( "# uniforms: %i\n", total_uniforms );
		for( int i = 0; i < total_uniforms; ++i )
		{
			
			if ( xstr_nicmp( uniforms[i].first, name, xstr_len(uniforms[i].first) ) == 0 )
			{
				//				LOGV( "uniform: %s, at %i\n", name, uniforms[i].second );
				return uniforms[i].second;
			}
		}
		
		LOGW( "No uniform named %s (%i)\n", name, id );
		return -1;
	} // get_uniform_location
	
	void Shader::release() {}
	
	ShaderPermutationGroup::ShaderPermutationGroup()
	{
		num_defines = 0;
		num_attributes = 0;
		num_uniforms = 0;
		num_requires = 0;
		num_conflicts = 0;
	}
	
	ShaderPermutationGroup::~ShaderPermutationGroup()
	{
		if ( num_defines )
		{
			DESTROY_ARRAY( ShaderString, defines, num_defines );
			num_defines = 0;
		}
		
		if ( num_attributes )
		{
			DESTROY_ARRAY( ShaderString, attributes, num_attributes );
			num_attributes = 0;
		}
		
		if ( num_uniforms )
		{
			DESTROY_ARRAY( ShaderString, uniforms, num_uniforms );
			num_uniforms = 0;
		}
		
		if ( num_requires )
		{
			DESTROY_ARRAY( ShaderString, requires, num_requires );
			num_requires = 0;
		}
		
		if ( num_conflicts )
		{
			DESTROY_ARRAY( ShaderString, conflicts, num_conflicts );
			num_conflicts = 0;
		}
	}
	
	
	ShaderPermutations::ShaderPermutations()
	{
		options = 0;
		attributes = 0;
		num_attributes = 0;
		uniforms = 0;
		num_uniforms = 0;
		num_permutations = 0;
		num_parameters = 0;
	}
	
	ShaderPermutations::~ShaderPermutations()
	{
		if ( num_permutations )
		{
			if ( options )
			{
				DEALLOC(options);
			}
			
			DESTROY_ARRAY(ShaderPermutationGroup, attributes, num_attributes );
			DESTROY_ARRAY(ShaderPermutationGroup, uniforms, num_uniforms );
		}
	} // ~ShaderPermutations
	
	renderer::ShaderObject create_shader_from_file( const char * shader_path, renderer::ShaderObjectType type, const char * preprocessor_defines )
	{
		renderer::ShaderObject shader_object;
		char * buffer;
		size_t length = 0;
		buffer = fs::file_to_buffer( shader_path, 0, &length );
		if ( buffer )
		{
			StackString<32> version;
			util::strip_shader_version( buffer, version );
			if ( version._length == 0 )
			{
#if defined(PLATFORM_IS_MOBILE) || defined(PLATFORM_IS_RASPBERRYPI)
				version = "#version 100";
#else
				version = "#version 150";
#endif
				LOGW( "Unable to extract version from shader! Forcing to '%s'.\n", version() );
				
			}
			version.append( "\n" );
			
			// specify version string first, followed by any defines, then the actual shader source
			if ( preprocessor_defines == 0 )
			{
				preprocessor_defines = "";
			}
			
			shader_object = renderer::driver()->shaderobject_create( type );
			
			if ( !renderer::driver()->shaderobject_compile( shader_object, buffer, preprocessor_defines, version()) )
			{
				LOGE( "Error compiling shader %s\n", shader_path );
			}
			
			DEALLOC(buffer);
		}
		else
		{
			LOGE( "Unable to open shader '%s'\n", shader_path );
		}
		
		return shader_object;
	} // create_shader_from_file
	
	void load_shader( const char * shader_path, Shader * shader )
	{
		LOGV( "loading shader '%s'\n", shader_path );
		StackString<MAX_PATH_SIZE> filename = shader_path;
		renderer::ShaderParameters params;
		
		renderer::ShaderProgram shader_program;
		shader_program.object = 0;
		if (!renderer::driver())
		{
			LOGW( "Renderer is not initialized!\n" );
			return;
		}
		renderer::driver()->shaderprogram_deactivate( shader_program );
		
		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
		shader->object = program.object;
		
		filename.append( ".vert" );
		renderer::ShaderObject vertex_shader = create_shader_from_file( filename(), renderer::SHADER_VERTEX, 0 );
		
		filename = shader_path;
		filename.append( ".frag" );
		renderer::ShaderObject fragment_shader = create_shader_from_file( filename(), renderer::SHADER_FRAGMENT, 0 );
		
		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
		
		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
		if ( renderer::driver()->shaderprogram_link_and_validate( *shader, *shader ) )
		{
			renderer::driver()->shaderprogram_activate( *shader );
			renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
			renderer::driver()->shaderprogram_deactivate( *shader );
		}
		
		renderer::driver()->shaderprogram_detach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_detach( *shader, fragment_shader );
		
		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );
	} // load_shader
	
	void destroy_shader( Shader * shader )
	{
		renderer::ShaderProgram program;
		program.object = shader->object;
		renderer::driver()->shaderprogram_destroy( program );
	} // destroy_shader
	
	void load_test_shader( Shader * shader )
	{
		renderer::ShaderParameters params;
		
		renderer::ShaderProgram program = renderer::driver()->shaderprogram_create( params );
		shader->object = program.object;
		
		renderer::ShaderObject vertex_shader = create_shader_from_file( "shaders/fontshader.vert", renderer::SHADER_VERTEX, 0 );
		renderer::ShaderObject fragment_shader = create_shader_from_file( "shaders/fontshader.frag", renderer::SHADER_FRAGMENT, 0 );
		
		renderer::driver()->shaderprogram_attach( *shader, vertex_shader );
		renderer::driver()->shaderprogram_attach( *shader, fragment_shader );
		
		
		shader->set_frag_data_location( "out_color" );
		shader->alloc_uniforms( 3 );
		shader->uniforms[0].set_key( "projection_matrix" );
		shader->uniforms[1].set_key( "modelview_matrix" );
		shader->uniforms[2].set_key( "diffusemap" );
		
		shader->alloc_attributes( 3 );
		shader->attributes[0].set_key( "in_position" ); shader->attributes[0].second = 0;
		shader->attributes[1].set_key( "in_color" ); shader->attributes[1].second = 1;
		shader->attributes[2].set_key( "in_uv" ); shader->attributes[2].second = 2;
		
		
		renderer::driver()->shaderprogram_bind_attributes( *shader, *shader );
		renderer::driver()->shaderprogram_link_and_validate( *shader, *shader );
		
		renderer::driver()->shaderprogram_bind_uniforms( *shader, *shader );
		
		renderer::driver()->shaderobject_destroy( vertex_shader );
		renderer::driver()->shaderobject_destroy( fragment_shader );
		
	} // load_test_shader
}; // namespace assets