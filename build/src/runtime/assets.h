// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#pragma once
#include <runtime/assets_common.h>

#include <renderer/renderer.h> // for ShaderString

namespace render2
{
	struct Texture;
	struct Shader;
} // namespace render2

namespace gemini
{
	struct Material;
	struct Mesh;
	struct FontData;
	struct FontMetrics;
	struct FontVertex;
	struct FontCreateParameters;
	struct Sound;


	namespace assets
	{
		// called to initialize default textures and other required resources.
		void startup(render2::Device* device, bool load_default_assets = true);

		// purge all assets
		void purge();

		// purge all assets and reclaim unused memory
		void shutdown();
	} // namespace assets

	AssetHandle mesh_load(const char* path, bool ignore_cache = false, void* parameters = nullptr);
	Mesh* mesh_from_handle(AssetHandle handle);

	AssetHandle shader_load(const char* path, bool ignore_cache = false, void* parameters = nullptr);
	render2::Shader* shader_from_handle(AssetHandle handle);

	AssetHandle texture_load(const char* path, bool ignore_cache = false, void* parameters = nullptr);
	render2::Texture* texture_from_handle(AssetHandle handle);

	AssetHandle material_load(const char* path, bool ignore_cache = false, void* parameters = nullptr);
	Material* material_from_handle(AssetHandle handle);

	/// @returns The number of vertices required to render string with
	/// string_length in characters.
	size_t font_count_vertices(size_t string_length);
	size_t font_draw_string(AssetHandle handle, FontVertex* vertices, const char* utf8, size_t string_length, const Color& color);

	AssetHandle font_load(const char* path, bool ignore_cache, FontCreateParameters* parameters);

	// retrieve metrics for this font
	void font_metrics(AssetHandle handle, FontMetrics& out_metrics);
	FontData* font_from_handle(AssetHandle handle);

	// retrieve the font texture used by a font
	render2::Texture* font_texture(AssetHandle handle);

	// fetch metrics for a string
	int32_t font_string_metrics(AssetHandle handle, const char* utf8, size_t string_length, glm::vec2& mins, glm::vec2& maxs);


	AssetHandle sound_load(const char* path, bool ignore_cache, void* parameters = nullptr);
	Sound* sound_from_handle(AssetHandle handle);
} // namespace gemini
