// -------------------------------------------------------------
// Copyright (C) 2004- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include "font.h"
#include "renderer.h"
#include "renderstream.h"

#define FONT_MAX_VERTICES 1024
#define FONS_VERTEX_COUNT FONT_MAX_VERTICES
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"

#include <core/typedefs.h>
#include <core/str.h>

#include <runtime/filesystem.h>
#include <runtime/logging.h>


#include <stdlib.h>

using namespace renderer;

namespace font
{
	namespace internal
	{	
		renderer::VertexStream _vertexstream;
		renderer::ShaderProgram * _shader;

		int16_t render_width;
		int16_t render_height;
				
		struct FontVertex
		{
			float position[2];
			float uv[2];
			unsigned char color[4];
		};

		struct FontData : public renderer::Font
		{
			renderer::Texture* texture;
			struct FONScontext* context;
			int16_t font_height;
			
			float line_height;
			
			FontData() : texture(0), context(0), line_height(0.0f) {}
		};
		
		
		typedef std::vector<FontData*> FontDataVector;
		FontDataVector _font_data;

		uint32_t _uniforms[3];


		int font_create(void* userdata, int width, int height)
		{
			// @brief this may be called multiple times, so delete existing textures
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
			// @brief resize the texture;
			// explicit expand or resetting of the atlas texture.
			// @returns 1 on success, 0 on failure
			return font_create(userdata, width, height);
		}
		
		void font_update(void* userdata, int* rect, const unsigned char* pixels)
		{
			// @brief update the texture
			renderer::IRenderDriver * driver = renderer::driver();
			FontData* data = static_cast<FontData*>(userdata);
			
			int width = (rect[2] - rect[0]);
			int height = (rect[3] - rect[1]);
			
			image::Image image;
			image.flags |= image::F_ALPHA;
			image.pixels = (unsigned char*)pixels;

			mathlib::Recti area(rect[0], rect[1], width, height);
			
			driver->texture_update(data->texture, image, area);
			image.pixels = 0;
		}
		
		void font_draw(void* userdata, const float* verts, const float* tcoords, const unsigned int* colors, int vertex_count)
		{
			// @brief called when the font should be drawn
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
				w = internal::render_width;
				h = internal::render_height;
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
			// @brief called when the renderer should be deleted
			FontData* data = static_cast<FontData*>(userdata);
			renderer::IRenderDriver * driver = renderer::driver();
			driver->texture_destroy(data->texture);
		}
	} // namespace internal
	
	void startup(renderer::ShaderProgram* fontshader, int render_width, int render_height)
	{
		// initialize the vertex stream
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_FLOAT2 );
		internal::_vertexstream.desc.add( renderer::VD_UNSIGNED_BYTE4 );
			
		internal::_vertexstream.create( FONT_MAX_VERTICES, 0, renderer::DRAW_TRIANGLES, renderer::BUFFER_STREAM );

		set_viewport_size(render_width, render_height);
		
		// setup the font shader
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
		
