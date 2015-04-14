// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include <stdint.h>
#include <core/interface.h>

#include <core/mathlib.h>


namespace core
{
	namespace logging
	{
		class ILog;
	}
}

namespace gemini
{
	class IEntityManager;
	class IModelInterface;

	
	namespace physics
	{
		class IPhysicsInterface;
	}
	
	class IDebugDraw;
	class IAudioInterface;
	class IExperimental;
	class IEngineEntity;
	
	class IEngineInterface
	{
	public:
		virtual ~IEngineInterface() {};

		// systems
		virtual IEntityManager* entities() = 0;
		virtual IModelInterface* models() = 0;
		virtual physics::IPhysicsInterface* physics() = 0;
		virtual IExperimental* experiment() = 0;
		virtual core::logging::ILog* log() = 0;
		virtual IDebugDraw* debugdraw() = 0;
		virtual IAudioInterface* audio() = 0;
		
		// memory hooks
		virtual void* allocate(size_t bytes) = 0;
		virtual void deallocate(void* pointer) = 0;
		
		virtual void render_view(const glm::vec3& origin, const glm::vec2& view_angles) = 0;
		virtual void render_gui() = 0;
		
		
		
		
//		virtual void render_world(const glm::vec3& origin, const glm::vec2& view_angles) = 0;

		// render a viewmodel (no depth testing; different fov)
		virtual void render_viewmodel(IEngineEntity* entity, const glm::vec3& origin, const glm::vec2& view_angles) = 0;
		
		virtual void get_view_angles(glm::vec2& view_angles) = 0;
		
		// center the cursor in the main window (if applicable on this platform)
		virtual void center_cursor() = 0;
	};
	
	namespace engine
	{
		typedef core::Interface<IEngineInterface> api;
	}
} // namespace gemini