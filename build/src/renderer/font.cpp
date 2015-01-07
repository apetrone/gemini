// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone

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
#include <stdlib.h>

#include <core/typedefs.h>
#include <core/filesystem.h>

#include <slim/xstr.h>
#include <slim/xlog.h>

#include "font.h"
#include "renderer.h"
#include "renderstream.h"

#define FONT_MAX_VERTICES 1024
#define FONS_VERTEX_COUNT FONT_MAX_VERTICES
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"



namespace font
{
	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		struct FONScontext* _font_context;
		renderer::ShaderProgram * _shader;
				
		struct FontVertex
		{
			float position[2];
			float uv[2];
			unsigned char color[4];
		};
		
		struct FontData
		{
			renderer::Texture* texture;
			int render_width;
			int render_height;
			
			FontData() : texture(nullptr), render_width(0), render_height(0) {}
		};

		FontData* _font_data = nullptr;

		uint32_t _uniforms[3];


		int font_create(void* userdata, int width, int height)
		{
			// @description this may be called multiple times, so delete existing textures
			// create a texture with the given width and height.
			// @returns 1 on success, 0 on failure
			
			FontData* data = static_cast<FontData*>(userdata);
			assert(data != nullptr);
			
			renderer::IRenderDriver * driver = renderer::driver();
			
			image::Image image;
			image.channels = 1;
			image.flags |= image::F_ALPHA;
			image.width = width;
			image.height = height;
			image.pixels = 0;
			
			// handle multiple calls of this function
			assert(data->texture == nullptr);
			data->texture = driver->texture_create(image);
			
			return 1;
		}
		
		int font_resize(void* userdata, int width, int height)
		{
			// @description resize the texture;
			// explicit expand or resetting of the atlas texture.
			// @returns 1 on success, 0 on failure
			return font_create(userdata, width, height);
		}
		
		void font_update(void* userdata, int* rect, const unsigned char* pixels)
		{
			// @description update the texture
			renderer::IRenderDriver * driver = renderer::driver();
			FontData* data = static_cast<FontData*>(userdata);
			
			int width = (rect[2] - rect[0]);
			int height = (rect[3] - rect[1]);
			
			image::Image image;
			image.flags |= image::F_ALPHA;
			image.pixels = (unsigned char*)pixels;

			gemini::Recti area(rect[0], rect[1], width, height);
			
			driver->texture_update(data->texture, image, area);
		}
		
		void font_draw(void* userdata, const float* verts, const float* tcoords, const unsigned int* colors, int vertex_count)
		{
			// @description called when the font should be drawn
			renderer::VertexStream & vs = internal::_vertexstream;
			FontData* data = static_cast<FontData*>(userdata);
			
			if ( !vs.has_room( vertex_count, 0 ) )
			{
				LOGE( "Unable to draw font: vertexstream has no room! (%i)\n", vs.total_vertices );
			}
			else
			{
				FontVertex* vertex = (FontVertex*)vs.request(vertex_count);

				// pack the data
				for (int i = 0; i < vertex_count; ++i)
				{
					memcpy(&vertex[i].position, &verts[i*2], sizeof(float)*2);
					memcpy(&vertex[i].uv, &tcoords[i*2], sizeof(float)*2);
					memcpy(&vertex[i].color, &colors[i], sizeof(unsigned int));
				}
				internal::_vertexstream.update();
				
				glm::mat4 modelview_matrix = glm::mat4(1.0f);
				glm::mat4 projection_matrix;
				
				float w = 0; float h = 0;
				w = (real)data->render_width;
				h = (real)data->render_height;
				projection_matrix = glm::ortho(0.0f, w, 0.0f, h, -1.0f, 1.0f);
				
				RenderStream rs;
				
				// activate shader
				rs.add_shader( internal::_shader );
				
				// setup uniforms
				rs.add_uniform_matrix4(internal::_uniforms[0], &modelview_matrix );
				rs.add_uniform_matrix4(internal::_uniforms[1], &projection_matrix );
				rs.add_sampler2d(internal::_uniforms[2], 0, data->texture);
				
				// add draw call for vertexbuffer
				rs.add_draw_call( internal::_vertexstream.vertexbuffer );
				
				// run all commands
				rs.run_commands();
				
				vs.reset();
			}
		}
		
