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

#include <assert.h>

#include <cmath>

#include <string.h> // for memcpy

#include <core/typedefs.h>
#include <core/array.h>

#define GLM_FORCE_RADIANS 1
#include <glm/vec2.hpp>
#include <glm/mat2x2.hpp>
#include <glm/common.hpp> // for mix
#include <glm/gtx/quaternion.hpp>


#include <functional>

namespace gui
{
	typedef ::glm::vec2 Point;

	#define DECLARE_PANEL( panelClass ) virtual const char * GetClassname() const { return #panelClass; }

	typedef float DimensionType;
// -------------------------------------------------------------------------------------------------------------
	typedef int16_t ScreenInt;

	template <class EventArgsType, class ReturnVal = void>
	class EventHandler
	{
	public:
		typedef ReturnVal ( *callback_type )( EventArgsType );

		LIBRARY_EXPORT EventHandler()
		{
			_handler = 0;
		}

		LIBRARY_EXPORT virtual ~EventHandler() {}

		LIBRARY_EXPORT void operator = ( callback_type _fn )
		{
			_handler = _fn;
		}

		// call a specific function with the args if specified, otherwise fires the event with given args
		LIBRARY_EXPORT ReturnVal operator() ( EventArgsType args, callback_type _fn = 0 )
		{
			if ( _fn != 0 )
			{
				return _fn( args );
			}

			if ( _handler != 0 )
			{
				return _handler( args );
			}
		}

	private:
		callback_type _handler;
	}; // EventHandler



// -------------------------------------------------------------------------------------------------------------

	// ---------------------------------------------------------------------
	//
	// ---------------------------------------------------------------------
	template <size_t T>
	struct InterpolatedGeometry
	{
		glm::vec2 current[T];
		glm::vec2 last[T];
		glm::vec2 final[T];

		// 1. Snapshot of the current geometry
		// 2. Update the 'current' geometry
		// 3. Call lerp with the alpha
		// 4. draw using final

		void snapshot()
		{
			// take a current snapshot of the vertices
			for (size_t index = 0; index < T; ++index)
				last[index] = current[index];
		}

		void lerp(float alpha)
		{
			for (size_t index = 0; index < T; ++index)
			{
				final[index] = glm::mix(last[index], current[index], alpha);
			}
		}

		glm::vec2* vertices()
		{
			return current;
		}

		glm::vec2& operator[](size_t index)
		{
			return current[index];
		}

		const glm::vec2* interpolated_vertices() const
		{
			return final;
		}
	};


	//
	// Size
	struct Size
	{
		DimensionType width, height;

		Size( DimensionType _width = 0, DimensionType _height = 0 );

		void set_width( DimensionType in_width );
		DimensionType get_width() const;
	}; // Size

	//
	// Point
//	struct LIBRARY_EXPORT Point
//	{
//		ScreenInt x, y;
//
//		Point( ScreenInt _x = 0, ScreenInt _y = 0 );
//		Point(const Point& other);
//
//		bool is_near_point( const Point & other, ScreenInt radius );
//
//		DimensionType get_x();
//		void set_x( ScreenInt value );
//		DimensionType get_y();
//		void set_y( ScreenInt value );
//
//
//		Point rotated(float rads);
//		float length() const;
//
//		Point operator-( const Point &other ) const;
//		Point operator+( const Point &other ) const;
//		const Point& operator += (const Point& other);
//	}; // Point

	//
	// Rect
	// this assumes an origin in the upper left hand corner and assumes bottom > top, right > left
	struct Rect
	{
		Point origin;
		Size size;

		LIBRARY_EXPORT Rect(const Point& _origin, const Size& _size);
		LIBRARY_EXPORT Rect(DimensionType left = 0, DimensionType top = 0, DimensionType width = 0, DimensionType height = 0);

		LIBRARY_EXPORT Size& get_size();
		LIBRARY_EXPORT Point& get_origin();

		LIBRARY_EXPORT void set(DimensionType x, DimensionType y, DimensionType width, DimensionType height);
		LIBRARY_EXPORT DimensionType width() const;
		LIBRARY_EXPORT DimensionType height() const;

		LIBRARY_EXPORT bool fits_inside(const Rect& other) const;
		LIBRARY_EXPORT bool is_point_inside( const glm::vec2& pt) const;
	}; // Rect



