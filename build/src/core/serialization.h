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
#pragma once


#include <core/logging.h>
#include <core/typespec.h>

namespace gemini
{
	template <bool, class T, class F>
	struct If;

	template <class T, class F>
	struct If<false, T, F>
	{
		typedef F type;
	};

	template <class T, class F>
	struct If<true, T, F>
	{
		typedef T type;
	};
} // namespace gemini

// Static Asserts cannot be used for the default / fallback templates
// because GCC/Clang will compile all templates regardless if they're in-use
// or not.


// This is used to set the serializer dispatch method for a type
#define SERIALIZER_SET_DISPATCH(C, type) \
	template <>\
	struct SerializerDispatchType<C>\
	{\
		static constexpr SerializerType value = type;\
	}

// SerializerTypes
enum SerializerType
{
	SerializerType_UNDEFINED,
	SerializerType_INTERNAL,
	SerializerType_EXTERNAL,
	SerializerType_POD
};

template <class T>
struct SerializerDispatchType
{
	static constexpr SerializerType value = SerializerType_UNDEFINED;
};

namespace gemini
{
	// This is used in external serializations to define members.
	#define MAKE_INSTANCE_MEMBER(instance, name) make_field_key_value_pair(#name, instance.name, member_offset(&instance, &instance.name))

	// This is used in internal serializers to define members.
	#define MAKE_MEMBER(name) make_field_key_value_pair(#name, name, member_offset(this, &name))

	// This is used in internal serializers to define an alias to a member variable.
	#define MAKE_MEMBER_ALIAS(variable, name) make_field_key_value_pair(#name, variable, member_offset(this, &variable))

	template <class T>
	struct FieldKeyValuePair
	{
		const char* name;
		T& value;
		intptr_t offset;

		FieldKeyValuePair(const char* _name, T&& _value, intptr_t member_offset)
			: name(_name)
			, value(_value)
			, offset(member_offset)
		{
		}

	private:
		FieldKeyValuePair & operator=(FieldKeyValuePair const &) = delete;
	}; // FieldKeyValuePair


	template <class T>
	FieldKeyValuePair<T> make_field_key_value_pair(const char* name, T& value, intptr_t member_offset)
	{
		return FieldKeyValuePair<T>(name, std::forward<T>(value), member_offset);
	}

	// An internal Serializer example
	/*
		template <class Archive>
		void serialize(Archive& archive)
		{
			archive & MAKE_MEMBER(label_margin);
			archive & MAKE_MEMBER(background_color);
			archive & MAKE_MEMBER(foreground_color);
			archive & MAKE_MEMBER(interpolation);
			archive & MAKE_MEMBER(step_value);
			archive & MAKE_MEMBER(another_val);
			archive & MAKE_MEMBER(font_name);
		}
	*/

	// An external Serializer example
	/*
		template <class Archive>
		void serialize(Archive& archive, StyleTest& instance)
		{
			archive & MAKE_INSTANCE_MEMBER(instance, label_margin);
			archive & MAKE_INSTANCE_MEMBER(instance, background_color);
			archive & MAKE_INSTANCE_MEMBER(instance, foreground_color);
		}
	*/

	template <class T, class X>
	intptr_t member_offset(T* instance, X* member)
	{
		return (char*)member - (char*)instance;
	}


	template <class T>
	struct SerializeDispatcherUndefined
	{
		template <class Archive>
		void dispatch(Archive& archive, FieldKeyValuePair<T>& pair)
		{
			// If you hit this,
			// T is being serialized with no defined serializer type.
			assert(0);
		}
	};

	template <class T>
	struct SerializeDispatcherInternal
	{
		template <class Archive>
		void dispatch(Archive& archive, FieldKeyValuePair<T>& pair)
		{
			LOGV("internal dispatch; type is '%s'\n", TypeSpecName<T>::value);
			pair.value.serialize(archive);
		}
	};

	template <class T>
	struct SerializeDispatcherExternal
	{
		template <class Archive>
		void dispatch(Archive& archive, FieldKeyValuePair<T>& pair)
		{
			LOGV("external dispatch\n");
			serialize(archive, pair.value);
		}
	};

	template <class T>
	struct SerializeDispatcherPOD
	{
		template <class Archive>
		void dispatch(Archive& archive, FieldKeyValuePair<T>& pair)
		{
			LOGV("POD dispatch; type is '%s', size is %i\n", typespec_name_from_value<T>(&pair.value), TypeSpecSize<T>::value);
			if (archive.IsSaving)
			{
				archive.save(pair.value);
			}
			else if (archive.IsLoading)
			{
				archive.load(pair.value);
			}
		}
	};

	template <class Archive, class T>
	void SerializeDispatcher(Archive& archive, FieldKeyValuePair<T>& pair)
	{
		typename If <!typespec_is_class<T>::value, // If T is not a class...
			SerializeDispatcherPOD<T>, // use POD dispatcher

			// otherwise if T uses internal serializer
			typename If <SerializerDispatchType<T>::value == SerializerType_INTERNAL,
				// dispatcher is internal
				SerializeDispatcherInternal<T>,

				// else if T uses external serializer
				typename If <SerializerDispatchType<T>::value == SerializerType_EXTERNAL,
					// dispatcher is external
					SerializeDispatcherExternal<T>,
					// else, serializer is undefined.

					SerializeDispatcherUndefined<T>>::type >::type>::type dispatcher;

		dispatcher.dispatch(archive, pair);
	} // SerializeDispatcher


	template <class Archive, class T>
	void serialize(Archive& archive, T& instance)
	{
		// If you hit this, T was defined with an external serializer
		// but a specialization for this function could not be found for T.
		assert(0);
	}

} // namespace gemini

#include <core/serialization/archive.h>
#include <core/serialization/fieldcollector.h>
#include <core/serialization/keyvaluearchive.h>