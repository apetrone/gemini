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



namespace gemini
{
	// This is used in external serializations to define members.
	#define MAKE_INSTANCE_MEMBER(instance, name) make_field_key_value_pair(#name, instance.name, member_offset(&instance, &instance.name))

	// This is used in internal serializers to define members.
	#define MAKE_MEMBER(name) make_field_key_value_pair(#name, name, member_offset(this, &name))

	// This is used in internal serializers to define an alias to a member variable.
	#define MAKE_MEMBER_ALIAS(variable, name) make_field_key_value_pair(#name, variable, member_offset(this, &variable))

	// This is used to set the serializer dispatch method for a type
	#define SERIALIZER_SET_DISPATCH(C, type) \
		template <>\
		struct SerializerType<C>\
		{\
			enum\
			{\
				value = type\
			};\
		};


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

	// SerializerTypes
	enum
	{
		SerializerType_UNDEFINED,
		SerializerType_INTERNAL,
		SerializerType_EXTERNAL,
		SerializerType_POD
	};


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


	template <class T>
	struct SerializerType
	{
		static constexpr uint32_t value = SerializerType_UNDEFINED;
	};

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
			static_assert(0, "T is being serialized with no serializer type.");
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
		void dispatch(Archive& archiv, FieldKeyValuePair<T>& pair)
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
		If <!std::is_class<T>::value,
			SerializeDispatcherPOD<T>,
			If <SerializerType<T>::value == SerializerType_INTERNAL,
			SerializeDispatcherInternal<T>,
			If <SerializerType<T>::value == SerializerType_EXTERNAL,
			SerializeDispatcherExternal<T>,
			SerializeDispatcherUndefined<T>>::type>::type>::type dispatcher;

		dispatcher.dispatch(archive, pair);
	} // SerializeDispatcher



	template <class Archive, class T>
	void serialize(Archive& archive, T& instance)
	{
		// If you hit this, T was defined with an external serializer
		// but a specialization for this function could not be found for T.
		static_assert(0, "EXTERNAL serializer missing for class T");
	}

} // namespace gemini


// archive.h
namespace gemini
{
	template <class ArchiveType>
	struct ArchiveInterface
	{
		ArchiveInterface()
		{
		}

		ArchiveType& instance()
		{
			return *static_cast<ArchiveType*>(this);
		}

		template <class T>
		ArchiveType& operator&(FieldKeyValuePair<T>& pair)
		{
			if (IsSaving)
			{
				instance().save_pair(pair);
			}
			else if (IsLoading)
			{
				instance().load_pair(pair);
			}
			return instance();
		}

		template <class T>
		ArchiveType& operator<<(const T& value)
		{
			FieldKeyValuePair<T> pair("Unnamed", std::forward<T>(const_cast<T&>(value)), 0);
			SerializeDispatcher<ArchiveType, T>(instance(), pair);
			return instance();
		}

		template <class T>
		void save_pair(FieldKeyValuePair<T>& pair)
		{
			instance().save(pair.value);
		}

		template <class T>
		ArchiveType& operator >> (T& value)
		{
			FieldKeyValuePair<T> pair("Unnamed", std::forward<T>(value), 0);
			SerializeDispatcher(instance(), pair);
			return instance();
		}

		template <class T>
		void load_pair(FieldKeyValuePair<T>& pair)
		{
			instance().load(pair.value);
		}

		template <class T>
		void save(T& value)
		{
			//' If you hit this, no save function was specified for the derived
			// class being used.
			assert(0);
		}

		template <class T>
		void load(T& value)
		{
			//' If you hit this, no load function was specified for the derived
			// class being used.
			assert(0);
		}

		uint16_t IsLoading = 0;
		uint16_t IsSaving = 0;
	}; // ArchiveInterface
} // namespace gemini

// fieldcollector.h
#include <core/array.h>
#include <core/typespec.h>

