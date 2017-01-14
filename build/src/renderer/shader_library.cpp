// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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

#include <core/logging.h>
#include <core/mem.h>

#include <renderer/renderer.h>
#include <runtime/filesystem.h>

#include <renderer/shader_library.h>

namespace gemini
{
	ShaderLibrary::ShaderLibrary(Allocator& allocator, render2::Device* render_device)
		: AssetLibrary2(allocator)
		, device(render_device)
	{
	}

	AssetLoadStatus ShaderLibrary::create(LoadState& state, platform::PathString& fullpath)
	{
		LOGV("create shader \"%s\"\n", fullpath());

		if (state.asset)
		{
			// re-load a shader
			assert(0); // not yet implemented!
		}
		else
		{
			// We could determine here how many stages we need to load.
			// I'll leave that as a TODO for now...

			platform::PathString asset_uri = fullpath;
			asset_uri.append(".vert");

			// create shaders
			Array<unsigned char> vertex_shader_source(*state.allocator);
			core::filesystem::instance()->virtual_load_file(vertex_shader_source, asset_uri());
			assert(!vertex_shader_source.empty());

			render2::ShaderSource vertex_source;
			vertex_source.data = &vertex_shader_source[0];
			vertex_source.data_size = vertex_shader_source.size();
			vertex_source.stage_type = render2::SHADER_STAGE_VERTEX;

			asset_uri = fullpath;
			asset_uri.append(".frag");
			Array<unsigned char> fragment_shader_source(*state.allocator);
			core::filesystem::instance()->virtual_load_file(fragment_shader_source, asset_uri());
			assert(!fragment_shader_source.empty());

			render2::ShaderSource frag_source;
			frag_source.data = &fragment_shader_source[0];
			frag_source.data_size = fragment_shader_source.size();
			frag_source.stage_type = render2::SHADER_STAGE_FRAGMENT;

			render2::ShaderSource* sources[] = { &vertex_source, &frag_source };

			state.asset = device->create_shader(sources, 2);
		}

		return AssetLoad_Success;
	}

	void ShaderLibrary::destroy(LoadState& state)
	{
		device->destroy_shader(state.asset);
	}
} // namespace gemini
