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
#pragma once

#include <string>
#include <map>

#include <core/datastream.h>
#include <core/str.h>

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
		public: static classname* plugin_create() { return CREATE(classname); }



		// interface for reader extensions
		template <class Type>
		class Reader : public Plugin
		{
		public:
			Reader() {}
			virtual ~Reader() {}
			
			virtual void read(Type* model, util::DataStream& data) = 0;
		};
		
		// interface for writer extensions
		template <class Type>
		class Writer : public Plugin
		{
		public:
			Writer() {}
			virtual ~Writer() {}
			
			virtual void write(Type* model, util::DataStream& data) = 0;
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
					DESTROY(Reader<Type>, ext.reader);
				}
				
				if (ext.writer)
				{
					DESTROY(Writer<Type>, ext.writer);
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