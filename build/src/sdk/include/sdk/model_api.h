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

#include <core/mathlib.h>

namespace gemini
{
	struct GeometryInstanceData
	{
		unsigned int material_id;
		unsigned int shader_id;
	};

	class IModelInstanceData
	{
	public:
		virtual ~IModelInstanceData() {};
		
		virtual unsigned int asset_index() const = 0;
		virtual glm::mat4& get_local_transform() = 0;
		virtual void set_local_transform(const glm::mat4& transform) = 0;
//		virtual void get_geometry_data(unsigned int index, GeometryInstanceData& geometry_data) const = 0;
	};

	class IModelInterface
	{
	public:
		virtual ~IModelInterface() {};
		
		
//		virtual int32_t get_model_index(const char* model_path) = 0;
		
		virtual int32_t create_instance_data(const char* model_path) = 0;
		virtual void destroy_instance_data(int32_t index) = 0;
		
		virtual IModelInstanceData* get_instance_data(int32_t index) = 0;
		
	};
}