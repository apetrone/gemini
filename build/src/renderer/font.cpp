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

#include <core/logging.h>
#include <core/typedefs.h>
#include <core/str.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_RECT_PACK_IMPLEMENTATION
#if defined(PLATFORM_COMPILER_MSVC)
	// disable some warnings in stb_rect_pack
	#pragma warning(push)
	#pragma warning(disable: 4100) // 'foo': unreferenced formal parameter
	#pragma warning(disable: 4365) // 'argument': conversion from 'int' to 'size_t', signed/unsigned mismatch
	#pragma warning(disable: 4505) // 'foo' : unreferenced local function has been removed
#endif

#include "stb_rect_pack.h"

#if defined(PLATFORM_COMPILER_MSVC)
	#pragma warning(pop)
#endif

namespace font
{
	struct Glyph
	{
		uint32_t codepoint;
		stbrp_rect rect;
	};

	const size_t FONT_INITIAL_RECT_TOTAL = 256;
	const size_t FONT_ATLAS_RESOLUTION = 256;

	gemini::Allocator* _font_allocator = nullptr;

	struct GlyphData
	{
		int advancex;
		int advancey;
		uint32_t width;
		uint32_t height;
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
		FT_UInt glyph_index = FT_Get_Char_Index(face, static_cast<FT_ULong>(codepoint));
		FT_Error error = FT_Err_Ok;

		// glyph doesn't exist in the font; need to display blank rect?
		// invalid glyphs will have an index of 0
		if (glyph_index == 0)
		{
//			LOGW("Invalid glyph %i -> %c\n", codepoint, codepoint);
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


//			FT_BBox cbox;
//			FT_Glyph current;
//			FT_Get_Glyph(face->glyph, &current);
//			FT_Glyph_Get_CBox(current, FT_GLYPH_BBOX_PIXELS, &cbox);
//			FT_Done_Glyph(current);


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

		glyphdata.width = static_cast<uint32_t>(face->glyph->metrics.width >> 6);
		glyphdata.height = static_cast<uint32_t>(face->glyph->metrics.height >> 6);

		assert(glyphdata.width == face->glyph->bitmap.width);
		assert(glyphdata.height == face->glyph->bitmap.rows);

		if (FT_HAS_VERTICAL(face))
		{
			glyphdata.vbearingx = face->glyph->metrics.vertBearingX >> 6;
			glyphdata.vbearingy = face->glyph->metrics.vertBearingY >> 6;
		}
	}

	// internal data
	struct FontData
	{
		Type type;
		uint32_t pixel_size;
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
		uint32_t line_height;
		uint32_t border;

		HashSet<int, GlyphData*> glyphdata_cache;

		FontData(gemini::Allocator& allocator)
			: type(FONT_TYPE_INVALID)
			, pixel_size(0)
			, face(nullptr)
			, data(nullptr)
			, data_size(0)
			, texture(nullptr)
			, has_kerning(0)
			, is_fixed_width(0)
			, line_height(0)
			, border(0)
			, rp_rects(allocator)
			, glyphdata_cache(allocator)
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

		GlyphData* lookup_glyph_data(int codepoint)
		{
			// check the glyph data cache for this codepoint
			if (glyphdata_cache.has_key(codepoint))
			{
				return glyphdata_cache.get(codepoint);
			}

			// create a new one, populate it and store in the cache
			GlyphData* glyph_data = MEMORY_NEW(GlyphData, core::memory::global_allocator());
			get_gylph_info(face, codepoint, *glyph_data);
			glyphdata_cache[codepoint] = glyph_data;

			return glyph_data;
		}
	};


	namespace detail
	{
		FT_Library _ftlibrary;
		Array<FontData*>* _fonts;
		render2::Device* _device = nullptr;

		void delete_font_data(FontData* data)
		{
			FT_Done_Face(data->face);

			MEMORY_DEALLOC(data->data, core::memory::global_allocator());

			if (data->texture)
			{
				detail::_device->destroy_texture(data->texture);
			}

			data->rp_rects.clear();

			// iterate over instances in glyphdata_cache and delete them
			for (HashSet<int, GlyphData*>::Iterator it = data->glyphdata_cache.begin(); it != data->glyphdata_cache.end(); ++it)
			{
				GlyphData* gd = it.value();
				MEMORY_DELETE(gd, core::memory::global_allocator());
			}

			data->glyphdata_cache.clear();

			MEMORY_DELETE(data, core::memory::global_allocator());
		}

	} // namespace detail