		for (internal::FontData* font : internal::_font_data)
		{
			// delete internal font stash context and data
			fonsDeleteInternal(font->context);
			MEMORY_DELETE(font, core::memory::global_allocator());
		}
	} // shutdown
	

	
	void draw_string(Handle handle, int x, int y, const char* utf8, const core::Color& color)
	{
		internal::FontData* font = internal::_font_data[handle];
		
		// adjust for the baseline
		y = internal::render_height - y - font->line_height;
	

		if (font->is_valid())
		{
			RenderStream rs;
			
			// setup global rendering state
			rs.add_state(renderer::STATE_DEPTH_TEST, 0);
			rs.add_state(renderer::STATE_BLEND, 1);
			rs.add_blendfunc(renderer::BLEND_SRC_ALPHA, renderer::BLEND_ONE_MINUS_SRC_ALPHA);
			rs.run_commands();
			
			
			// draw
			fonsSetColor(font->context, color.as_uint32());
			fonsDrawText(font->context, x, y, utf8, 0);
			
			// restore state
			rs.rewind();
			rs.add_state(renderer::STATE_BLEND, 0);
			rs.add_state(renderer::STATE_DEPTH_TEST, 1);
			rs.run_commands();
		}
	} // draw_string
	
	
	void dimensions_for_text(internal::FontData* font, const char* utf8, float& minx, float& miny, float& maxx, float& maxy)
	{
		if (font->is_valid())
		{
			float bounds[4];
			fonsTextBounds(font->context, 0, 0, utf8, 0, bounds);
			
			minx = bounds[0];
			miny = bounds[1];
			maxx = bounds[2];
			maxy = bounds[3];
		}
	} // dimensions_for_text
	
	void set_viewport_size(int render_width, int render_height)
	{
		internal::render_width = render_width;
		internal::render_height = render_height;
	}
	
	unsigned int measure_height(Handle handle, const char * utf8 )
	{
		internal::FontData* font = internal::_font_data[handle];
		if (!font)
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxy-miny;
	} // measure_height
	
	unsigned int measure_width(Handle handle, const char* utf8 )
	{
		internal::FontData* font = internal::_font_data[handle];
		if (!font)
		{
			return 0;
		}
		
		float minx = 0, miny = 0, maxx = 0, maxy = 0;
		dimensions_for_text(font, utf8, minx, miny, maxx, maxy);
		return maxx-minx;
	} // measure_width
	
	Handle load_font_from_memory(const void* data, unsigned int data_size, unsigned short point_size)
	{
		internal::FontData* font = MEMORY_NEW(internal::FontData, core::memory::global_allocator());
		
		Handle handle = internal::_font_data.size();
		internal::_font_data.push_back(font);
					
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
		params.userPtr = font;
		
		font->context = fonsCreateInternal(&params);
		assert(font->context != 0);
		
		
		font->handle = fonsAddFontMem(font->context, "font", (unsigned char*)data, data_size, 0);
		if (!font->is_valid())
		{
			LOGE( "Unable to load font from memory!\n" );
			font->handle = -1;
		}
		
		// setup the font
		fonsSetFont(font->context, font->handle);
		fonsSetSize(font->context, point_size);

		// read and cache metrics
		float ascender = 0;
		float descender = 0;
		fonsVertMetrics(font->context, &ascender, &descender, &font->line_height);

		return handle;
	} // load_font_from_memory
} // namespace font


#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

namespace render2
{
	namespace font
	{
		struct Glyph
		{
			uint32_t codepoint;
			stbrp_rect rect;
		};

		const size_t FONT_INITIAL_RECT_TOTAL = 256;
		const size_t FONT_ATLAS_RESOLUTION = 256;

		// internal data
		struct FontData
		{
			Type type;
			unsigned int point_size;
			FT_Face face;
			void* data;
			size_t data_size;
			render2::Texture* texture;
			stbrp_context rp_context;
			stbrp_node rp_nodes[FONT_INITIAL_RECT_TOTAL];

			// array of rects
			Array<stbrp_rect> rp_rects;

			int has_kerning;
			int is_fixed_width;
			float line_height;
			int border;

			FontData() :
				type(FONT_TYPE_INVALID),
				point_size(0),
				face(nullptr),
				data(nullptr),
				data_size(0),
				texture(nullptr),
				has_kerning(0),
				is_fixed_width(0),
				line_height(0),
				border(0)
			{
			}

			stbrp_rect* get_glyph(int codepoint)
			{
				for (size_t index = 0; index < rp_rects.size(); ++index)
				{
					stbrp_rect& r = rp_rects[index];
					if (codepoint == r.id)
					{
						return &r;
					}
				}

				return nullptr;
			}
		};


		namespace detail
		{
			FT_Library _ftlibrary;
			Array<FontData*> _fonts(0);
			render2::Device* _device = nullptr;
		} // namespace detail

		bool Handle::is_valid() const
		{
			size_t reference = static_cast<size_t>(ref);
			return (detail::_fonts.size() > reference) && (ref != -1);
		}




