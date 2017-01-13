// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "unit_test.h"

#include <core/core.h>
#include <core/logging.h>

#include <platform/platform.h>

#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/jobqueue.h>
#include <runtime/geometry.h>

#include <runtime/http.h>

#include <assert.h>

#include <core/mathlib.h>

// ---------------------------------------------------------------------
// jobqueue
// ---------------------------------------------------------------------

void print_string(const char* data)
{
	LOGV("thread: 0x%x, string: %s\n", (size_t)platform::thread_id(), data);
}

float mat3_determinant(const glm::mat3& xform)
{
	return
		+xform[0][0] * (xform[1][1] * xform[2][2] - xform[2][1] * xform[1][2])
		- xform[1][0] * (xform[0][1] * xform[2][2] - xform[2][1] * xform[0][2])
		+ xform[2][0] * (xform[0][1] * xform[1][2] - xform[1][1] * xform[0][2]);
}



UNITTEST(Geometry)
{
	using namespace gemini;
	glm::vec3 vertices[] = {
		glm::vec3(-2.0f, 3.0f, 0.0f),
		glm::vec3(-2.0f, 1.0f, 0.0f),
		glm::vec3(2.0f, 3.0f, 0.0f),
		glm::vec3(2.0f, 1.0f, 0.0f)
	};

	glm::mat3 xf(
		3, 2, 4,
		2, 0, 2,
		4, 2, 3);

	//
	assert(mat3_determinant(xf) == glm::determinant(xf));

	OrientedBoundingBox box;
	compute_oriented_bounding_box_by_points(box, vertices, 4);
}