	bool Handle::is_valid() const
	{
		size_t reference = static_cast<size_t>(ref);
		return (detail::_fonts->size() > reference) && (ref != -1);
	}


	// ---------------------------------------------------------------------
	// implementation
	// ---------------------------------------------------------------------

	void startup(gemini::Allocator& allocator, render2::Device* device)
	{
		_font_allocator = &allocator;

		detail::_fonts = MEMORY2_NEW(allocator, Array<FontData*>)(allocator);

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
		for (auto& data : (*detail::_fonts))
		{
			detail::delete_font_data(data);
		}

		MEMORY2_DELETE(*_font_allocator, detail::_fonts);


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

		uint32_t border = font->border;

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

	int uvs_for_codepoint(FontData* font, int codepoint, glm::vec2* uvs)
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
			assert(rect->id == codepoint);

			compute_uvs_for_rect(font, *rect, uvs);
			return 0;
		}

		// get the glyph index
		FT_ULong glyph_index = FT_Get_Char_Index(font->face, static_cast<FT_ULong>(codepoint));

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

		// There is currently no support for paletted bitmaps
		assert(bitmap->palette == nullptr);

		font->border = 2;
		render2::Image img(*_font_allocator);
		img.flags = image::F_ALPHA;
		img.width = bitmap->width + (2 * font->border);
		img.height = bitmap->rows + (2 * font->border);
		img.channels = 1;
		img.create(img.width, img.height, 1);
		img.alignment = 1; // tightly packed
		img.fill(gemini::Color(1.0f, 0, 1.0f));

		img.copy(bitmap->buffer,
			bitmap->width,
			static_cast<uint32_t>(bitmap->rows),
			static_cast<uint32_t>(bitmap->pitch),
			font->border);

		// insert the glyph into our cache
		stbrp_rect newrect;
		newrect.id = codepoint;
		newrect.x = 0;
		newrect.y = 0;
		newrect.w = static_cast<stbrp_coord>(img.width);
		newrect.h = static_cast<stbrp_coord>(img.height);

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

	Handle load_from_memory(const void* data, size_t data_size, size_t pixel_size, Type /*target_type*/)
	{
		Handle handle;
		FT_Error error = FT_Err_Ok;

		FontData* font = MEMORY_NEW(FontData, core::memory::global_allocator())(*_font_allocator);

		// font needs a copy of the data so long as FT_Face is loaded.
		// so make a local copy and store it.
		font->data = MEMORY_ALLOC(data_size, core::memory::global_allocator());
		font->data_size = data_size;
		font->pixel_size = static_cast<uint32_t>(pixel_size);
		memcpy(font->data, data, data_size);

		// try to parse the font data
		FT_Face face = nullptr;
		error = FT_New_Memory_Face(detail::_ftlibrary, static_cast<const FT_Byte*>(font->data), static_cast<FT_Long>(font->data_size), 0, &face);
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
		error = FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(pixel_size));
		if (error != FT_Err_Ok)
		{
			LOGW("Error while setting the character size!\n");
			assert(0);
		}


		handle.ref = static_cast<int>(detail::_fonts->size());
		font->type = FONT_TYPE_BITMAP;
		font->face = face;

		// cache line height and other flags
		font->line_height = static_cast<uint32_t>((font->face->ascender - font->face->descender) >> 6);
		font->is_fixed_width = FT_HAS_FIXED_SIZES(face);
		font->has_kerning = FT_HAS_KERNING(face);

		// initialize the rect packing context
		stbrp_init_target(&font->rp_context, FONT_ATLAS_RESOLUTION, FONT_ATLAS_RESOLUTION, font->rp_nodes, FONT_INITIAL_RECT_TOTAL);

		render2::Image image(*_font_allocator);
		image.filter = image::FILTER_NONE;
		image.flags = image::F_ALPHA | image::F_CLAMP;

		image.create(FONT_ATLAS_RESOLUTION, FONT_ATLAS_RESOLUTION, 1);
		image.fill(gemini::Color(1.0f, 0.0f, 1.0f));
		font->texture = detail::_device->create_texture(image);
		(*detail::_fonts).push_back(font);

		return handle;
	}