		// ---------------------------------------------------------------------
		// implementation
		// ---------------------------------------------------------------------
		struct GlyphData
		{
			int advancex;
			int advancey;
			int width;
			int height;
			int hbearingx;
			int hbearingy;
			int vbearingx;
			int vbearingy;
			FT_UInt index;

			GlyphData()
			{
				memset(this, 0, sizeof(GlyphData));
			}
		};

		void get_gylph_info(FT_Face face, int codepoint, GlyphData& glyphdata)
		{
			FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
			FT_Error error = FT_Err_Ok;

			// glyph doesn't exist in the font; need to display blank rect?
			// invalid glyphs will have an index of 0
			if (glyph_index == 0)
			{
				LOGW("Invalid glyph %i -> %c\n", codepoint, codepoint);
				return;
			}

			error = FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
			assert(error == FT_Err_Ok);
			if (error)
			{
				LOGW("Error while loading character %i, glyph: %i\n", codepoint, glyph_index);
				return;
			}

			glyphdata.index = glyph_index;

			// these are expressed in 26.6 units; hence the division by 64.
			glyphdata.advancex = face->glyph->advance.x >> 6;
			glyphdata.advancey = face->glyph->advance.y >> 6;

			int hadvance = face->glyph->metrics.horiAdvance >> 6;
			assert(glyphdata.advancex == hadvance);

			if (FT_HAS_HORIZONTAL(face))
			{
				glyphdata.hbearingx = face->glyph->metrics.horiBearingX >> 6;
				glyphdata.hbearingy = face->glyph->metrics.horiBearingY >> 6;
			}

			glyphdata.width = face->glyph->metrics.width >> 6;
			glyphdata.height = face->glyph->metrics.height >> 6;

			assert((int)glyphdata.width == face->glyph->bitmap.width);
			assert((int)glyphdata.height == face->glyph->bitmap.rows);

			if (FT_HAS_VERTICAL(face))
			{
				glyphdata.vbearingx = face->glyph->metrics.vertBearingX >> 6;
				glyphdata.vbearingy = face->glyph->metrics.vertBearingY >> 6;
			}
		}

		void startup(render2::Device* device)
		{
			FT_Error error = FT_Init_FreeType(&detail::_ftlibrary);
			if (error)
			{
				LOGE("Could not initialize FreeType library!\n");
			}

			FT_Int major, minor, patch;
			FT_Library_Version(detail::_ftlibrary, &major, &minor, &patch);
			LOGV("initialized FreeType %i.%i.%i\n", major, minor, patch);

			detail::_device = device;
		}

		void shutdown()
		{
			for (auto& data : detail::_fonts)
			{
				FT_Done_Face(data->face);

				MEMORY_DEALLOC(data->data, core::memory::global_allocator());

				if (data->texture)
				{
					detail::_device->destroy_texture(data->texture);
				}

				data->rp_rects.clear();

				MEMORY_DELETE(data, core::memory::global_allocator());
			}
			detail::_fonts.clear();


			if (FT_Done_FreeType(detail::_ftlibrary))
			{
				LOGE("Failed to shutdown FreeType library!\n");
			}
		}

		void compute_uvs_for_rect(FontData* font, const stbrp_rect& rect, glm::vec2* uvs)
		{
			//
			// compute uvs
			const float TEXTURE_WIDTH = (float)FONT_ATLAS_RESOLUTION;
			const float TEXTURE_HEIGHT = (float)FONT_ATLAS_RESOLUTION;

			int border = font->border;

			// lower left
			uvs[0] = glm::vec2(
							   ((rect.x + border) / TEXTURE_WIDTH),
							   ((rect.y + border) / TEXTURE_HEIGHT)
							   );

			// lower right
			uvs[1] = glm::vec2(
							   ((rect.x + rect.w - border) / TEXTURE_WIDTH),
							   ((rect.y + border) / TEXTURE_HEIGHT)
							   );

			// upper right
			uvs[2] = glm::vec2(
							   ((rect.x + rect.w - border) / TEXTURE_WIDTH),
							   ((rect.y + rect.h - border) / TEXTURE_HEIGHT)
							   );

			// upper left
			uvs[3] = glm::vec2(
							   ((rect.x + border) / TEXTURE_WIDTH),
							   ((rect.y + rect.h - border) / TEXTURE_HEIGHT)
							   );
		}