		void font_delete(void* userdata)
		{
			// @description called when the renderer should be deleted
			FontData* data = static_cast<FontData*>(userdata);
			renderer::IRenderDriver * driver = renderer::driver();
			driver->texture_destroy(data->texture);
		}
	}; // namespace internal
	
	void startup(renderer::ShaderProgram* fontshader, int render_width, int render_height)
	{
		// initialize the vertex stream
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
			
		internal::_vertexstream.create( FONT_MAX_VERTICES, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );
		

		internal::_font_data = CREATE(internal::FontData);
		set_viewport_size(render_width, render_height);
		
		
		// setup font stash parameters
		FONSparams params;
		memset(&params, 0, sizeof(FONSparams));
		
		params.width = 256;
		params.height = 256;
		params.flags = FONS_ZERO_BOTTOMLEFT;
		params.renderCreate = internal::font_create;
		params.renderResize = internal::font_resize;
		params.renderUpdate = internal::font_update;
		params.renderDraw = internal::font_draw;
		params.renderDelete = internal::font_delete;
		params.userPtr = internal::_font_data;
		
		internal::_font_context = fonsCreateInternal(&params);
		assert(internal::_font_context != 0);

		internal::_shader = fontshader;
		assert(internal::_shader != 0);
		
		internal::_uniforms[0] = internal::_shader->get_uniform_location("modelview_matrix");
		internal::_uniforms[1] = internal::_shader->get_uniform_location("projection_matrix");
		internal::_uniforms[2] = internal::_shader->get_uniform_location("diffusemap");
	} // startup

	void shutdown()
	{
		// cleanup used memory here
		internal::_vertexstream.destroy();
		
		if (internal::_font_context)
		{
			// delete internal font stash context and data
			fonsDeleteInternal(internal::_font_context);
			internal::_font_context = 0;
			
			DESTROY(FontData, internal::_font_data);
		}
	} // shutdown
	

	
	void draw_string(const renderer::Font& font, int x, int y, const char* utf8, const Color& color)
	{
		if (font.is_valid())
		{
			RenderStream rs;
			
			// setup global rendering state
			rs.add_state(renderer::STATE_DEPTH_TEST, 0);
			rs.add_state(renderer::STATE_BLEND, 1);
			rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
			rs.run_commands();
			
			// draw
			fonsSetColor(internal::_font_context, RGBAToUInt(color.r, color.g, color.b, color.a));
			fonsDrawText(internal::_font_context, x, y, utf8, 0);
			
			// restore state
			rs.rewind();
			rs.add_state( renderer::STATE_BLEND, 0 );
			rs.add_state( renderer::STATE_DEPTH_TEST, 1 );
			rs.run_commands();
		}
	} // draw_string
	
	
	void dimensions_for_text(const renderer::Font& font, const char* utf8, float& minx, float& miny, float& maxx, float& maxy)
	{
		if (font.is_valid())
		{
			float bounds[4];
			fonsTextBounds(internal::_font_context, 0, 0, utf8, 0, bounds);
			
			minx = bounds[0];
			miny = bounds[1];
			maxx = bounds[2];
			maxy = bounds[3];
		}
	} // dimensions_for_text
	
	void set_viewport_size(int render_width, int render_height)
	{
		internal::_font_data->render_width = render_width;
		internal::_font_data->render_height = render_height;
	}
	
	unsigned int measure_height(const renderer::Font& font, const char * utf8 )
	{
		if (!font.is_valid())
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxy-miny;
	} // measure_height
	
	unsigned int measure_width(const renderer::Font& font, const char* utf8 )
	{
		if (!font.is_valid())
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxx-minx;
	} // measure_width
	
	renderer::Font load_font_from_memory(const void* data, unsigned int data_size, unsigned short point_size)
	{
		renderer::Font font;
		font.handle = fonsAddFontMem(internal::_font_context, "font", (unsigned char*)data, data_size, 0);
		if (!font.is_valid())
		{
			LOGE( "Unable to load font from memory!\n" );
			font.handle = -1;
		}
		
		// setup the font
		fonsSetFont(internal::_font_context, font.handle);
		fonsSetSize(internal::_font_context, point_size);

		return font;
	} // load_font_from_memory
}; // namespace font
