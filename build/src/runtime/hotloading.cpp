// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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


#include <json/json.h>

#include <runtime/hotloading.h>
#include <runtime/material_library.h>
#include <runtime/mesh_library.h>

#include <core/mathlib.h>
#include <core/stackstring.h>
#include <threadsafequeue.h>
#include <core/typedefs.h>

#include <runtime/runtime.h>

#if defined(USE_WEBSERVER)
#include "civetweb.h"
#include "CivetServer.h"
#endif

using namespace core;


#if 0



struct BaseVar
{
	BaseVar* next;
	std::string name;

	virtual void load(Json::Value& value) = 0;
	virtual std::string value_string() = 0;

	static void render_values(int x, int y);
};

BaseVar* tail = 0;

void BaseVar::render_values(int x, int y)
{
	BaseVar* current = tail;
	while (current)
	{
		debugdraw::text(x, y, xstr_format("[VAR] %s = %s", current->name.c_str(), current->value_string().c_str()), Color::from_rgba(255, 255, 255, 255));

		y += 12;
		current = current->next;
	}
}

BaseVar* find_by_name(const std::string& name)
{
	BaseVar* current = tail;

	while( current )
	{
		if (name == current->name)
		{
			return current;
		}
		current = current->next;
	}

	return 0;
}

template <class Type>
struct JsonLoader
{
	static void load(Json::Value& json_value, Type* value);
};

template <>
struct JsonLoader<float>
{
	static void load(Json::Value& json_value, float* value)
	{
		if (json_value.isDouble())
		{
			*value = json_value.asFloat();
		}
	}
};

template <>
struct JsonLoader<int>
{
	static void load(Json::Value& json_value, int* value)
	{
		if (json_value.isInt())
		{
			*value = json_value.asInt();
		}
	}
};

template <class Type>
struct Var : public BaseVar
{
	Type value;

	Var(const char* varname)
	{
		name = varname;
		next = tail;
		tail = this;
	}


	virtual void load(Json::Value& json_value)
	{
		JsonLoader<Type>::load(json_value, &value);
	}

	virtual std::string value_string()
	{
		std::string val = std::to_string(value);
		return val;
	}
};

//Var<float> meter("meter");
//Var<int> test("value_test");

void put_new_json(const std::string& json_document)
{
	Json::Value root;
	Json::Reader reader;

	bool success = reader.parse(json_document, root);
	if (success)
	{
		// apply this to variables
		Json::ValueIterator it = root.begin();

		size_t total_values_loaded = 0;
		size_t missing_values = 0;
		for( ; it != root.end(); ++it)
		{
			Json::Value key = it.key();
			Json::Value value = (*it);

			const std::string& name = key.asString();
			BaseVar* v = find_by_name(name);
			if (v)
			{
				v->load(value);
				++total_values_loaded;
			}
			else
			{
				++missing_values;
			}
		}

		LOGV("loaded (%u/%u) values\n", total_values_loaded, (total_values_loaded+missing_values));
	}
}
#endif

namespace gemini
{

	// not sure what else to call this...
	namespace hotloading
	{
		struct HotloadingState
		{
			NotificationClient notify_client;
		};

		HotloadingState* _hotloading_state = nullptr;

		namespace _internal
		{
#if defined(USE_WEBSERVER)
			CivetServer* server = nullptr;
#endif
			ThreadSafeQueue<String> reload_queue;
			gemini::Allocator* allocator = nullptr;

		}

#if defined(USE_WEBSERVER)


		class JsonConfigHandler : public CivetHandler
		{
			virtual bool handlePut(CivetServer* /*server*/, struct mg_connection* conn)
			{
//				const struct mg_request_info* request = mg_get_request_info(conn);
				String buf;

				buf.resize(1024);
				int bytes = mg_read(conn, &buf[0], 1024);
				assert(bytes < 1024);


				//			LOGV("read %i bytes, %s\n", bytes, buf.c_str());

	//			put_new_json(buf);

				// On Error, we can return "400 Bad Request",
				// but we'll have to provide a response body to describe why.

				String output = "204 No Content";
				mg_write(conn, &output[0], output.length());

				return true;
			}

			virtual bool handleDelete(CivetServer* /*server*/, struct mg_connection* /*conn*/)
			{
				return true;
			}
		};


		class AssetHotloadHandler : public CivetHandler
		{
			ThreadSafeQueue<String>& queue;

		public:

			AssetHotloadHandler(ThreadSafeQueue<String>& command_queue) : queue(command_queue)
			{}