	void destroy_font(Handle& handle)
	{
		FontData* font = (*detail::_fonts)[static_cast<size_t>(handle.ref)];
		detail::delete_font_data(font);

		// invalidate the handle
		handle.ref = -1;
	}

	unsigned int get_pixel_size(Handle handle)
	{
		FontData* font = (*detail::_fonts)[static_cast<size_t>(handle.ref)];
		return font->pixel_size;
	}

	void get_font_metrics(Handle handle, Metrics& out_metrics)
	{
		if (!handle.is_valid())
		{
			return;
		}

		FontData* font = (*detail::_fonts)[static_cast<size_t>(handle.ref)];
		FT_Size_Metrics& sm = font->face->size->metrics;
		out_metrics.height = sm.height >> 6;
		out_metrics.ascender = sm.ascender >> 6;
		out_metrics.descender = sm.descender >> 6;
		out_metrics.max_height = glm::max((int)out_metrics.height, (out_metrics.ascender - out_metrics.descender));
//		LOGV("height = %i, ascender = %i, descender = %i (pixels)\n", out_metrics.height, out_metrics.ascender, out_metrics.descender);
	}

	int get_glyph_metrics(Handle /*handle*/, uint32_t /*codepoint*/, glm::vec2& /*mins*/, glm::vec2& /*maxs*/, int* /*advance*/)
	{
		return 0;
	}

	int get_string_metrics(Handle handle, const char* utf8, size_t string_length, glm::vec2& mins, glm::vec2& maxs)
	{
		if (!handle.is_valid())
		{
			return -1;
		}

		FontData* font = (*detail::_fonts)[static_cast<size_t>(handle.ref)];
		glm::vec2 pen;
		uint32_t previous_codepoint = 0;


		mins = glm::vec2(0.0f);
		maxs = glm::vec2(0.0f);

		float largest_width = 0;
		float largest_height = 0;

		for (size_t index = 0; index < string_length; ++index)
		{
			int codepoint = utf8[index];
			GlyphData gd;
			get_gylph_info(font->face, codepoint, gd);

			if (gd.height > largest_height)
				largest_height = static_cast<float>(gd.height);

			if (previous_codepoint != 0 && font->has_kerning)
			{
				FT_Vector kerning_delta;
				FT_Get_Kerning(font->face, previous_codepoint, gd.index, FT_KERNING_DEFAULT, &kerning_delta);

				pen.x += kerning_delta.x >> 6;
			}

			pen.x += gd.advancex;
			previous_codepoint = gd.index;
			if (pen.x > largest_width)
				largest_width = static_cast<float>(pen.x);
		}

		maxs.x = largest_width;
		maxs.y = largest_height;

		return 0;
	}

	size_t count_vertices(Handle /*handle*/, size_t string_length)
	{
		return 6 * string_length;
	}

	size_t draw_string(Handle handle, FontVertex* vertices, const char* utf8, size_t string_length, const gemini::Color& color)
	{
		// font handle is invalid; nothing to do
		if (!handle.is_valid())
			return 0;

		FontData* data = (*detail::_fonts)[static_cast<size_t>(handle.ref)];

		glm::vec2 pen(0.0f, 0.0f);

		FontVertex* vertex;

		uint32_t previous_codepoint = 0;
		for (size_t index = 0; index < string_length; ++index)
		{
			int codepoint = utf8[index];

			GlyphData* glyph_data = data->lookup_glyph_data(codepoint);
			const GlyphData& gd = *glyph_data;

			if (previous_codepoint != 0 && data->has_kerning)
			{
				FT_Vector kerning_delta;
				FT_Get_Kerning(data->face, previous_codepoint, gd.index, FT_KERNING_DEFAULT, &kerning_delta);

				pen.x += kerning_delta.x >> 6;
			}

			glm::vec2 uvs[4];
			if (codepoint > 32)
			{
				uvs_for_codepoint(data, codepoint, uvs);
			}

			// build the rects in counter-clockwise order
			vertex = &vertices[index * 6];

			glm::vec2 offset(gd.hbearingx, -gd.hbearingy);
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

		return string_length * 6;
	}

	render2::Texture* get_font_texture(Handle handle)
	{
		if (handle.is_valid())
		{
			FontData* data = (*detail::_fonts)[static_cast<size_t>(handle.ref)];
			return data->texture;
		}

		// Unable to get the font texture for handle!
		assert(0);
		return nullptr;
	}
} // namespace font
