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
#include "ui/ui.h"
#include "ui/utils.h"

namespace gui
{
	namespace render
	{
		TextureHandle WhiteTexture(-1);
	} // namespace render

	// ---------------------------------------------------------------------
	// Size
	// ---------------------------------------------------------------------
	Size::Size(DimensionType _width, DimensionType _height)
	{
		width = _width;
		height = _height;
	}
	
	void Size::set_width(DimensionType in_width)
	{
		width = in_width;
	}
	
	DimensionType Size::get_width() const
	{
		return width;
	}

	// ---------------------------------------------------------------------
	// Rect
	// ---------------------------------------------------------------------
	Rect::Rect(const Point& _origin, const Size& _size)
	{
		origin = _origin;
		size = _size;
	}
	
	Rect::Rect(DimensionType left, DimensionType top, DimensionType width, DimensionType height)
	{
		this->set(left, top, width, height);
	}
	
	Size& Rect::get_size()
	{
		return this->size;
	}
	
	Point& Rect::get_origin()
	{
		return this->origin;
	}
	
	void Rect::set(DimensionType x, DimensionType y, DimensionType width, DimensionType height)
	{
		origin.x = x;
		origin.y = y;
		
		size.width = width;
		size.height = height;
	}
	
	DimensionType Rect::width() const
	{
		return size.width;
	}
	
	DimensionType Rect::height() const
	{
		return size.height;
	}
	
	bool Rect::fits_inside(const Rect& other) const
	{
		if (this->width() <= other.width() && this->height() <= other.height())
			return true;
		
		return false;
	}
	
	bool Rect::is_point_inside(const Point& pt) const
	{
		if (origin.x > pt.x)
			return false;
		else if (origin.y > pt.y)
			return false;
		else if ((origin.x + size.width) < pt.x)
			return false;
		else if ((origin.y + size.height) < pt.y)
			return false;
		
		return true;
	}
	
	// ---------------------------------------------------------------------
	// Color
	// ---------------------------------------------------------------------
	Color::Color(byte red, byte green, byte blue, byte alpha)
	{
		rgba[0] = red;
		rgba[1] = green;
		rgba[2] = blue;
		rgba[3] = alpha;
	}


	// ---------------------------------------------------------------------
	// math utils
	// ---------------------------------------------------------------------
	void transform_geometry(glm::vec2* geometry, size_t total_vertices, const glm::mat2& transform)
	{
		for (size_t index = 0; index < total_vertices; ++index)
		{
			geometry[index] = geometry[index] * transform;
		}
	}

	glm::mat2 rotation_matrix(const float radians)
	{
		return glm::mat2(
		  cos(radians), -sin(radians),
		  sin(radians), cos(radians)
	  );
	}

	glm::mat2 scale_matrix(const Point& scale)
	{
		return glm::mat2(
			scale.x, 0.0f,
			0.0f, scale.y
		);
	}
} // namespace gui
