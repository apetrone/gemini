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
#include "typedefs.h"
#include "log.h"
#include "renderer.hpp"
#include "factory.hpp"
#include "xstr.h"



// compile-time selection of these classes starts here.

#if PLATFORM_IS_RASPBERRYPI || (PLATFORM_IOS && PLATFORM_IS_MOBILE) || PLATFORM_USE_GLES2
	// force use of OpenGL ES v2
	#include <gldrivers/opengl_glesv2.hpp>
	#define RENDERER_TYPE 1

#else
	// use OpenGL
//	#include <gldrivers/opengl_legacy21.hpp>
	#include <gldrivers/opengl_core32.hpp>
	#define RENDERER_TYPE 0
#endif

namespace renderer
{
	IRenderDriver * _render_driver = 0;
	
	IRenderDriver * driver() { return _render_driver; }
	
	int startup( DriverType driver_type )
	{
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
		if ( _render_driver )
		{
			DESTROY(IRenderDriver, _render_driver);
		}
	} // shutdown
	
}; // namespace renderer


namespace renderer
{
	
	// VertexTypeDescriptor
	unsigned int VertexDescriptor::size[ VD_TOTAL ] =
	{
		sizeof(float) * 2,
		sizeof(float) * 3,
		sizeof(float) * 4,
		sizeof(unsigned char) * 3,
		sizeof(unsigned char) * 4,
		sizeof(unsigned int)
	};
	
	unsigned int VertexDescriptor::elements[ VD_TOTAL ] =
	{
		2,
		3,
		4,
		3,
		4,
		1
	};
	
	VertexDescriptor::VertexDescriptor()
	{
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
			if ( descriptor == VD_FLOAT2 )
			{
				attribSize = (sizeof(float)*2);
			}
			else if ( descriptor == VD_FLOAT3 )
			{
				attribSize = (sizeof(float)*3);
			}
			else if ( descriptor == VD_UNSIGNED_INT )
			{
				attribSize = sizeof(unsigned int);
			}
			else if ( descriptor == VD_UNSIGNED_BYTE3 )
			{
				attribSize = sizeof(unsigned char) * 3;
			}
			else if ( descriptor == VD_UNSIGNED_BYTE4 )
			{
				attribSize = sizeof(unsigned char) * 4;
			}
			
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

	ShaderKeyValuePair::ShaderKeyValuePair()
	{
		this->first = 0;
		this->second = 0;
	}
	
	ShaderKeyValuePair::~ShaderKeyValuePair()
	{
		if ( this->first )
		{
			DEALLOC( this->first );
		}
	}
	
	void ShaderKeyValuePair::set_key(const char *key)
	{
		if (this->first != 0)
		{
			DEALLOC(this->first);
		}

		size_t length = xstr_len(key);
		this->first = (char*)ALLOC( length + 1 );
		memset( this->first, 0, length+1 );
		xstr_ncpy( this->first, key, length );
	} // set_key

	ShaderParameters::ShaderParameters()
	{
		this->total_attributes = 0;
		this->total_uniforms = 0;
		this->uniforms = 0;
		this->attributes = 0;
		this->frag_data_location = 0;
	}
	
	ShaderParameters::~ShaderParameters()
	{
		if ( this->uniforms )
		{
			DESTROY_ARRAY(ShaderKeyValuePair, this->uniforms, this->total_uniforms);
		}
		
		if ( this->attributes )
		{
			DESTROY_ARRAY(ShaderKeyValuePair, this->attributes, this->total_attributes);
		}
		
		if ( this->frag_data_location )
		{
			DEALLOC(frag_data_location);
		}
	}
	
	void ShaderParameters::alloc_attributes( unsigned int attributes_count )
	{
		this->total_attributes = attributes_count;
		assert( this->attributes == 0 );
		this->attributes = CREATE_ARRAY( ShaderKeyValuePair, attributes_count );
		for( unsigned int i = 0; i < attributes_count; ++i )
		{
			this->attributes[i].first = 0;
		}
	} // alloc_attributes
	
	void ShaderParameters::alloc_uniforms( unsigned int uniform_count )
	{
		this->total_uniforms = uniform_count;
		assert( this->uniforms == 0 );
		this->uniforms = CREATE_ARRAY( ShaderKeyValuePair, uniform_count );
		for( unsigned int i = 0; i < uniform_count; ++i )
		{
			this->uniforms[i].first = 0;
		}
	} // alloc_uniforms
	
	void ShaderParameters::set_frag_data_location( const char * location )
	{
		size_t len = xstr_len(location);
		this->frag_data_location = (char*)ALLOC( len+1 );
		memset( this->frag_data_location, 0, len+1 );
		xstr_ncpy( this->frag_data_location, location, len );
	} // set_frag_data_location
}; // namespace renderer