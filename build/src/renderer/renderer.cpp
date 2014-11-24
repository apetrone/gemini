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
#include "renderer.h"
#include "shaderconfig.h"

#include <platform/typedefs.h>
#include <core/factory.h>

#include <slim/xstr.h>
#include <slim/xlog.h>

#include <core/configloader.h>

#include "gldrivers/opengl_common.h"

// compile-time selection of these classes starts here.

#if PLATFORM_USE_GLES2
	// force use of OpenGL ES v2
	#include "gldrivers/opengl_glesv2.h"
	#define RENDERER_TYPE 1
#elif PLATFORM_USE_GLES3
	#error Not yet implemented.
	#include "gldrivers/opengl_glesv3.h"
	#define RENDER_TYPE 2
#else
	// use OpenGL
	#include "gldrivers/opengl_core32.h"
	#define RENDERER_TYPE 0
#endif

namespace renderer
{
	IRenderDriver * _render_driver = 0;
	
	IRenderDriver * driver() { return _render_driver; }

	renderer::ShaderProgram* create_shaderprogram_from_file(const char* path)
	{
		return shader_config::load_shaderprogram_from_file(path);
	}

	int startup( DriverType driver_type, const RenderSettings& settings )
	{
		// setup vertex descriptor
		VertexDescriptor::startup();
	
		shader_config::startup();
	
		typedef Factory<IRenderDriver, 4> RendererFactory;
		RendererFactory factory;

		// run-time selection of the renderer has to happen here based on hints by the kernel
		// for now, we just hard-code these
#if RENDERER_TYPE > 0
		factory.register_class( GLESv2::creator, "OpenGL ES 2.0", GLESv2 );
		driver_type = GLESv2;
#else
		factory.register_class( &GLCore32::creator, "OpenGL3.2", OpenGL );
		driver_type = OpenGL;
#endif
		
		// choose the correct driver at run time; based on some hints?
		RendererFactory::Record * record = factory.find_class( 0, driver_type );
		if ( record )
		{
			_render_driver = record->creator();
			if ( _render_driver )
			{
				LOGV( "Initialized renderer: '%s'\n", _render_driver->description() );

				// init render driver settings
				_render_driver->init_with_settings(settings);
				
				return 1;
			}
		}
		else
		{
			LOGE( "Unable to find a renderer matching driver type: %i\n", driver_type );
		}

		return 0;
	} // startup
	
	void shutdown()
	{
		shader_config::shutdown();
	
		if ( _render_driver )
		{
			DESTROY(IRenderDriver, _render_driver);
		}
	} // shutdown
	
}; // namespace renderer


namespace renderer
{	
	int ShaderProgram::get_uniform_location( const char * name )
	{
//		LOGV( "# uniforms: %i\n", total_uniforms );
		for( int i = 0; i < uniforms.size(); ++i )
		{
			if (std::string(name) == uniforms[i].first)
			{
//				LOGV( "uniform: %s, at %i\n", name, uniforms[i].second );
				return uniforms[i].second;
			}
		}
		
		LOGW( "No uniform named %s (%i)\n", name, -1 );
		return -1;
	} // get_uniform_location
	
	void ShaderProgram::show_uniforms()
	{
		LOGV("uniforms:\n");
		for (size_t i = 0; i < uniforms.size(); ++i)
		{
			LOGV("%s -> %i\n", uniforms[i].first.c_str(), uniforms[i].second);
		}
	}
	
	void ShaderProgram::show_attributes()
	{
		LOGV("attributes:\n");
		for (size_t i = 0; i < attributes.size(); ++i)
		{
			LOGV("%s -> %i\n", attributes[i].first.c_str(), attributes[i].second);
		}
	}
}

namespace renderer
{
	
	// VertexTypeDescriptor
	uint16_t VertexDescriptor::size[ VD_TOTAL ] = {0};
	uint16_t VertexDescriptor::elements[ VD_TOTAL ] = {0};
	
	void VertexDescriptor::startup()
	{
		// clear table
		memset(VertexDescriptor::size, 0, sizeof(uint16_t)*VD_TOTAL);
		memset(VertexDescriptor::elements, 0, sizeof(uint16_t)*VD_TOTAL);
		
		// populate table with vertex descriptor types
		map_type(VD_FLOAT2, sizeof(float), 2);
		map_type(VD_FLOAT3, sizeof(float), 3);
		map_type(VD_FLOAT4, sizeof(float), 4);
		
		map_type(VD_INT4, sizeof(int), 4);
		
		map_type(VD_UNSIGNED_BYTE3, sizeof(unsigned char), 3);
		map_type(VD_UNSIGNED_BYTE4, sizeof(unsigned char), 4);
		
		map_type(VD_UNSIGNED_INT, sizeof(unsigned int), 1);
		
		// validate table
		for (size_t i = 0; i < VD_TOTAL; ++i)
		{
			assert(VertexDescriptor::size[i] != 0);
			assert(VertexDescriptor::elements[i] != 0);
		}
	}
	
	void VertexDescriptor::map_type(uint32_t type, uint16_t size, uint16_t elements)
	{
		VertexDescriptor::size[type] = size*elements;
		VertexDescriptor::elements[type] = elements;
	}
	
	VertexDescriptor::VertexDescriptor()
	{
		id = 0;
		reset();
		memset( description, 0, sizeof(VertexDescriptorType) * MAX_DESCRIPTORS );
	}
	
	void VertexDescriptor::add( VertexDescriptorType desc )
	{
		description[ id ] = desc;
		++id;
		
		if ( id >= MAX_DESCRIPTORS-1 )
		{
			printf( "Reached MAX_DESCRIPTORS. Resetting\n" );
			id = 0;
		}
		
		attribs = id;
	} // add
	
	VertexDescriptorType VertexDescriptor::get( int i )
	{
		return description[ i ];
	} // get
	
	void VertexDescriptor::reset()
	{
		if ( id > 0 )
		{
			attribs = id;
		}
		id = 0;
	} // reset
	
	unsigned int VertexDescriptor::calculate_vertex_stride()
	{
		unsigned int size = 0;
		unsigned int attribSize = 0;
		unsigned int descriptor;
		
		for( unsigned int i = 0; i < attribs; ++i )
		{
			descriptor = description[i];
			attribSize = VertexDescriptor::size[descriptor];
						
			size += attribSize;
		}
		
		return size;
	} // calculate_vertex_stride

	const VertexDescriptor & VertexDescriptor::operator= ( const VertexDescriptor & other )
	{
		this->attribs = other.attribs;
		this->id = other.id;
		
		for( unsigned int i = 0; i < VD_TOTAL; ++i )
		{
			this->size[i] = other.size[i];
			this->elements[i] = other.elements[i];
		}
		
		for( unsigned int id = 0; id < MAX_DESCRIPTORS; ++id )
		{
			this->description[id] = other.description[id];
		}
	
		return *this;
	} // operator=

}; // namespace renderer