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
#pragma once

#include "typedefs.h"
#include "mem.h"

namespace reflection
{
	// ---------------------------------------------------------------------
	// macros
	// ---------------------------------------------------------------------
#define TYPEINFO_DECLARE_CLASS(T)\
	protected:\
		reflection::TypeInfo* type_info;\
	public:\
		typedef T ThisClass;\
		virtual const reflection::TypeInfo* get_type_info();

#define TYPEINFO_REGISTER_TYPE_NAME(T, N)\
	namespace reflection\
	{\
		template <>\
		struct TypeIdentifier<T>\
		{\
			static const char* get_type_identifier()\
			{\
				return #N;\
			}\
		};\
	}

#define TYPEINFO_REGISTER_TYPE_INFO(T)\
	const reflection::TypeInfo* T::get_type_info()\
	{\
		return reflection::get_type_info<T>();\
	}\
	reflection::TypeRegistrar T##_registrar(#T, reflection::get_type_info<T>());

#define TYPEINFO_REGISTER_TYPE_BASE(T, B)\
	namespace reflection\
	{\
		template <>\
		const TypeInfo* get_base_type<T>()\
		{\
			return get_type_info<B>();\
		}\
	}

#define TYPEINFO_PROPERTY(T)\
	reflection::make_class_property(#T, &T)

	// ---------------------------------------------------------------------
	// TypeInfo
	// ---------------------------------------------------------------------
	class TypeInfo
	{
	public:
		virtual const char* type_identifier() const = 0;
		virtual bool is_same_type(const TypeInfo* other) const = 0;
		virtual bool is_same_type(const char* identifier) const = 0;
		virtual const TypeInfo* get_base_type() const = 0;
	};

	template <class T>
	struct TypeIdentifier
	{
		static const char* get_type_identifier()
		{
			// This assert is hit if you forget to register the type name.
			// See TYPEINFO_REGISTER_TYPE_NAME
			assert(0);
			return "UnknownType";
		}
	};

	template <class T>
	const TypeInfo* get_base_type()
	{
		return nullptr;
	}

	template <class T>
	class TypeInfoInstance : public TypeInfo
	{
	public:
		virtual const char* type_identifier() const
		{
			return reflection::TypeIdentifier<T>::get_type_identifier();
		}

		virtual bool is_same_type(const TypeInfo* other) const
		{
			return (this == other);
		}

		virtual bool is_same_type(const char* identifier) const
		{
			return core::str::case_insensitive_compare(identifier, type_identifier(), 0) == 0;
		}

		virtual const TypeInfo* get_base_type() const
		{
			return reflection::get_base_type<T>();
		}
	};

	template <class T>
	const TypeInfoInstance<T>* get_type_info()
	{
		static TypeInfoInstance<T> _instance;
		return &_instance;
	}

	// ---------------------------------------------------------------------
	// TypeRegistry
	// ---------------------------------------------------------------------
	struct TypeInfoLink
	{
		struct TypeInfoLink* next;
		const TypeInfo* typeinfo;
	};

	class TypeRegistry
	{
	private:
		static TypeInfoLink* root;

	public:
		const TypeInfo* get_type_info(const char* classname) const
		{
			TypeInfoLink* current = TypeRegistry::root;

			while(current)
			{
				if (core::str::case_insensitive_compare(current->typeinfo->type_identifier(), classname, 0) == 0)
					return current->typeinfo;

				current = current->next;
			}

			// typeinfo could not be found
			assert(0);
			return nullptr;
		}

		void register_info(TypeInfoLink* info)
		{
			info->next = TypeRegistry::root;
			TypeRegistry::root = info;
		}

	public:
		static TypeRegistry* instance()
		{
			static TypeRegistry _type_registry;
			return &_type_registry;
		};

		static const TypeRegistry& const_instance()
		{
			return *const_cast<const TypeRegistry*>(instance());
		}
	}; // TypeRegistry

	TypeInfoLink* TypeRegistry::root = nullptr;

	struct TypeRegistrar
	{
		TypeInfoLink node;
		TypeRegistrar(const char* type_name, const TypeInfo* info)
		{
			node.next = nullptr;
			node.typeinfo = info;
			TypeRegistry::instance()->register_info(&node);
		}
	}; // TypeRegistrar


	// ---------------------------------------------------------------------
	// ClassProperty
	// ---------------------------------------------------------------------
	template <class T>
	struct ClassProperty
	{
		const char* name;
		T* address;

		ClassProperty(
					  const char* property_name,
					  T* value) :
			name(property_name),
			address(value)
		{
		}
	};

	template <class T>
	reflection::ClassProperty<T> make_class_property(const char* name, T* value)
	{
		return reflection::ClassProperty<T>(name, value);
	}


} // namespace reflection
