// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone

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

#include <slim/xlog.h>
#include <slim/xstr.h>

#include <core/typedefs.h>
#include <core/mathlib.h>
#include <core/threadsafequeue.h>
#include <core/stackstring.h>

#include <json/json.h>

#include "hotloading.h"
#include "assets/asset_material.h"
#include "assets/asset_shader.h"

#include "civetweb.h"
#include "CivetServer.h"




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
		debugdraw::text(x, y, xstr_format("[VAR] %s = %s", current->name.c_str(), current->value_string().c_str()), Color(255, 255, 255, 255));
		
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

// not sure what else to call this...
namespace hotloading
{
	namespace _internal
	{
		CivetServer* server = nullptr;
		ThreadSafeQueue<String> reload_queue;
	}
	

	class JsonConfigHandler : public CivetHandler
	{
		virtual bool handlePut(CivetServer* server, struct mg_connection* conn)
		{
			const struct mg_request_info* request = mg_get_request_info(conn);
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
		
		virtual bool handleDelete(CivetServer* server, struct mg_connection* conn)
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
		
		virtual bool handlePut(CivetServer* server, struct mg_connection* conn)
		{
			const struct mg_request_info* request = mg_get_request_info(conn);
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
					assets::shaders()->load_from_path(item.c_str(), assets::AssetParameters(), true);
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
					assets::materials()->load_from_path(item.c_str(), assets::AssetParameters(), true);
				}
				else
				{
					LOGW("Reload is not supported for assets in \"%s\"\n", dirname.c_str());
				}
			}
		}
	}



	
	void startup()
	{
		struct mg_callbacks cb;
		memset(&cb, 0, sizeof(mg_callbacks));
		cb.log_message = log_message;
		
		const char* options[] = {
			"listening_ports", "1983",
			"request_timeout_ms", "10",
			0
		};
		
		_internal::server = CREATE(CivetServer, options, &cb);
		_internal::server->addHandler("/json", new JsonConfigHandler());
		_internal::server->addHandler("/reload", new AssetHotloadHandler(_internal::reload_queue));
	}

	void shutdown()
	{
		DESTROY(CivetServer, _internal::server);
	}
	
	void tick()
	{
		if (_internal::server)
		{
			process_reload_queue();
		}
	}
}