	// -------------------------------------------------------------------------------------------------------------

	struct LIBRARY_EXPORT InputState
	{
		virtual ~InputState() {}

		virtual bool is_mouse_down(int index) = 0;
		virtual bool is_mouse_held(int index) = 0;
	};

	// -------------------------------------------------------------------------------------------------------------
	struct LIBRARY_EXPORT KeyState
	{
		// the state follows this logic:
		// (state & 1) -> isDown this update
		// (state & 2) -> wasDown last update
		// (state & 4) -> released this update
		// (state & 8) -> impulse flag; if set, isDown or released happened this update
		unsigned char state : 4;

		bool is_down() const { return (state & 1) > 0; }
		bool was_down() const { return (state & 2) > 0; }

		void clear();
		KeyState();
	}; // KeyState

	namespace render
	{
		extern TextureHandle WhiteTexture;
	} // namespace render

#ifdef byte
#undef byte
#endif

#ifndef byte
	typedef unsigned char byte;
#endif

	struct LIBRARY_EXPORT Color
	{
		byte rgba[4];

		Color(byte red = 0, byte green = 0, byte blue = 0, byte alpha = 255);

		inline byte r() const { return rgba[0]; }
		inline byte g() const { return rgba[1]; }
		inline byte b() const { return rgba[2]; }
		inline byte a() const { return rgba[3]; }
	};

	// ---------------------------------------------------------------------
	// event handling / delegate
	// ---------------------------------------------------------------------
	template <class T>
	class DelegateHandler
	{
		// inspirations:
		// boost::signals
		// Don Clugston's Fast Delegate
		// function and bind

		// Requirements:
		// + must handle arbitrary arguments or struct type
		// + need to bind a member function pointer or free (static) function
		// - should be able to support delayed invocation
		//	 This is possible, but not implemented at this time.

		// std::function has deleted operator==; therefore
		// we cannot implement a disconnect function as the backing
		// store of delegate<T> is an std::function.
	public:

		// connect overloads
		template <class C>
		void connect(void (C::*function_ptr)(T), C* instance)
		{
			Delegate func;
			func.set(function_ptr, instance);
			connections.push_back(func);
		}

		template <class C>
		void connect(void (C::*function_ptr)(T) const, C* instance)
		{
			Delegate func;
			func.set(function_ptr, instance);
			connections.push_back(func);
		}

		void connect(void (*function_ptr)(T))
		{
			Delegate func;
			func.set(function_ptr);
			connections.push_back(func);
		}

		// emit event to all connections
		void operator()(T value) const
		{
			for (size_t index = 0; index < connections.size(); ++index)
			{
				const Delegate& connection = connections[index];
				connection.invoke(value);
			}
		}

		// remove all connections from this event
		void clear_connections()
		{
			connections.clear();
		}

	private:
		class Delegate
		{
		public:
			template <class C>
			void set(void (C::*function_ptr)(T), C* instance)
			{
				function_pointer = std::bind(function_ptr, instance, std::placeholders::_1);
			}

			template <class C>
			void set(void (C::*function_ptr)(T) const, C* instance)
			{
				function_pointer = std::bind(function_ptr, instance, std::placeholders::_1);
			}

			void set(void (*function_ptr)(T))
			{
				function_pointer = std::bind(function_ptr, std::placeholders::_1);
			}

			void invoke(T value) const
			{
				assert(function_pointer != nullptr);
				function_pointer(value);
			}

		private:
			std::function<void (T)> function_pointer;
		};

		void remove_if_found(const Delegate& func)
		{
			connections.erase(func);
		}

		Array< Delegate > connections;
	};

	// ---------------------------------------------------------------------
	// math stuff
	// ---------------------------------------------------------------------
	template <class T>
	T sin(const T value)
	{
		return ::sin(value);
	}

	template <class T>
	T cos(const T value)
	{
		return ::cos(value);
	}


	void transform_geometry(glm::vec2* geometry, size_t total_vertices, const glm::mat2& transform);

	// generate a 2D rotation matrix
	glm::mat2 rotation_matrix(const float z_radians);

	// generate a 2D scale matrix
	glm::mat2 scale_matrix(const Point& scale);
} // namespace gui
