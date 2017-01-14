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
#include "typedefs.h"
#include "filesystem.h"

#include <runtime/configloader.h>
#include <runtime/runtime.h>


#include <core/logging.h>
#include <core/str.h>
#include <core/config.h>
#include <core/argumentparser.h>

#include <platform/platform.h>
#include <platform/window.h>

#include <renderer/renderer.h>

#include "filesystem_interface.h"



using platform::PathString;
using namespace core;

namespace gemini
{
	namespace detail
	{
		struct RuntimeState
		{
			gemini::Allocator allocator;
			render2::Device* render_device;
		}; // RuntimeState

		RuntimeState _runtime_state;
		RuntimeState* _state = &_runtime_state;

		// log handler function declarations
		void file_logger_message(core::logging::Handler* handler, const char* message, const char* filename, const char* function, int line, int type)
		{
			core::StackString<MAX_PATH_SIZE> path = filename;
			fprintf((FILE*)handler->userdata, "[%i %s %s %i] %s", type, path.basename()(), function, line, message);
			//fprintf( (FILE*)handler->userdata, "\t%s", message );
			fflush((FILE*)handler->userdata);
		}

		int file_logger_open(core::logging::Handler* handler)
		{
			const char* logname = (const char*)handler->userdata;
			handler->userdata = fopen(logname, "wb");
			return handler->userdata != 0;
		}

		void file_logger_close(core::logging::Handler* handler)
		{
			if (handler->userdata)
			{
				fclose((FILE*)handler->userdata);
			}
		}

		class RuntimeResourceProvider : public render2::ResourceProvider
		{
		public:
			virtual bool load_file(Array<unsigned char>& data, const char* filename) const override
			{
				core::filesystem::instance()->virtual_load_file(data, filename);
				return true;
			}

			virtual bool file_exists(const char* filename) const override
			{
				return core::filesystem::instance()->file_exists(filename);
			}
		}; // RuntimeResourceProvider

		static RuntimeResourceProvider resource_provider;
	}

	// initialize filesystem

	// set paths

	// initialize handlers

	platform::Result runtime_startup(const char* application_data_path,
		std::function<void(const char*)> custom_path_setup,
		uint32_t runtime_flags)
	{
		detail::_state->allocator = memory_allocator_default(MEMORY_ZONE_RUNTIME);
		detail::_state->render_device = nullptr;

		platform::PathString root_path = platform::get_program_directory();
		LOGV("root_path: %s\n", root_path());
		platform::PathString content_path = platform::fs_content_directory();
		LOGV("content_path: %s\n", content_path());

		// create file system instance
		core::filesystem::IFileSystem* filesystem = MEMORY2_NEW(detail::_state->allocator, core::filesystem::FileSystemInterface);
		core::filesystem::set_instance(filesystem);
		if (!filesystem)
		{
			return platform::Result::failure("Unable to create filesystem instance");
		}

		if (custom_path_setup)
		{
			custom_path_setup(application_data_path);
		}
		else
		{
			// default path setup
			platform::PathString application_path = platform::get_user_application_directory(application_data_path);
			core::filesystem::instance()->root_directory(root_path);
			core::filesystem::instance()->content_directory(content_path);
			core::filesystem::instance()->user_application_directory(application_path);
		}

#if defined(PLATFORM_FILESYSTEM_SUPPORT)
		if (runtime_flags & RF_SAVE_LOGS_TO_DISK)
		{

			// install disk logging handler
			const char GEMINI_LOG_PATH[] = "logs";
			const unsigned int GEMINI_DATETIME_STRING_MAX = 128;

			platform::DateTime dt;
			platform::datetime(dt);


			core::filesystem::IFileSystem* fs = core::filesystem::instance();

			char datetime_string[GEMINI_DATETIME_STRING_MAX];
			core::str::sprintf(datetime_string, GEMINI_DATETIME_STRING_MAX, "%02d-%02d-%04d-%02d-%02d-%02d.log",
				dt.month, dt.day, dt.year, dt.hour, dt.minute, dt.second);

			platform::PathString log_directory;
			log_directory = fs->user_application_directory();
			log_directory.append(PATH_SEPARATOR_STRING).append(GEMINI_LOG_PATH).append(PATH_SEPARATOR_STRING);
			log_directory.normalize(PATH_SEPARATOR);

			// make sure target folder is created
			platform::path::make_directories(log_directory());

			log_directory.append(datetime_string);

			core::logging::Handler filelogger;
			filelogger.open = detail::file_logger_open;
			filelogger.close = detail::file_logger_close;
			filelogger.message = detail::file_logger_message;
			filelogger.userdata = (void*)log_directory();
			core::logging::instance()->add_handler(&filelogger);
		}
#endif

		// install the resource provider to the renderer?
		render2::set_resource_provider(&detail::resource_provider);

		if (runtime_flags & RF_WINDOW_SYSTEM)
		{
			// initialize window subsystem
			platform::window::startup(platform::window::RenderingBackend_Default);
		}

		// asset startup

		return platform::Result::success();
	}

	void runtime_shutdown()
	{
		// asset shutdown

		core::filesystem::instance()->shutdown();

		core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		MEMORY2_DELETE(detail::_state->allocator, filesystem);
	} // shutdown

