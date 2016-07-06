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
#pragma once

#include <core/typedefs.h>
#include <core/str.h>

#define TYPESPEC_OFFSET(C, M) \
	::gemini::typespec_member_offset(C, &(C).M)

#define TYPESPEC_PROPERTY(P) \
	::gemini::typespec_make_class_property(#P, P, TYPESPEC_OFFSET(*this, P))

#define TYPESPEC_IDENTIFIER(T) \
	template <>\
	const char* ::gemini::typespec_type_identifier<T>::name = #T;

#define TYPESPEC_SIZE(T) \
	template <>\
	size_t typespec_size<T>::value = sizeof(T);

#define TYPESPEC_REGISTER_TYPE(T)\
	TYPESPEC_IDENTIFIER(T);\
	TYPESPEC_SIZE(T);\
	static TypeSpecTypeInfo<T> tr_##T;\

namespace gemini
{
	template <class T>
	struct typespec_type_identifier
	{
		static const char* name;
	}; // typespec_type_identifiers

	template <class T>
	struct typespec_size
	{
		static size_t value;
	};

	template <class T>
	struct TypeSpecClassProperty
	{
		const char* name;
		T& ref;
		size_t offset;

		TypeSpecClassProperty(const char* property_name,
			T& instance,
			size_t property_offset)
			: name(property_name)
			, ref(instance)
			, offset(property_offset)
		{
		}
	}; // TypeSpecClassProperty


	template <>
	struct TypeSpecClassProperty<const char*>
	{
		const char* name;
		const char* ref;
		size_t offset;

		TypeSpecClassProperty(const char* property_name,
			const char* instance,
			size_t property_offset)
			: name(property_name)
			, ref(instance)
			, offset(property_offset)
		{
		}
	}; // TypeSpecClassProperty

	template <class T>
	size_t typespec_member_offset(const T& instance, const void* member)
	{
		return \
			static_cast<const unsigned char*>(member) -
			static_cast<const unsigned char*>(static_cast<const void*>(&instance));
	}

	template <class T>
	const TypeSpecClassProperty<T> typespec_make_class_property(
		const char* name,
		const T& instance,
		size_t offset)
	{
		return TypeSpecClassProperty<T>(name, const_cast<T&>(instance), offset);
	} // TypeSpecClassProperty

	class TypeRegistry;

	class TypeInfo
	{
	protected:
		TypeInfo* next;
		friend class TypeRegistry;
	public:
		virtual ~TypeInfo() {}

		virtual const char* type_name() const = 0;

		virtual void* construct() = 0;
		virtual void destruct(void* ptr) = 0;
	};










	class TypeRegistry
	{
	public:

		static TypeInfo* tail;

		TypeRegistry()
		{
		}

		static TypeRegistry& instance()
		{
			TypeRegistry _type_registry;
			return _type_registry;
		}

		void register_type(TypeInfo* info)
		{
			info->next = tail;
			tail = info;
		}

		template <class T>
		TypeInfo* find_type()
		{
			TypeInfo* current = tail;

			while (current)
			{
				if (core::str::case_insensitive_compare(current->type_name(), typespec_type_identifier<T>::name, 0) == 0)
					return current;
				current = current->next;
			}

			return nullptr;
		} // find_type

		TypeInfo* find_type_named(const char* type_name)
		{
			TypeInfo* current = tail;

			while (current)
			{
				if (core::str::case_insensitive_compare(current->type_name(), type_name, 0) == 0)
					return current;
				current = current->next;
			}

			return nullptr;
		} // find_type_named
	}; // TypeRegistry


	template <class T>
	T* type_constructor()
	{
		return new T;
	}

	template <class T>
	void type_destructor(T* ptr)
	{
		delete ptr;
	}

	template <class T>
	class TypeSpecTypeInfo : public TypeInfo
	{
	public:
		TypeSpecTypeInfo()
		{
			TypeRegistry::instance().register_type(this);
		}

		virtual const char* type_name() const
		{
			return typespec_type_identifier<T>::name;
		}

		virtual void* construct()
		{
			return type_constructor<T>();
		}

		virtual void destruct(void* ptr)
		{
			type_destructor(ptr);
		}
	}; // TypeSpecTypeInfo


	template <class T>
	class Collector
	{
	public:

		const T& down_cast(const Collector& instance) const
		{
			return static_cast<const T&>(instance);
		}

		T& down_cast(Collector& instance)
		{
			return const_cast<T&>(down_cast(static_cast<const Collector&>(instance)));
		}

		const T& instance() const
		{
			return down_cast(*this);
		}

		T& instance()
		{
			return down_cast(*this);
		}

		template <class X>
		void operator<< (X* item)
		{
			instance().write(item);
		}

		template <class X>
		void operator<< (X& item)
		{
			this->operator<< (&item);
		}

		template <class X>
		void operator>> (X* item)
		{
			/*typespec_traits<T>::is_pod::value*/
			instance().read(item);
		}

		template <class X>
		void operator>> (X& item)
		{
			this->operator>> (&item);
		}
	}; // Collector
} // namespace gemini