		int uvs_for_codepoint(FontData* font, uint32_t codepoint, glm::vec2* uvs)
		{
			assert(font);

			// we don't handle 'space' characters.
			assert(codepoint > 0);

			stbrp_rect* rect = nullptr;
			rect = font->get_glyph(codepoint);
			// see if the codepoint is in the cache
			if (rect)
			{
				// the rect wasn't packed in our atlas
				assert(rect->was_packed);

				// the rect isn't associated with the requested code point
				assert(static_cast<uint32_t>(rect->id) == codepoint);

				compute_uvs_for_rect(font, *rect, uvs);
				return 0;
			}

			// get the glyph index
			FT_ULong glyph_index = FT_Get_Char_Index(font->face, codepoint);

			// load the glyph
			FT_Load_Glyph(font->face, glyph_index, FT_LOAD_RENDER);

			FT_Bitmap* bitmap = &font->face->glyph->bitmap;

			// the pitch is the number of bytes including padding of one bitmap
			// row. It is positive when the bitmap has a 'down' flow and
			// negative when the bitmap has an 'up' flow.

			// bitmap's dimensions are valid
			assert(bitmap->rows > 0 && bitmap->width > 0 && bitmap->pitch > 0);

			// bitmap is 8-bit grayscale
			assert(FT_PIXEL_MODE_GRAY == bitmap->pixel_mode);

			font->border = 2;
			render2::Image img;
			img.flags = image::F_ALPHA;
			img.width = bitmap->width + (2 * font->border);
			img.height = bitmap->rows + (2 * font->border);
			img.channels = 1;
			img.create(img.width, img.height, 1);
			img.alignment = 1; // tightly packed
			img.fill(core::Color(255, 0, 255));

			img.copy(bitmap->buffer, bitmap->width, bitmap->rows, bitmap->pitch, font->border);

			// insert the glyph into our cache
			stbrp_rect newrect;
			newrect.id = codepoint;
			newrect.x = 0;
			newrect.y = 0;
			newrect.w = img.width;
			newrect.h = img.height;

			// pack the rect in the font atlas (or not)
			stbrp_pack_rects(&font->rp_context, &newrect, 1);

			if (newrect.was_packed)
			{
				font->rp_rects.push_back(newrect);

				// the rect was packed; so update the texture
				detail::_device->update_texture(font->texture, img, glm::vec2(newrect.x, newrect.y), glm::vec2(newrect.w, newrect.h));

				compute_uvs_for_rect(font, newrect, uvs);
				return 0;
			}
			else
			{
				// I dunno.
				LOGW("Unable to pack codepoint: %i\n", codepoint);
				return 1;
			}
		}

		Handle load_from_memory(const void* data, unsigned int data_size, unsigned int pixel_size, Type target_type)
		{
			Handle handle;
			FT_Error error = FT_Err_Ok;

			FontData* font = MEMORY_NEW(FontData, core::memory::global_allocator());

			// font needs a copy of the data so long as FT_Face is loaded.
			// so make a local copy and store it.
			font->data = MEMORY_ALLOC(data_size, core::memory::global_allocator());
			font->data_size = data_size;
			memcpy(font->data, data, data_size);

			// try to parse the font data
			FT_Face face = nullptr;
			error = FT_New_Memory_Face(detail::_ftlibrary, (const FT_Byte*)font->data, font->data_size, 0, &face);
			assert(error == FT_Err_Ok);
			if (error == FT_Err_Unknown_File_Format)
			{
				LOGE("Invalid ttf format!\n");
				return handle;
			}
			else if (error != FT_Err_Ok)
			{
				LOGE("Could not read font file\n");
				return handle;
			}

			// try to set the pixel size of characters
			error = FT_Set_Pixel_Sizes(face, 0, pixel_size);
			if (error != FT_Err_Ok)
			{
				LOGW("Error while setting the character size!\n");
				assert(0);
			}


			handle.ref = detail::_fonts.size();
			font->type = FONT_TYPE_BITMAP;
			font->face = face;

			// cache line height and other flags
			font->line_height = (font->face->ascender - font->face->descender) >> 6;
			font->is_fixed_width = FT_HAS_FIXED_SIZES(face);
			font->has_kerning = FT_HAS_KERNING(face);

			// initialize the rect packing context
			stbrp_init_target(&font->rp_context, FONT_ATLAS_RESOLUTION, FONT_ATLAS_RESOLUTION, font->rp_nodes, FONT_INITIAL_RECT_TOTAL);

			render2::Image image;
			image.filter = image::FILTER_NONE;
			image.flags = image::F_ALPHA | image::F_CLAMP;
			image.width = FONT_ATLAS_RESOLUTION;
			image.height = FONT_ATLAS_RESOLUTION;
			image.channels = 1;

			image.create(image.width, image.height, image.channels);
			image.fill(core::Color(255, 0, 255));
			font->texture = detail::_device->create_texture(image);
			detail::_fonts.push_back(font);

			return handle;
		}