	void runtime_load_arguments(std::vector<std::string>& arguments, ::core::argparse::ArgumentParser& parser)
	{
		const platform::MainParameters& mainparams = platform::get_mainparameters();
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
		arguments = parser.split_tokens(mainparams.argc, mainparams.argv);
#elif defined(PLATFORM_WINDOWS)
		arguments = parser.split_tokens(mainparams.commandline);
#else
	#error Not implemented on this platform!
#endif
	} // runtime_load_arguments

	short runtime_standard_port_for_service(const char * service)
	{
		if (core::str::case_insensitive_compare(service, "http", 4) == 0)
		{
			return 80;
		}
		else if (core::str::case_insensitive_compare(service, "https", 5) == 0)
		{
			return 443;
		}

		return 0;
	} // net_standard_port_for_service

	int32_t runtime_decompose_url(const char* url, char* filename, char* hostname, char* service_type, uint16_t* port)
	{
		const size_t url_length = str::len(url);
		const char* cur = url;
		const char* svc;
		int32_t slash_position = -1;
		int32_t host_start = -1;
		svc = str::strstr(url, "://");
		if (svc)
		{
			// found a service.
			str::copy(service_type, url, (svc - url));
		}

		// if the last character is a '/' then our job is easy
		if (url[url_length - 1] == '/')
		{
			//printf( "URL ends with a /\n" );
			slash_position = url_length - 1;
		}


		for (size_t index = 0; index < url_length; ++index)
		{
			if (host_start == -1 && cur[index] == '/' && cur[index + 1] == '/')
			{
				//printf( "Found Host Start: %i\n", i+2 );
				host_start = index + 2;
			}
			else if (host_start == -1 && cur[index] == '.')
			{
				host_start = 0;
			}

			if (slash_position == -1 && cur[index] == '/' && cur[index - 1] != '/' && cur[index + 1] != '/')
			{
				//printf( "Found Slash Pos: %i\n", i );
				slash_position = index;
				break;
			}
		}

		// special case when there is no ending slash
		if (slash_position == -1)
		{
			filename[0] = '/';
			slash_position = url_length;
		}
		else
		{
			for (size_t file_index = 0, source_index = slash_position; source_index < url_length; ++source_index, ++file_index)
			{
				filename[file_index] = url[source_index];

			}
		}

		for (size_t index = 0, source_index = host_start; source_index < slash_position; ++source_index, ++index)
		{
			hostname[index] = url[source_index];
		}


		// see if a special port was specified
		cur = strchr(hostname, ':');
		if (cur != 0)
		{
			// truncate host name
			hostname[cur - hostname] = 0;
			*port = atoi(cur + 1);
		}
		else
		{
			*port = runtime_standard_port_for_service(service_type);
		}


		return 0;
	} // runtime_decompose_url



	static util::ConfigLoadStatus load_render_config(const Json::Value& root, void* data)
	{
		::renderer::RenderSettings* settings = static_cast<::renderer::RenderSettings*>(data);

		// TODO: there should be a better way to do this?
		if (!root["gamma_correct"].isNull())
		{
			settings->gamma_correct = root["gamma_correct"].asBool();
		}

		return util::ConfigLoad_Success;
	} // load_render_config

	static util::ConfigLoadStatus settings_conf_loader(const Json::Value & root, void * data)
	{
		util::ConfigLoadStatus result = util::ConfigLoad_Success;

		Settings* cfg = (Settings*)data;
		if (!cfg)
		{
			return util::ConfigLoad_Failure;
		}

		const Json::Value& physics_tick_rate = root["physics_tick_rate"];
		if (!physics_tick_rate.isNull())
		{
			cfg->physics_tick_rate = physics_tick_rate.asUInt();
		}

		const Json::Value& enable_asset_reloading = root["enable_asset_reloading"];
		if (!enable_asset_reloading.isNull())
		{
			cfg->enable_asset_reloading = enable_asset_reloading.asBool();
		}

		const Json::Value& window_width = root["window_width"];
		if (!window_width.isNull())
		{
			cfg->window_width = window_width.asInt();
		}

		const Json::Value& window_height = root["window_height"];
		if (!window_height.isNull())
		{
			cfg->window_height = window_height.asInt();
		}

		const Json::Value& window_title = root["window_title"];
		if (!window_title.isNull())
		{
			cfg->window_title = window_title.asString().c_str();
		}

		const Json::Value& application_directory = root["application_directory"];
		if (!application_directory.isNull())
		{
			cfg->application_directory = application_directory.asString().c_str();
		}

		const Json::Value& renderer = root["renderer"];
		if (!renderer.isNull())
		{
			// load renderer settings
			result = load_render_config(renderer, &cfg->render_settings);
		}

		return result;
	} // settings_conf_loader

	bool runtime_load_application_config(Settings& config)
	{
		bool success = util::json_load_with_callback("conf/settings.conf", settings_conf_loader, &config, true);
		if (!success)
		{
			LOGW("Unable to load settings.conf! Let's hope wise defaults were chosen...\n");

			// This is hit when the game content path is invalid.
			assert(0);
		}

		return success;
	} // runtime_load_application_config
} // namespace gemini
