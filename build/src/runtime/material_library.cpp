// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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

#include <runtime/material_library.h>

#include <core/logging.h>
#include <core/mem.h>


namespace gemini
{
	MaterialLibrary::MaterialLibrary(Allocator& allocator, render2::Device* render_device)
		: AssetLibrary2(allocator)
		, device(render_device)
	{
	}

	AssetLoadStatus MaterialLibrary::create(LoadState& state, platform::PathString& fullpath)
	{
		LOGV("loading material \"%s\"\n", fullpath());

		//platform::PathString asset_uri = fullpath;
		//asset_uri.append(".material");

		//bool is_new_asset = state.asset == nullptr;
		//if (is_new_asset)
		//{
		//	state.asset = MEMORY2_NEW(*state.allocator, Mesh)(*state.allocator);
		//}

		//if (core::util::json_load_with_callback(asset_uri(), load_json_model, &state, true) == core::util::ConfigLoad_Success)
		//{
		//	return AssetLoad_Success;
		//}

		//if (is_new_asset)
		//{
		//	MEMORY2_DELETE(*state.allocator, state.asset);
		//}

		return AssetLoad_Failure;
	}

	void MaterialLibrary::destroy(LoadState& state)
	{
		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini
