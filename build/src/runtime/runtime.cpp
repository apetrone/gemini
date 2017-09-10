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

#include <platform/platform.h>
#include <platform/window.h>

#include <renderer/renderer.h>

#include "filesystem_interface.h"

#include <rapid/rapid.h>



using platform::PathString;
using namespace core;
using namespace platform;

// Uncomment this to display runtime info from runtime.
//#define GEMINI_RUNTIME_DEBUG

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


	}

	// initialize filesystem

	// set paths

	// initialize handlers

	platform::Result runtime_startup(const char* application_data_path,
		platform::PathString content_path,
		std::function<platform::PathString(const char*)> get_application_directory,
		uint32_t runtime_flags)
	{
		detail::_state->allocator = memory_allocator_default(MEMORY_ZONE_RUNTIME);
		detail::_state->render_device = nullptr;

		platform::PathString root_path = platform::get_program_directory();
#if defined(GEMINI_RUNTIME_DEBUG)
		LOGV("root_path: %s\n", root_path());
#endif
		if (content_path.is_empty())
		{
			content_path = platform::fs_content_directory();
		}

#if defined(GEMINI_RUNTIME_DEBUG)
		LOGV("content_path: %s\n", content_path());
#endif

		assert(core::filesystem::instance() == nullptr);

		// create file system instance
		core::filesystem::IFileSystem* filesystem = MEMORY2_NEW(detail::_state->allocator, core::filesystem::FileSystemInterface);
		core::filesystem::set_instance(filesystem);
		if (!filesystem)
		{
			return platform::Result::failure("Unable to create filesystem instance");
		}

		// default path setup
		filesystem->root_directory(root_path);
		filesystem->content_directory(content_path);
		filesystem->virtual_add_root(root_path());
		filesystem->virtual_add_root(content_path());

		// See if the user application directory must be overridden.
		platform::PathString user_application_path;
		if (get_application_directory)
		{
			user_application_path = get_application_directory(application_data_path);
		}
		else
		{
			user_application_path = platform::get_user_application_directory(application_data_path);;
		}
		filesystem->user_application_directory(user_application_path);

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

		if (runtime_flags & RF_WINDOW_SYSTEM)
		{
			// initialize window subsystem
			platform::window::startup(platform::window::RenderingBackend_Default);
		}

		// asset startup

		runtime_load_rapid();

		return platform::Result::success();
	}

	void runtime_shutdown()
	{
		// asset shutdown

		runtime_unload_rapid();

		core::filesystem::instance()->shutdown();

		core::filesystem::IFileSystem* filesystem = core::filesystem::instance();
		MEMORY2_DELETE(detail::_state->allocator, filesystem);
	} // shutdown

	void runtime_load_arguments(gemini::Allocator& allocator, Array<gemini::string>& arguments)
	{
		const platform::MainParameters& mainparams = platform::get_mainparameters();
#if defined(PLATFORM_LINUX) || defined(PLATFORM_APPLE)
		string_tokenize_commandline(allocator, arguments, mainparams.argc, mainparams.argv);
#elif defined(PLATFORM_WINDOWS)
		string_tokenize_commandline(allocator, arguments, mainparams.commandline);
#else
		#error Not implemented on this platform!
#endif
	} // runtime_load_arguments

	void runtime_destroy_arguments(gemini::Allocator& allocator, Array<gemini::string>& arguments)
	{
		for (size_t index = 0; index < arguments.size(); ++index)
		{
			string_destroy(allocator, arguments[index]);
		}
		arguments.clear();
	} // runtime_destroy_arguments

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
















	void notify_client_create(NotificationClient* client)
	{
		memset(client, 0, sizeof(NotificationClient));

		// create a broadcast socket
		client->broadcast_socket = net_socket_open(net_socket_type::UDP);
		assert(net_socket_is_valid(client->broadcast_socket));

		net_address bind_address;
		net_address_init(&bind_address);
		net_address_set(&bind_address, "0.0.0.0", NOTIFY_COMMUNICATION_PORT);

		net_socket_set_broadcast(client->broadcast_socket, 1);

		int32_t bind_result = net_socket_bind(client->broadcast_socket, &bind_address);
		assert(bind_result == 0);
	}

	void notify_client_destroy(NotificationClient* client)
	{
		net_socket_close(client->broadcast_socket);
		net_socket_close(client->communication_socket);
	}

	void notify_client_tick(NotificationClient* client)
	{
		uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;
		if (client->state == NCS_DISCONNECTING)
		{
			return;
		}

		memset(client->scratch_memory, 0, 256);
		client->scratch_allocator = memory_allocator_linear(MEMORY_ZONE_DEFAULT, client->scratch_memory, 256);

		if (client->state == NCS_BROADCASTING)
		{
			// broadcast message to find a server
			if (runtime_msec_assign_if_timedout(client->last_broadcast_msec, NOTIFY_BROADCAST_DELAY_MSEC))
			{
				// broadcast again
				net_address broadcast_address;
				net_address_init(&broadcast_address);
				net_address_set(&broadcast_address, "255.255.255.255", NOTIFY_BROADCAST_PORT);

				uint32_t value = NMT_BROADCAST_REQUEST;
				NotifyMessage broadcast_message;
				broadcast_message.channel = 0;
				broadcast_message.type = NMT_BROADCAST_REQUEST;
				broadcast_message.data = &value;
				broadcast_message.data_size = sizeof(uint32_t);

				net_socket_sendto(client->broadcast_socket, &broadcast_address, static_cast<const char*>(broadcast_message.data), broadcast_message.data_size);
			}
		}
		else if (client->state == NCS_CONNECTED)
		{
			if (runtime_msec_assign_if_timedout(client->last_heartbeat_msec, NOTIFY_HEARTBEAT_MSEC))
			{
				// send a heartbeat to the server
				NotifyMessage message;
				message.channel = 0;
				message.type = NMT_HEARTBEAT;
				message.data = &message.type;
				message.data_size = sizeof(message.type);
				notify_message_send(client->communication_socket, &message);
			}

			if (runtime_msec_assign_if_timedout(client->last_server_heartbeat_msec, NOTIFY_HEARTBEAT_MSEC * NOTIFY_CLIENT_TIMEOUT_MAX))
			{
				net_socket_close(client->communication_socket);
				client->communication_socket = NET_SOCKET_INVALID;
				client->state = NCS_BROADCASTING;
				client->last_broadcast_msec = current_milliseconds - NOTIFY_BROADCAST_DELAY_MSEC;
				client->last_heartbeat_msec = current_milliseconds;
				client->last_server_heartbeat_msec = 0;
			}
		}

		net_socket sock_max = client->broadcast_socket;

		// see if the communication socket has any data
		fd_set receive;
		FD_ZERO(&receive);
		FD_SET(client->broadcast_socket, &receive);

		if (net_socket_is_valid(client->communication_socket))
		{
			FD_SET(client->communication_socket, &receive);
			if (sock_max < client->communication_socket)
			{
				sock_max = client->communication_socket;
			}
		}

		timeval zero_timeval;
		zero_timeval.tv_sec = 0;
		zero_timeval.tv_usec = 1;

		int select_result = select(sock_max + 1, &receive, nullptr, nullptr, &zero_timeval);
		assert(select_result >= 0);

		if (FD_ISSET(client->broadcast_socket, &receive))
		{
			int32_t command = 0;
			net_address source;
			int32_t bytes_read = net_socket_recvfrom(client->broadcast_socket, &source, (char*)&command, sizeof(int32_t));

			if (command == NMT_BROADCAST_CONFIRM)
			{
				net_address_port(&source, NOTIFY_COMMUNICATION_PORT);

				// create a communication socket
				client->communication_socket = net_socket_open(net_socket_type::TCP);
				assert(net_socket_is_valid(client->communication_socket));

				int32_t result = net_socket_connect(client->communication_socket, &source);
				if (result == 0)
				{
					client->state = NCS_CONNECTED;
					client->last_server_heartbeat_msec = platform::microseconds() * MillisecondsPerMicrosecond;

					NotifyMessage message;
					message.channel = 0;
					notify_message_subscribe(client->scratch_allocator, message, client->subscribe_flags);
					notify_message_send(client->communication_socket, &message);
				}
				else
				{
					LOGE("Unable to connect to server!\n");
				}
			}
			else
			{
				LOGW("Unrecognized command: %i\n", command);
			}
		}
		else if (FD_ISSET(client->communication_socket, &receive))
		{
			NotifyMessage msg;
			int32_t bytes_read = notify_message_read(client->scratch_allocator, client->communication_socket, &msg);
			if (bytes_read > 0)
			{
				if (msg.type == NMT_HEARTBEAT)
				{
					client->last_server_heartbeat_msec = platform::microseconds() * MillisecondsPerMicrosecond;
				}
				else if (msg.type == NMT_STRING)
				{
					if (client->channel_delegate.is_valid())
					{
						client->channel_delegate(msg.channel, msg.data, msg.data_size);
					}
					else
					{
						LOGW("Received message; but channel delegate is invalid\n");
					}
				}
				else
				{
					LOGV("Unhandled message from server; received: %i\n", msg.type);
				}
			}
		}
	}

	void notify_client_subscribe(NotificationClient* client, uint32_t channel, NotifyMessageDelegate delegate)
	{
		// Channel zero is reserved.
		assert(channel > 0);
		client->channel_delegate = delegate;
		client->subscribe_flags |= (1 << channel);
	}


	int32_t notify_message_read(Allocator& allocator, platform::net_socket source, NotifyMessage* message)
	{
		const size_t BUFFER_MAX_SIZE = 256;
		char buffer[BUFFER_MAX_SIZE] = { 0 };

		int32_t bytes_read = net_socket_recv(source, buffer, BUFFER_MAX_SIZE);
		if (bytes_read > 0)
		{
			core::util::MemoryStream stream;
			stream.init(buffer, BUFFER_MAX_SIZE);
			stream.read(message->type);
			stream.read(message->channel);
			stream.read(message->data_size);

			message->data = MEMORY2_ALLOC(allocator, message->data_size);
			assert(message->data);
			stream.read(message->data, message->data_size);
		}

		return bytes_read;
	}

	int32_t notify_message_send(net_socket destination, const NotifyMessage* message)
	{
		char buffer[256] = { 0 };
		core::util::MemoryStream stream;
		stream.init(buffer, 256);
		stream.write(message->type);
		stream.write(message->channel);
		stream.write(message->data_size);
		stream.write(message->data, message->data_size);
		return net_socket_send(destination, buffer, stream.current_offset());
	}

	void notify_message_string(NotifyMessage& message, const platform::PathString& path)
	{
		message.type = NMT_STRING;
		message.data = (void*)&path[0];
		message.data_size = path.size();
	}

	void notify_message_subscribe(Allocator& allocator, NotifyMessage& message, uint8_t flags)
	{
		message.type = NMT_SUBSCRIBE;
		message.data = MEMORY2_ALLOC(allocator, sizeof(flags));
		memcpy(message.data, &flags, sizeof(uint8_t));
		message.data_size = sizeof(uint8_t);
	}


	net_socket notify_server_accept(net_socket communication_socket, net_address* source)
	{
		// try to accept
		net_socket sock;
		for (;;)
		{
			sock = net_socket_accept(communication_socket, source);
			if (sock == -1)
			{
				if (errno == EINTR)
				{
					// interrupted function. sucks.
					continue;
				}
				else
				{
					LOGW("accept failed: %i\n", errno);
					return NET_SOCKET_INVALID;
				}
			}
			break;
		}

		return sock;
	}

	void notify_server_create(NotificationServer* server)
	{
		memset(server, 0, sizeof(NotificationServer));

		// create a broadcast socket
		server->broadcast_socket = net_socket_open(net_socket_type::UDP);
		assert(net_socket_is_valid(server->broadcast_socket));

		net_address bind_address;
		net_address_init(&bind_address);
		net_address_set(&bind_address, "0.0.0.0", NOTIFY_BROADCAST_PORT);

		int32_t bind_result = net_socket_bind(server->broadcast_socket, &bind_address);
		assert(bind_result == 0);

		server->communication_socket = net_socket_open(net_socket_type::TCP);
		assert(net_socket_is_valid(server->communication_socket));

		net_address com_address;
		net_address_init(&com_address);
		net_address_set(&com_address, "0.0.0.0", NOTIFY_COMMUNICATION_PORT);

		net_socket_bind(server->communication_socket, &com_address);
		net_socket_listen(server->communication_socket, 2);
	}

	void notify_server_destroy(NotificationServer* server)
	{
		net_socket_close(server->broadcast_socket);
		net_socket_close(server->communication_socket);

		// destroy all clients

		NotificationServerSideClient* client = server->clients;
		while (client)
		{
			NotificationServerSideClient* temp = client;
			client = client->next;

			MEMORY2_DELETE(detail::_state->allocator, temp);
		}

		server->clients = nullptr;
	}

	void notify_server_handle_broadcast_event(NotificationServer* server)
	{
		net_address source;
		const size_t PACKET_SIZE = 128;
		char packet_buffer[PACKET_SIZE] = { 0 };
		int32_t bytes_read = net_socket_recvfrom(server->broadcast_socket, &source, packet_buffer, PACKET_SIZE);
		if (bytes_read)
		{
			core::util::MemoryStream stream;
			stream.init(packet_buffer, PACKET_SIZE);

			int32_t broadcast_query_value = 0;
			stream.read(broadcast_query_value);

			if (broadcast_query_value == NMT_BROADCAST_REQUEST)
			{
				// client is looking for a server; reply with confirmation
				int32_t reply_value = NMT_BROADCAST_CONFIRM;
				net_socket_sendto(server->broadcast_socket, &source, (const char*)&reply_value, sizeof(int32_t));
			}
		}
	}

	void notify_server_handle_event(NotificationServer* server, NotificationServerSideClient* client)
	{
		memset(server->scratch_memory, 0, 256);
		Allocator allocator = memory_allocator_linear(MEMORY_ZONE_DEFAULT, server->scratch_memory, 256);

		NotifyMessage message;
		int32_t bytes_read = notify_message_read(allocator, client->csock, &message);
		if (bytes_read > 0)
		{
			if (message.type == NMT_HEARTBEAT)
			{
				client->last_heartbeat_msec = platform::microseconds() * MillisecondsPerMicrosecond;
			}
			else if (message.type == NMT_SUBSCRIBE)
			{
				memcpy(&client->subscribed_channels, message.data, message.data_size);
			}
		}
		// else: value is probably -1 on a non-blocking socket.
	}

	void notify_server_tick(NotificationServer* server)
	{
		timeval zero_timeval;
		zero_timeval.tv_sec = 0;
		zero_timeval.tv_usec = 1;

		fd_set receive;
		FD_ZERO(&receive);
		FD_SET(server->broadcast_socket, &receive);
		FD_SET(server->communication_socket, &receive);


		net_socket sock_max = server->communication_socket;

		// iterate over clients and add their fds
		NotificationServerSideClient* client = server->clients;
		while (client)
		{
			FD_SET(client->csock, &receive);
			if (client->csock > sock_max)
			{
				sock_max = client->csock;
			}

			client = client->next;
		}

		// TODO: better performance if we used epoll on linux.
		int select_result = select(sock_max + 1,
			&receive,
			nullptr,
			nullptr,
			&zero_timeval);
		assert(select_result >= 0);

		if (FD_ISSET(server->broadcast_socket, &receive))
		{
			notify_server_handle_broadcast_event(server);
		}
		else if (FD_ISSET(server->communication_socket, &receive))
		{
			net_address source;
			net_socket sock = notify_server_accept(server->communication_socket, &source);
			if (net_socket_is_valid(sock))
			{
				char buffer[32] = { 0 };
				net_address_host(&source, buffer, 32);

				// create a client
				NotificationServerSideClient* new_client = MEMORY2_NEW(detail::_state->allocator, NotificationServerSideClient);
				new_client->address = source;
				new_client->csock = sock;
				new_client->flags = 0;
				new_client->subscribed_channels = 0;
				new_client->last_heartbeat_msec = platform::microseconds() * MillisecondsPerMicrosecond;
				new_client->next = server->clients;
				server->clients = new_client;
			}
		}

		// loop through all clients, handle reads, and when due, send a heartbeat
		if (server->clients)
		{
			bool send_heartbeat = runtime_msec_assign_if_timedout(server->next_heartbeat_msec, NOTIFY_HEARTBEAT_MSEC);
			client = server->clients;
			while (client)
			{
				if (client->flags & NSCF_DELETE_NEXT_TICK)
				{
					client = client->next;
					continue;
				}

				if (FD_ISSET(client->csock, &receive))
				{
					// handle reads for this client socket
					notify_server_handle_event(server, client);
				}

				if (send_heartbeat)
				{
					uint64_t client_last_heartbeat_msec = client->last_heartbeat_msec;
					if (runtime_msec_assign_if_timedout(client_last_heartbeat_msec, NOTIFY_CLIENT_TIMEOUT_MAX * NOTIFY_HEARTBEAT_MSEC))
					{
						client->flags |= NSCF_DELETE_NEXT_TICK;
						client = client->next;
						continue;
					}

					NotifyMessage message;
					message.type = NMT_HEARTBEAT;
					message.channel = 0;
					message.data = &message.type;
					message.data_size = sizeof(message.type);
					notify_message_send(client->csock, &message);
				}
				client = client->next;
			}
		}

		// trim deleted clients
		client = server->clients;
		NotificationServerSideClient* prev = client;
		while (client)
		{
			if (client->flags & NSCF_DELETE_NEXT_TICK)
			{
				if (prev == client)
				{
					NotificationServerSideClient* next = client->next;
					server->clients = next;
					prev = next;
					MEMORY2_DELETE(detail::_state->allocator, client);
					client = next;
					continue;
				}
				else
				{
					NotificationServerSideClient* next = client->next;
					prev->next = next;
					MEMORY2_DELETE(detail::_state->allocator, client);
					client = next;
					continue;
				}
			}

			prev = client;
			client = client->next;
		}
	}

	void notify_server_publish(NotificationServer* server, const NotifyMessage* message)
	{
		// This should really queue up the publish instead of directly sending it.
		NotificationServerSideClient* client = server->clients;
		while (client)
		{
			if (client->subscribed_channels & (1 << message->channel))
			{
				notify_message_send(client->csock, message);
			}
			client = client->next;
		}
	}

	bool runtime_msec_assign_if_timedout(uint64_t& target_msec, uint32_t timeout_msec)
	{
		uint64_t current_milliseconds = platform::microseconds() * MillisecondsPerMicrosecond;
		assert(current_milliseconds >= target_msec);
		if ((current_milliseconds - target_msec) > timeout_msec)
		{
			target_msec = current_milliseconds;
			return true;
		}

		return false;
	} // runtime_msec_assign_if_timedout

	static RapidInterface rapid;
	static platform::DynamicLibrary* rapid_library = nullptr;

	void runtime_load_rapid()
	{
		if (rapid_library)
		{
			runtime_unload_rapid();
		}

		PathString program_directory = platform::get_program_directory();
		PathString lib_directory = program_directory;
		core::str::directory_up(&lib_directory[0]);
		core::str::directory_up(&lib_directory[0]);
		lib_directory.append(PATH_SEPARATOR_STRING);
		lib_directory.append("lib");
		lib_directory.append(PATH_SEPARATOR_STRING);
		lib_directory.append("debug_x86_64");
		lib_directory.append(PATH_SEPARATOR_STRING);
#if defined(PLATFORM_LINUX)
		lib_directory.append("lib");
#endif
		lib_directory.append("rapid");
		lib_directory.append(platform::dylib_extension());

#if defined(GEMINI_RUNTIME_DEBUG)
		LOGV("rapid lib = %s\n", lib_directory());
#endif
		rapid_library = platform::dylib_open(lib_directory());
		assert(rapid_library);

		populate_interface_fn pif = reinterpret_cast<populate_interface_fn>(platform::dylib_find(rapid_library, "populate_interface"));
		assert(pif);

		pif(rapid);
	}

	void runtime_unload_rapid()
	{
		if (rapid_library)
		{
			memset(&rapid, 0, sizeof(RapidInterface));
			platform::dylib_close(rapid_library);
			rapid_library = nullptr;
		}
	}

	RapidInterface* runtime_rapid()
	{
		if (!rapid_library)
		{
			return nullptr;
		}

		return &rapid;
	}

	platform::PathString runtime_platform_asset_root(const platform::PathString& root)
	{
		// dev builds (passed by -game) are located at:
		// "<game_path>/builds/<PLATFORM_NAME>"
		platform::PathString content_path = root;
		content_path.append(PATH_SEPARATOR_STRING);
		content_path.append("builds");
		content_path.append(PATH_SEPARATOR_STRING);
		content_path.append(PLATFORM_NAME);
		return content_path;
	} // runtime_platform_asset_root
} // namespace gemini