		unsigned int get_point_size(Handle handle)
		{
			return 0;
		}

		int get_glyph_metrics(Handle handle, uint32_t codepoint, glm::vec2& mins, glm::vec2& maxs, int* advance)
		{
			return 0;
		}

		int get_string_metrics(Handle handle, const char* utf8, glm::vec2& mins, glm::vec2& maxs)
		{
			return 0;
		}



		void draw_string(Handle handle, Array<FontVertex>& vertices, const char* utf8, const core::Color& color)
		{
			// font handle is invalid; nothing to do
			if (!handle.is_valid())
				return;

			FontData* data = detail::_fonts[handle.ref];

			glm::vec2 pen(0.0f, 0.0f);


			FontVertex* vertex;

			// this is treated as ANSI for now.
			size_t length = core::str::len(utf8);

			// we need length * 2 triangles
			vertices.resize(length * 6);

			uint32_t previous_codepoint = 0;

			for (size_t index = 0; index < length; ++index)
			{
				uint32_t codepoint = utf8[index];

				GlyphData gd;
				get_gylph_info(data->face, codepoint, gd);

				glm::vec2 offset(gd.hbearingx, -gd.hbearingy);

				if (previous_codepoint != 0 && data->has_kerning)
				{
					FT_Vector kerning_delta;
					FT_Get_Kerning(data->face, previous_codepoint, gd.index, FT_KERNING_DEFAULT, &kerning_delta);

					pen.x += kerning_delta.x >> 6;
				}

				glm::vec2 uvs[4];
				if (codepoint > 32)
					uvs_for_codepoint(data, codepoint, uvs);

				// build the rects in counter-clockwise order
				vertex = &vertices[index * 6];

				glm::vec2 pos = offset + pen;

				// lower left
				vertex->position = pos + glm::vec2(0, gd.height);
				vertex->color = color;
				vertex->uv = uvs[0];
				vertex++;

				// lower right
				vertex->position = pos + glm::vec2(gd.width, gd.height);
				vertex->color = color;
				vertex->uv = uvs[1];
				vertex++;

				// upper right
				vertex->position = pos + glm::vec2(gd.width, 0);
				vertex->color = color;
				vertex->uv = uvs[2];
				vertex++;

				vertex->position = pos + glm::vec2(gd.width, 0);
				vertex->color = color;
				vertex->uv = uvs[2];
				vertex++;

				// upper left
				vertex->position = pos;
				vertex->color = color;
				vertex->uv = uvs[3];
				vertex++;

				// lower left
				vertex->position = pos + glm::vec2(0, gd.height);
				vertex->color = color;
				vertex->uv = uvs[0];
				vertex++;

				pen.x += gd.advancex;
				previous_codepoint = gd.index;
			}
		}

		render2::Texture* get_font_texture(Handle handle)
		{
			if (handle.is_valid())
			{
				FontData* data = detail::_fonts[handle.ref];
				return data->texture;
			}
			return nullptr;
		}
	} // namespace font
} // namespace render2