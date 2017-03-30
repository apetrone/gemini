// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//      * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.

//      * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimer in the documentation
//      and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//       SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <sdk/utils.h>
#include <sdk/physics_collisionobject.h>

#include <sdk/game_api.h>

namespace gemini
{
	EntityFactoryRegistrar& entity_factory_registrar()
	{
		static EntityFactoryRegistrar _registrar;
		return _registrar;
	}

	namespace util
	{
		Entity* create_entity_by_classname(const char* classname)
		{
			FactoryClass* factory_class = entity_factory_registrar().find_factory_by_name(classname);
			if (factory_class)
			{
				return (Entity*)factory_class->create();
			}

			// If you hit this assert, the entity class hasn't been registered.
			assert(0);
			return (Entity*)0;
		}

		float clamp_rotation(float value)
		{
			if (value > 360)
			{
				value -= 360;
			}
			else if (value < -360)
			{
				value += 360;
			}

			return value;
		}

		Entity* entity_from_collisionobject(physics::ICollisionObject* object)
		{
			return reinterpret_cast<Entity*>(object->get_user_data());
		}
	}

	// from game_api: create the inverse camera transform matrix
	glm::mat4 camera_state_to_transform(const CameraState& camera_state)
	{
		glm::mat4 pivot_offset = glm::translate(glm::mat4(1.0f), glm::vec3(camera_state.horizontal_offset, camera_state.vertical_offset, 0.0f));
		glm::vec3 position = camera_state.position; // (-camera_state.view * camera_state.distance_from_pivot);
		return glm::translate(glm::mat4(1.0f), position) * glm::toMat4(camera_state.rotation) * pivot_offset;
	}

	// given a camera state, transform input vector
	// to camera space.
	glm::vec3 transform_to_camera_space(const CameraState& camera_state, const glm::vec3& input)
	{
		glm::mat4 pivot_offset = glm::translate(glm::mat4(1.0f), glm::vec3(camera_state.horizontal_offset, camera_state.vertical_offset, 0.0f));
		glm::mat4 world_transform = glm::translate(glm::mat4(), input);
		glm::mat4 result = world_transform * glm::toMat4(camera_state.rotation) * pivot_offset;

		return glm::vec3(glm::column(result, 3));
	} // transform_to_camera_sapce
} // namespace gemini
