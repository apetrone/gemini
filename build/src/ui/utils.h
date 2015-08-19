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

#define GLM_FORCE_RADIANS 1
#include <glm/vec2.hpp>
#include <glm/mat2x2.hpp>
#include <glm/common.hpp> // for mix
#include <glm/gtx/quaternion.hpp>

namespace gui
{
	typedef ::glm::vec2 Point;

	#define DECLARE_PANEL( panelClass ) virtual const char * GetClassname() const { return #panelClass; }

	typedef float DimensionType;
// -------------------------------------------------------------------------------------------------------------
	typedef int16_t ScreenInt;

	template <class EventArgsType, class ReturnVal = void>
	class LIBRARY_EXPORT EventHandler
	{
	public:
		typedef ReturnVal ( *callback_type )( EventArgsType );

		EventHandler()
		{
			_handler = 0;
		}

		virtual ~EventHandler() {}

		void operator = ( callback_type _fn )
		{
			_handler = _fn;
		}

		// call a specific function with the args if specified, otherwise fires the event with given args
		ReturnVal operator() ( EventArgsType args, callback_type _fn = 0 )
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
	struct LIBRARY_EXPORT Size
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
	struct LIBRARY_EXPORT Rect
	{
		Point origin;
		Size size;
		
		Rect(const Point& _origin, const Size& _size);
		Rect(DimensionType left = 0, DimensionType top = 0, DimensionType width = 0, DimensionType height = 0);
		
		Size& get_size();
		Point& get_origin();
		
		void set(DimensionType x, DimensionType y, DimensionType width, DimensionType height);
		DimensionType width() const;
		DimensionType height() const;
		
		bool fits_inside(const Rect& other) const;
		bool is_point_inside( const glm::vec2& pt) const;
	}; // Rect
	

	
	// -------------------------------------------------------------------------------------------------------------
	
	struct LIBRARY_EXPORT InputState
	{
		virtual ~InputState() {}
		
		virtual bool IsMouseDown( int index ) = 0;
		virtual bool IsMouseHeld( int index ) = 0;
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
		
		bool isDown() const { return (state & 1) > 0; }
		bool wasDown() const { return (state & 2) > 0; }
		
		void Clear();
		KeyState();
	}; // KeyState
	
		
	typedef uint32_t TextureHandle;
	enum TextureResult
	{
		TextureResult_Success = 0,
		TextureResult_Failed = 1,
		
	}; // TextureResult
	
	typedef uint32_t FontHandle;
	enum FontResult
	{
		FontResult_Success = 0,
		FontResult_Failed = 1,
	}; // FontResult
	
//	class IRenderer;
//	struct Panel;
//	void DrawOutline( IRenderer * r, Point corners[8], const gui::Color & color );
//	void DrawFloat( IRenderer * r, float value, const Point & p, const gui::Color & color );
//	void CalculatePanelCorners( Panel * panel, Point corners[8] );
	
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
		
		byte r() const { return rgba[0]; }
		byte g() const { return rgba[1]; }
		byte b() const { return rgba[2]; }
		byte a() const { return rgba[3]; }
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