namespace gemini
{
	struct FieldCollector : public ArchiveInterface<FieldCollector>
	{
		gemini::Allocator allocator;
		Array<gemini::string> fields;
		Array<intptr_t> offsets;
		Array<TypeSpecInfo*> info;

		FieldCollector()
			: allocator(gemini::memory_allocator_default(MEMORY_ZONE_DEFAULT))
			, fields(allocator)
			, offsets(allocator)
			, info(allocator)
		{
			IsSaving = 1;
		}

		template <class T>
		void save_pair(FieldKeyValuePair<T>& pair)
		{
			TypeSpecInfo* typeinfo = typespec_make_info<T>();
			info.push_back(typeinfo);
			fields.push_back(string_create(allocator, pair.name));
			offsets.push_back(pair.offset);
		}

		template <class T>
		void save(T& value)
		{
		}

		template <class T>
		void load(T& value)
		{
		}

		~FieldCollector()
		{
			for (size_t index = 0; index < fields.size(); ++index)
			{
				string_destroy(allocator, fields[index]);
			}
		}
	}; // FieldCollector
} // namespace gemini


// keyvaluearchive.h
#include <core/hashset.h>
#include <core/logging.h>
#include <core/mathlib.h>

namespace gemini
{

	class KeyValueReader : public ArchiveInterface<KeyValueReader>
	{
	public:
		KeyValueReader(gemini::Allocator& _allocator)
			: allocator(_allocator)
			, items(_allocator)
		{
			IsLoading = 1;
		}

		template <class T>
		void handle_field(FieldKeyValuePair<T>& nvp)
		{

			LOGV("handle field KeyValueReader\n");
			//TypeSpecInfo* typeinfo = typespec_make_info<T>();
			//info.push_back(typeinfo);
			//fields.push_back(string_create(allocator, nvp.name));
			//offsets.push_back(nvp.offset);
		}

		void set_item(gemini::string key, gemini::string value)
		{
			items[key] = value;
		}

		template <class T>
		void load_pair(FieldKeyValuePair<T>& pair)
		{
			instance().prologue(pair);
			instance().load(pair.value);
			instance().epilogue(pair);
		}

		template <class T>
		void prologue(FieldKeyValuePair<T>& pair)
		{
			set_next_name(pair.name);
		}

		template <class T>
		void epilogue(FieldKeyValuePair<T>& pair)
		{
			string_destroy(allocator, current_field_name);
		}


		template <class T>
		void load(T& value)
		{
			// If you hit this, then the archive couldn't resolve the correct type.
			assert(0);
		}

		template <>
		void load(uint32_t& value)
		{
			if (items.has_key(current_field_name))
			{
				gemini::string field_value = items.get(current_field_name);
				value = atoi(field_value.c_str());
			}
		}

		template <>
		void load(float& value)
		{
			if (items.has_key(current_field_name))
			{
				gemini::string field_value = items.get(current_field_name);
				value = atof(field_value.c_str());
			}
		}

		template <>
		void load(char*& value)
		{
			if (items.has_key(current_field_name))
			{
				gemini::string field_value = items.get(current_field_name);
				value = new char[field_value.size() + 1];
				value[field_value.size()] = '\0';
				core::str::copy(value, field_value.string_data, field_value.string_data_size);
			}
		}

		template <>
		void load(glm::vec4& value)
		{
			if (items.has_key(current_field_name))
			{
				gemini::string field_value = items.get(current_field_name);
				int results = sscanf_s(field_value.c_str(),
					"%f %f %f %f",
					&value.x, &value.y, &value.z, &value.w);
				if (results < 4)
				{
					LOGV("Error reading vec4.\n");
				}
			}
		}


		void set_next_name(const char* name)
		{
			current_field_name = string_create(allocator, name);
		}

		HashSet<gemini::string, gemini::string> items;

	private:
		gemini::string current_field_name;
		gemini::Allocator& allocator;

	}; // KeyValueReader
} // namespace gemini