UNITTEST(jobqueue)
{
	const size_t MAX_JOB_ITERATIONS = 6;
	const char* iterations[] = {
		"ALPHA: 0",
		"ALPHA: 1",
		"ALPHA: 2",
		"ALPHA: 3",
		"ALPHA: 4",
		"ALPHA: 5",
		"ALPHA: 6",
		"ALPHA: 7",
		"ALPHA: 8",
		"ALPHA: 9",
		"BETA: 0",
		"BETA: 1",
		"BETA: 2",
		"BETA: 3",
		"BETA: 4",
		"BETA: 5",
		"BETA: 6",
		"BETA: 7",
		"BETA: 8",
		"BETA: 9",
		"DELTA: 0",
		"DELTA: 1",
		"DELTA: 2",
		"DELTA: 3",
		"DELTA: 4",
		"DELTA: 5",
		"DELTA: 6",
		"DELTA: 7",
		"DELTA: 8",
		"DELTA: 9",
		"EPSILON: 0",
		"EPSILON: 1",
		"EPSILON: 2",
		"EPSILON: 3",
		"EPSILON: 4",
		"EPSILON: 5",
		"EPSILON: 6",
		"EPSILON: 7",
		"EPSILON: 8",
		"EPSILON: 9",
		"FOXTROT: 0",
		"FOXTROT: 1",
		"FOXTROT: 2",
		"FOXTROT: 3",
		"FOXTROT: 4",
		"FOXTROT: 5",
		"FOXTROT: 6",
		"FOXTROT: 7",
		"FOXTROT: 8",
		"FOXTROT: 9",
		"GAMMA: 0",
		"GAMMA: 1",
		"GAMMA: 2",
		"GAMMA: 3",
		"GAMMA: 4",
		"GAMMA: 5",
		"GAMMA: 6",
		"GAMMA: 7",
		"GAMMA: 8",
		"GAMMA: 9"
	};

	gemini::Allocator default_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
	gemini::JobQueue jq(default_allocator);
	jq.create_workers(3);

	for (size_t index = 0; index < MAX_JOB_ITERATIONS; ++index)
	{
		jq.push_back(print_string, iterations[(index * 10) + 0]);
		jq.push_back(print_string, iterations[(index * 10) + 1]);
		jq.push_back(print_string, iterations[(index * 10) + 2]);
		jq.push_back(print_string, iterations[(index * 10) + 3]);
		jq.push_back(print_string, iterations[(index * 10) + 4]);
		jq.push_back(print_string, iterations[(index * 10) + 5]);
		jq.push_back(print_string, iterations[(index * 10) + 6]);
		jq.push_back(print_string, iterations[(index * 10) + 7]);
		jq.push_back(print_string, iterations[(index * 10) + 8]);
		jq.push_back(print_string, iterations[(index * 10) + 9]);

		jq.wait_for_jobs_to_complete();

		platform::thread_sleep(250);
	}

	LOGV("destroying workers...\n");
	jq.destroy_workers();
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	core::filesystem::IFileSystem* fs = core::filesystem::instance();
	TEST_ASSERT(fs != nullptr, filesystem_instance_exists);

	//	platform::PathString content_path;
	//	content_path = fs->root_directory();
	//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
	//	fs->content_directory(content_path);


	//	platform::PathString absolute_path;
	//	TEST_ASSERT(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}

// ---------------------------------------------------------------------
// http
// ---------------------------------------------------------------------
UNITTEST(http)
{
	platform::net_startup();

	gemini::http_startup();

	//gemini::http_request_file("https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png", "./downloads/download.jpg", "httplib");
	//while (gemini::http_active_download_count() > 0)
	//{
	//	gemini::http_update();
	//}

	gemini::http_shutdown();

	platform::net_shutdown();
}


// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
UNITTEST(logging)
{
	TEST_ASSERT(core::logging::instance() != nullptr, log_instance_is_valid);

	LOGV("This is a test of the logging system!\n");

	LOGW("This is a warning\n");
	LOGE("This is an error!\n");

	LOGW("Warning, %i parameters missing!\n", 3);
}

#if 0
// load asset via path
// asset should be cached
// assets should be re-loadable
// assets live in runtime, so anything at runtime or above can access them.
// assets should have external create/destroy callbacks
//	- this allows a subsystem to customize that
// assets should be returned as handles.
//	- this allows handles to be invalid, but not return raw pointers.
//	- this also allows handles to be lazily loaded or streamed
// should allow a default asset of sorts -- optional. If a named asset cannot be found.
// lookup asset pointer by asset handle (id)
// purge all assets
// allow insertion of arbitrary assets (creating default material/texture, for example)
// allow a prefix-path to be prepended to the search path
// allow asset parameters (only texture uses this)
// specify an allocator which can be passed in to create/destroy functions
#endif

namespace gemini
{
	enum AssetLoadStatus
	{
		// The asset is loaded and ready to use.
		AssetLoad_Success,

		// The asset could not be loaded.
		AssetLoad_Failure
	};

	template <class T, class Y>
	class AssetLibrary2
	{
	public:
		struct Handle
		{
			uint32_t index;
		};

	protected:
		typedef uint16_t HandleIndex;
		typedef HashSet<platform::PathString, HandleIndex> HandleIndexByName;

		struct LoadState
		{
			Allocator* allocator;
			T* asset;
		}; // AssetLoadState

		Allocator& allocator;
		Array<T*> assets;
		T* default_asset;
		HandleIndexByName handle_by_name;
		platform::PathString prefix_uri;

	private:
		Y* instance()
		{
			return reinterpret_cast<Y*>(this);
		}

		Y& instance_reference()
		{
			return *instance();
		}

	public:

		AssetLibrary2(gemini::Allocator& asset_allocator)
			: allocator(asset_allocator)
			, assets(asset_allocator)
			, default_asset(nullptr)
			, handle_by_name(asset_allocator)
		{
		}

		virtual ~AssetLibrary2()
		{
			purge();
		}

		void default(T* asset)
		{
			default_asset = asset;
		}

		T* default() const
		{
			return default_asset;
		}

		bool handle_is_valid(Handle handle)
		{
			return (handle.index > 0) && (handle.index <= assets.size());
		} // handle_is_valid

		Handle load(const char* relative_path, bool ignore_cache = false)
		{
			// 1. Check to see if the asset is already loaded...
			platform::PathString fullpath = prefix_uri;
			fullpath.append(relative_path);
			uint8_t asset_is_new = 1;
			HandleIndex handle_index;
			if (handle_by_name.has_key(fullpath()))
			{
				asset_is_new = 0;
				handle_index = handle_by_name[fullpath()];

				// An entry exists for it: return it.
				if (!ignore_cache)
				{
					Handle handle;
					handle.index = handle_index;
					return handle;
				}
			}

			LoadState load_state;
			load_state.asset = nullptr;
			load_state.allocator = &allocator;
			if (handle_index > 0)
			{
				// Populate with the loaded asset if exists.
				load_state.asset = assets[handle_index - 1];
			}

			AssetLoadStatus load_result = instance_reference().create(load_state, fullpath);
			if (load_result != AssetLoad_Failure)
			{
				Handle handle;
				if (asset_is_new)
				{
					handle = take_ownership(fullpath, load_state.asset, false);
				}

				return handle;
			}
			else
			{
				LOGW("FAILED to load asset [%s]\n", relative_path);
			}

			Handle handle;
			handle.index = 0;
			return handle;
		} // load

		T* lookup(Handle handle)
		{
			if (!handle_is_valid(handle))
			{
				return default_asset;
			}

			return assets[handle.index - 1];
		} // lookup

		void purge()
		{
			for (size_t index = 0; index < assets.size(); ++index)
			{
				LoadState load_state;
				load_state.allocator = &allocator;
				load_state.asset = assets[index];
				instance_reference().destroy(load_state);
			}
			assets.clear();
			handle_by_name.clear();
		} // purge

		void prefix_path(const platform::PathString& prefix)
		{
			prefix_uri = prefix;

			// ensure prefix_path has a trailing slash
			prefix_uri.strip_trailing(PATH_SEPARATOR);
			prefix_uri.append(PATH_SEPARATOR_STRING);
		} // prefix_path

		const platform::PathString& prefix_path() const
		{
			return prefix_uri;
		} // prefix_path

		Handle take_ownership(const platform::PathString& path, T* asset, bool verify_unique = true)
		{
			Handle handle;

			if (!verify_unique || !handle_by_name.has_key(path))
			{
				assert(assets.size() < USHRT_MAX);
				uint16_t index = static_cast<uint16_t>(assets.size());
				handle_by_name[path] = index + 1;
				assets.push_back(asset);

				handle.index = (index + 1);
				return handle;
			}

			handle.index = 0;
			return handle;
		} // take_ownership
	};

	struct Shader
	{
		int object_id;
	};

	class ShaderLibrary : public AssetLibrary2<Shader, ShaderLibrary>
	{
	public:

		ShaderLibrary(gemini::Allocator& allocator)
			: AssetLibrary2(allocator)
		{
		}

		AssetLoadStatus create(LoadState& state, platform::PathString& fullpath)
		{
			state.asset = MEMORY2_NEW(*state.allocator, Shader);
			state.asset->object_id = 37;

			return AssetLoad_Failure;
		}

		void destroy(LoadState& state)
		{
			MEMORY2_DELETE(*state.allocator, state.asset);
		}
	};
} // namespace gemini


int main(int, char**)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/test_runtime");

	using namespace gemini;

	//unittest::UnitTest::execute();

	gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_ASSETS);
	{
		ShaderLibrary sl(allocator);
		sl.prefix_path("shaders/150");

		Shader def_shader;
		def_shader.object_id = 42;
		sl.default(&def_shader);

		ShaderLibrary::Handle handle = sl.load("test");
		handle = sl.load("test2");
		handle = sl.load("test3");
		handle = sl.load("test4");
		handle = sl.load("test5");
		handle = sl.load("test6");

		Shader* shader_ptr = sl.lookup(handle);


		handle = sl.load("test6");
		Shader* second_request = sl.lookup(handle);

		assert(shader_ptr == second_request);
	}

	gemini::runtime_shutdown();
	gemini::core_shutdown();
	return 0;
}