			virtual bool handlePut(CivetServer* /*server*/, struct mg_connection* conn)
			{
//				const struct mg_request_info* request = mg_get_request_info(conn);
				String buf;

				buf.resize(1024);
				int bytes = mg_read(conn, &buf[0], 1024);
				assert(bytes < 1024);

				// send a response that we received it.
				String output = "204 No Content";
				mg_write(conn, &output[0], output.length());


				LOGV("reload: %s\n", buf.c_str());

				Json::Reader reader;
				Json::Value root;

				bool success = reader.parse(&buf[0], &buf[0] + buf.size(), root);
				if (!success)
				{
					LOGW("ignored reload request: failed to parse json\n");
					LOGW("json parsing failed: %s\n", reader.getFormatedErrorMessages().c_str());
				}
				else
				{
					if (!root["resource"].isNull())
					{
						StackString<1024> filename = root["resource"].asString().c_str();
						StackString<1024> dirname = filename.dirname();
						LOGV("dir: %s, file: %s\n", dirname(), filename.basename().remove_extension()());
						queue.enqueue(filename.remove_extension()());
					}
				}

				return true;
			}
		};

		static int log_message(const struct mg_connection *conn, const char *message)
		{
			(void) conn;
			printf("%s\n", message);
			return 0;
		}

#endif

		void process_reload_queue()
		{
			// process queue
			size_t queued_items = _internal::reload_queue.size();
			if (queued_items > 0)
			{
				for(size_t i = 0; i < queued_items; ++i)
				{
					String item = _internal::reload_queue.dequeue();
					// for now, assume everything is a shader.
					//				LOGV("processing: %s\n", item.c_str());
					StackString<512> relative_path = item.c_str();

					// get the basename to lookup asset library.
					String dirname = relative_path.dirname()();

					// TODO: replace this with a better mechanism
					if (dirname == "shaders")
					{
						shader_load(item.c_str(), true);
					}
					else if (dirname == "models")
					{
						LOGV("model reloading is not complete.\n");
						// clear the scene
	//					root->clear();

						// force mesh reload
	//					assets::Mesh* mesh = assets::meshes()->load_from_path(item.c_str(), assets::AssetParameters(), true);

						//add_mesh_to_root(root, item.c_str(), false);
					}
					else if (dirname == "materials")
					{
						material_load(item.c_str(), true);
					}
					else
					{
						LOGW("Reload is not supported for assets in \"%s\"\n", dirname.c_str());
					}
				}
			}
		}


		void on_asset_reload(uint32_t channel, void* data, uint32_t data_size)
		{
			LOGV("on_asset_reload: channel = %i, data: %x, data_size = %i\n",
				channel,
				data,
				data_size
			);

			LOGV("char data: %s\n", (const char*)data);
		}

		void startup(gemini::Allocator& allocator, NotifyMessageDelegate message_delegate)
		{
			//struct mg_callbacks cb;
			//memset(&cb, 0, sizeof(mg_callbacks));
			//cb.log_message = log_message;

			//const char* options[] = {
			//	"listening_ports", "1983",
			//	"request_timeout_ms", "10",
			//	0
			//};

			_internal::allocator = &allocator;
			//_internal::server = MEMORY2_NEW(allocator, CivetServer) (options, &cb);
			//_internal::server->addHandler("/json", new JsonConfigHandler());
			//_internal::server->addHandler("/reload", new AssetHotloadHandler(_internal::reload_queue));

			assert(_hotloading_state == nullptr);
			_hotloading_state = MEMORY2_NEW(allocator, HotloadingState);

			notify_client_create(&_hotloading_state->notify_client);

			//_hotloading_state->channel_delegate.bind<on_asset_reload>();
			notify_client_subscribe(&_hotloading_state->notify_client, 1, message_delegate);
			notify_client_subscribe(&_hotloading_state->notify_client, 2, message_delegate);
			notify_client_subscribe(&_hotloading_state->notify_client, 3, message_delegate);
			notify_client_subscribe(&_hotloading_state->notify_client, 4, message_delegate);
			notify_client_subscribe(&_hotloading_state->notify_client, 5, message_delegate);
		}

		void shutdown()
		{
			notify_client_destroy(&_hotloading_state->notify_client);
			MEMORY2_DELETE(*_internal::allocator, _hotloading_state);

			//MEMORY2_DELETE(*_internal::allocator, _internal::server);
		}

		void tick()
		{
			//if (_internal::server)
			//{
			//	process_reload_queue();
			//}

			notify_client_tick(&_hotloading_state->notify_client);
		}
	} // namespace hotloading
} // namespace gemini
