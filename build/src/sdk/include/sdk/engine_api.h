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

#include <stdint.h>
#include <core/interface.h>

#include <core/mathlib.h>

namespace gemini
{
	class IEntityManager;
	class IModelInterface;
	
	namespace physics
	{
		class IPhysicsInterface;
	}
	
	class IExperimental;

	class IEngineInterface
	{
	public:
		virtual ~IEngineInterface() {};

		// systems
		virtual IEntityManager* entities() = 0;
		virtual IModelInterface* models() = 0;
		virtual physics::IPhysicsInterface* physics() = 0;
		virtual IExperimental* experiment() = 0;
		
		// memory hooks
		virtual void* allocate(size_t bytes) = 0;
		virtual void deallocate(void* pointer) = 0;
		
		virtual void render_view(const glm::vec3& origin, const glm::vec2& view_angles) = 0;
//		virtual void render_view_from_entity(int32_t entity_index) = 0;

		virtual void get_view_angles(glm::vec2& view_angles) = 0;
	};
	
	namespace engine
	{
		typedef Interface<IEngineInterface> api;
	}
}