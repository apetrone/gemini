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

#include <core/array.h>
#include <core/mathlib.h>
#include <core/typedefs.h>

#include <runtime/asset_handle.h>


namespace render2
{
	class Device;
	class Pipeline;
	struct Pass;
	struct RenderTarget;
} // namespace render2

namespace gemini
{
	struct StaticMeshComponent
	{
		AssetHandle mesh_handle;
		uint16_t entity_index;

		// populated in extract phase
		glm::mat4 model_matrix;
		glm::mat3 normal_matrix;
	}; // StaticMeshComponent

	struct AnimatedMeshComponent
	{
		AssetHandle mesh_handle;
		uint16_t entity_index;

		// populated in extract phase
		glm::mat4 model_matrix;
		glm::mat3 normal_matrix;

		// Assumptions we're going to make for now for simplicity.
		// An animated mesh will only have ONE geometry chunk.

		// inverse bind poses used by the instance associated with this component.
		glm::mat4* inverse_bind_poses;
	}; // AnimatedMeshComponent

	struct RenderScene
	{
		Allocator* allocator;
		Array<StaticMeshComponent*> static_meshes;
		Array<AnimatedMeshComponent*> animated_meshes;

		render2::Pipeline* static_mesh_pipeline;
		render2::Pipeline* animated_mesh_pipeline;

		// additional scene-specific uniforms
		glm::vec3 light_position_world;
		glm::vec3 camera_position_world;
		glm::vec3 camera_view_direction;

		RenderScene(Allocator& _allocator)
			: static_meshes(_allocator)
			, animated_meshes(_allocator)
			, allocator(&_allocator)
			, static_mesh_pipeline(nullptr)
			, animated_mesh_pipeline(nullptr)
		{
		}
	}; // RenderScene

	void render_scene_startup(render2::Device* device, Allocator& allocator);
	void render_scene_shutdown();

	AnimatedMeshComponent* render_scene_add_animated_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform);
	StaticMeshComponent* render_scene_add_static_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform);
	RenderScene* render_scene_create(Allocator& allocator, render2::Device* device);
	void render_scene_destroy(RenderScene* scene, render2::Device* device);

	void render_scene_draw(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::RenderTarget* render_target = nullptr);
	void render_static_meshes(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass);
	void render_animated_meshes(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass);
} // namespace gemini