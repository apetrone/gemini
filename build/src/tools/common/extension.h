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
#pragma once

#include <core/datastream.h>
#include <core/str.h>

#include <string>
#include <map>

namespace gemini
{
	namespace tools
	{

		// Base class for plugins
		class Plugin
		{
		public:
		
			struct Info
			{
				enum Flags
				{
					None,
					IsLoaded
				};
			
				String path;
				uint32_t flags;
			};
		
			virtual ~Plugin() {}
		};
		
	#define DECLARE_PLUGIN_CLASS(classname) \
		public: static classname* plugin_create() { return MEMORY_NEW(classname, core::memory::global_allocator()); }



		// interface for reader extensions
		template <class Type>
		class Reader : public Plugin
		{
		public:
			Reader() {}
			virtual ~Reader() {}
			
			virtual bool read(Type* model, core::util::DataStream& data) = 0;
		};
		
		// interface for writer extensions
		template <class Type>
		class Writer : public Plugin
		{
		public:
			Writer() {}
			virtual ~Writer() {}
			
			
			// abs_base_path: The absolute base path filename where the writer
			// should output a target asset. Usually, the writer will append
			// a file extension onto this base path and then open this file
			// for writing.
			virtual bool write(const std::string& abs_base_path, Type* model) = 0;
		};
		
		// generic 'extension' data; subject to change
		template <class Type>
		struct Extension
		{
			Reader<Type>* reader;
			Writer<Type>* writer;
			
			Extension() : reader(nullptr), writer(nullptr) {}
		};
		
		// registry to keep track of extensions via file path extension
		template <class Type>
		struct ExtensionRegistry
		{
			typedef std::map<std::string, Extension<Type> > ExtensionMap;
			
			static ExtensionMap extensions;
		};
		
		template <class Type>
		typename ExtensionRegistry<Type>::ExtensionMap ExtensionRegistry<Type>::extensions;
		
		// map a file path extension to an extension datamodel type
		template <class Type>
		void register_extension(const std::string& extension, const Extension<Type> & ptr)
		{
			ExtensionRegistry<Type>::extensions.insert(std::pair<std::string, Extension<Type> >(extension, ptr));
		}
		
		// purge a type registry
		template <class Type>
		void purge_registry()
		{
			for (auto data : ExtensionRegistry<Type>::extensions)
			{
				Extension<Type>& ext = data.second;
				if (ext.reader)
				{
					MEMORY_DELETE(ext.reader, core::memory::global_allocator());
				}
				
				if (ext.writer)
				{
					MEMORY_DELETE(ext.writer, core::memory::global_allocator());
				}
			}
		}
		
		// get the extension struct for a file path extension
		template <class Type>
		const Extension<Type> find_entry_for_extension(const std::string& target_extension)
		{
			for (auto v : ExtensionRegistry<Type>::extensions)
			{
				const Extension<Type>& archiver_extension = v.second;
				
				if (target_extension == v.first)
				{
					return archiver_extension;
				}
			}
			
			return Extension<Type>();
		}
	} // namespace tools
} // namespace gemini