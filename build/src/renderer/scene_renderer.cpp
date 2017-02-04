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
#include <renderer/renderer.h>
#include <renderer/scene_renderer.h>
#include <renderer/vertexdescriptor.h>

#include <runtime/assets.h>
#include <runtime/mesh.h>


namespace gemini
{
	AnimatedMeshComponent* render_scene_add_animated_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform)
	{
		AnimatedMeshComponent* component = MEMORY2_NEW(*scene->allocator, AnimatedMeshComponent);
		component->entity_index = entity_index;
		component->mesh_handle = mesh_handle;
		component->model_matrix = model_transform;
		component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_transform)));
		scene->animated_meshes.push_back(component);

		return component;
	} // render_scene_add_animated_mesh

	void render_scene_add_static_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform)
	{
		StaticMeshComponent* component = MEMORY2_NEW(*scene->allocator, StaticMeshComponent);
		component->entity_index = entity_index;
		component->mesh_handle = mesh_handle;
		component->model_matrix = model_transform;
		component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_transform)));
		scene->static_meshes.push_back(component);
	} // render_scene_add_static_mesh

	RenderScene* render_scene_create(Allocator& allocator, render2::Device* device)
	{
		RenderScene* scene = MEMORY2_NEW(allocator, RenderScene)(allocator);

		// create pipelines

		// static mesh pipeline
		{
			render2::PipelineDescriptor desc;
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_normal", render2::VD_FLOAT, 3);
			vertex_format.add("in_uv", render2::VD_FLOAT, 2);
			desc.shader = shader_load("rendertest");
			desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
			scene->static_mesh_pipeline = device->create_pipeline(desc);
		}

		// animated (skeletal) mesh pipeline
		{
			render2::PipelineDescriptor desc;
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_normal", render2::VD_FLOAT, 3);
			vertex_format.add("in_uv", render2::VD_FLOAT, 2);
			vertex_format.add("in_blendindices", render2::VD_FLOAT, 4);
			vertex_format.add("in_blendweights", render2::VD_FLOAT, 4);
			desc.shader = shader_load("animated");
			desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
			scene->animated_mesh_pipeline = device->create_pipeline(desc);
		}

		return scene;
	} // render_scene_create

	void render_scene_destroy(RenderScene* scene, render2::Device* device)
	{
		for (size_t index = 0; index < scene->static_meshes.size(); ++index)
		{
			MEMORY2_DELETE(*scene->allocator, scene->static_meshes[index]);
		}

		for (size_t index = 0; index < scene->animated_meshes.size(); ++index)
		{
			MEMORY2_DELETE(*scene->allocator, scene->animated_meshes[index]);
		}

		device->destroy_pipeline(scene->static_mesh_pipeline);
		device->destroy_pipeline(scene->animated_mesh_pipeline);

		MEMORY2_DELETE(*scene->allocator, scene);
	} // render_scene_destroy

	void render_scene_draw(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::RenderTarget* render_target)
	{
		Color clear_color = Color::from_rgba(128, 128, 128, 255);
		render2::Pass render_pass;
		if (!render_target)
		{
			render_pass.target = device->default_render_target();
		}
		render_pass.target = render_target;
		render_pass.color(clear_color.red, clear_color.blue, clear_color.green, clear_color.alpha);
		render_pass.clear_color = true;
		render_pass.clear_depth = true;
		render_pass.depth_test = true;
		render_pass.depth_write = true;
		render_pass.cull_mode = render2::CullMode::Backface;
		render_static_meshes(scene, device, view, projection, render_pass);


		render2::Pass animated_pass;
		if (!render_target)
		{
			animated_pass.target = device->default_render_target();
		}
		animated_pass.target = render_target;
		animated_pass.clear_color = false;
		animated_pass.clear_depth = false;
		animated_pass.depth_test = true;
		animated_pass.depth_write = true;
		animated_pass.cull_mode = render2::CullMode::Backface;
		render_animated_meshes(scene, device, view, projection, animated_pass);
	} // render_scene_draw

	void render_static_meshes(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass)
	{
		render2::CommandQueue* queue = device->create_queue(pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		// render static meshes
		// setup the static mesh pipeline
		static uint32_t diffuse_unit = 0;
		serializer->pipeline(scene->static_mesh_pipeline);
		scene->static_mesh_pipeline->constants().set("view_matrix", &view);
		scene->static_mesh_pipeline->constants().set("projection_matrix", &projection);
		scene->static_mesh_pipeline->constants().set("diffuse", &diffuse_unit);
		scene->static_mesh_pipeline->constants().set("light_position_world", &scene->light_position_world);
		scene->static_mesh_pipeline->constants().set("camera_position_world", &scene->camera_position_world);
		scene->static_mesh_pipeline->constants().set("camera_view_direction", &scene->camera_view_direction);

		serializer->texture(texture_from_handle(texture_load("textures/measure")), diffuse_unit);

		for (size_t index = 0; index < scene->static_meshes.size(); ++index)
		{
			StaticMeshComponent* static_mesh = scene->static_meshes[index];

			Mesh* mesh = mesh_from_handle(static_mesh->mesh_handle);
			if (mesh)
			{
				serializer->constant("model_matrix", &static_mesh->model_matrix, sizeof(glm::mat4));
				serializer->constant("normal_matrix", &static_mesh->normal_matrix, sizeof(glm::mat3));

				// TODO: Convert into render blocks that can be re-sorted.
				for (size_t geo = 0; geo < mesh->geometry.size(); ++geo)
				{
					//::renderer::Geometry* geometry = mesh->geometry[geo];
					const GeometryDefinition* geometry = &mesh->geometry[geo];
					//Material* material = material_from_handle(geometry->material_id);
					// TODO: setup material for rendering
#if 0

					serializer->vertex_buffer(geometry->vertex_buffer);
					serializer->draw_indexed_primitives(geometry->index_buffer, geometry->indices.size());
#endif
				}
			}
		}

		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
	} // render_static_meshes

	void render_animated_meshes(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass)
	{
		render2::CommandQueue* queue = device->create_queue(pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		// render static meshes
		// setup the static mesh pipeline
		static uint32_t diffuse_unit = 0;
		serializer->pipeline(scene->static_mesh_pipeline);
		scene->static_mesh_pipeline->constants().set("view_matrix", &view);
		scene->static_mesh_pipeline->constants().set("projection_matrix", &projection);
		scene->static_mesh_pipeline->constants().set("diffuse", &diffuse_unit);
		scene->static_mesh_pipeline->constants().set("light_position_world", &scene->light_position_world);
		scene->static_mesh_pipeline->constants().set("camera_position_world", &scene->camera_position_world);
		scene->static_mesh_pipeline->constants().set("camera_view_direction", &scene->camera_view_direction);

		serializer->texture(texture_from_handle(texture_load("textures/measure")), diffuse_unit);

		for (size_t index = 0; index < scene->animated_meshes.size(); ++index)
		{
			AnimatedMeshComponent* instance = scene->animated_meshes[index];

			Mesh* mesh = mesh_from_handle(instance->mesh_handle);
			if (mesh)
			{
				serializer->constant("model_matrix", &instance->model_matrix, sizeof(glm::mat4));
				serializer->constant("normal_matrix", &instance->normal_matrix, sizeof(glm::mat3));
#if 0
				for (size_t geo = 0; geo < mesh->geometry.size(); ++geo)
				{
					::renderer::Geometry* geometry = mesh->geometry[geo];
					//Material* material = material_from_handle(geometry->material_id);
					// TODO: setup material for rendering

					serializer->vertex_buffer(geometry->vertex_buffer);
					serializer->draw_indexed_primitives(geometry->index_buffer, geometry->indices.size());
				}
#endif
			}
		}

		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
	} // render_animated_meshes
} // namespace gemini
