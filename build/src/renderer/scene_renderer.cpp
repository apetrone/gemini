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
#include <core/freelist.h>
#include <core/logging.h>

#include <renderer/commandbuffer.h>
#include <renderer/debug_draw.h>
#include <renderer/renderer.h>
#include <renderer/rendertarget.h>
#include <renderer/scene_renderer.h>
#include <renderer/vertexdescriptor.h>

#include <runtime/animation.h>
#include <runtime/assets.h>
#include <runtime/material.h>
#include <runtime/mesh.h>

#include <shared/shared_constants.h>


namespace gemini
{
	// cached meshes associated with an asset handle in the runtime.
	// Data required by the renderer for each mesh.
	struct RenderMeshInfo
	{
		render2::Buffer* vertex_buffer;
		render2::Buffer* index_buffer;
	}; // RenderMeshInfo

	struct RenderSceneState
	{
		RenderSceneState(Allocator& _allocator)
			: allocator(&_allocator)
			, render_mesh_by_handle(_allocator)
			, render_meshes(_allocator)
		{
		}

		Allocator* allocator;
		render2::Device* device;
		HashSet<AssetHandle, RenderMeshInfo*> render_mesh_by_handle;
		Array<RenderMeshInfo*> render_meshes;
	}; // RenderSceneState

	AnimatedMeshComponent::AnimatedMeshComponent(gemini::Allocator& in_allocator)
		: sequence_instances(in_allocator)
	{
	}

	RenderSceneState* render_scene_state = nullptr;

	void render_scene_startup(render2::Device* device, Allocator& allocator)
	{
		if (!render_scene_state)
		{
			render_scene_state = MEMORY2_NEW(allocator, RenderSceneState)(allocator);
			render_scene_state->device = device;
		}
	} // render_scene_startup


	void render_scene_shutdown()
	{
		assert(render_scene_state);

		// purge all meshes
		render_scene_state->render_mesh_by_handle.clear();
		for (size_t index = 0; index < render_scene_state->render_meshes.size(); ++index)
		{
			RenderMeshInfo* render_mesh = render_scene_state->render_meshes[index];

			render_scene_state->device->destroy_buffer(render_mesh->vertex_buffer);
			render_scene_state->device->destroy_buffer(render_mesh->index_buffer);

			MEMORY2_DELETE(*render_scene_state->allocator, render_mesh);
		}
		render_scene_state->render_meshes.clear();

		MEMORY2_DELETE(*render_scene_state->allocator, render_scene_state);
	} // render_scene_shutdown


	void interleave_static_mesh(char* vertex_data, Mesh* mesh, uint32_t total_vertices)
	{
		for (size_t vertex_index = 0; vertex_index < total_vertices; ++vertex_index)
		{
			renderer::StaticMeshVertex* vertex = reinterpret_cast<renderer::StaticMeshVertex*>(vertex_data) + vertex_index;
			vertex->position = mesh->vertices[vertex_index];
			vertex->normal = mesh->normals[vertex_index];
			vertex->uvs = mesh->uvs[vertex_index];
		}
	}


	void interleave_animated_mesh(char* vertex_data, Mesh* mesh, uint32_t total_vertices)
	{
		for (size_t vertex_index = 0; vertex_index < total_vertices; ++vertex_index)
		{
			renderer::AnimatedMeshVertex* vertex = reinterpret_cast<renderer::AnimatedMeshVertex*>(vertex_data) + vertex_index;
			vertex->position = mesh->vertices[vertex_index];
			vertex->normal = mesh->normals[vertex_index];
			vertex->uvs = mesh->uvs[vertex_index];
			vertex->blend_indices = mesh->blend_indices[vertex_index];
			vertex->blend_weights = mesh->blend_weights[vertex_index];
		}
	}


	void render_scene_track_mesh(RenderScene* scene, AssetHandle mesh_handle)
	{
		if (!render_scene_state->render_mesh_by_handle.has_key(mesh_handle))
		{
			Mesh* mesh = mesh_from_handle(mesh_handle);
			if (!mesh)
			{
				LOGW("Mesh (%i) is invalid. Skipping upload.\n", mesh_handle);
				return;
			}

			// upload to the GPU
			RenderMeshInfo* render_mesh = MEMORY2_NEW(*scene->allocator, RenderMeshInfo);
			render_mesh->vertex_buffer = nullptr;
			render_mesh->index_buffer = nullptr;

			bool is_animated_mesh = (mesh->skeleton.size() > 0);

			// static mesh only for now
			render2::VertexDescriptor descriptor;
			descriptor.add("in_position", render2::VD_FLOAT, 3);
			descriptor.add("in_normal", render2::VD_FLOAT, 3);
			descriptor.add("in_uv", render2::VD_FLOAT, 2);

			if (is_animated_mesh)
			{
				descriptor.add("in_blendindices", render2::VD_FLOAT, 4);
				descriptor.add("in_blendweights", render2::VD_FLOAT, 4);
			}

			uint32_t total_vertices = 0;
			uint32_t total_indices = 0;

			mesh_stats(mesh, total_vertices, total_indices);

			const size_t stride = render_scene_state->device->compute_vertex_stride(descriptor);
			const size_t vertex_buffer_size = total_vertices * stride;
			const size_t index_buffer_size = total_indices * render_scene_state->device->compute_index_stride();

			//LOGV("vertex_buffer_size: %i\n", vertex_buffer_size);
			//LOGV("index_buffer_size: %i\n", index_buffer_size);
			render_mesh->vertex_buffer = render_scene_state->device->create_vertex_buffer(vertex_buffer_size);
			render_mesh->index_buffer = render_scene_state->device->create_index_buffer(index_buffer_size);

			// interleave data for upload...
			char* vertex_data = static_cast<char*>(MEMORY2_ALLOC(*scene->allocator, vertex_buffer_size));
			if (!is_animated_mesh)
			{
				interleave_static_mesh(vertex_data, mesh, total_vertices);
			}
			else
			{
				interleave_animated_mesh(vertex_data, mesh, total_vertices);
			}

			render_scene_state->device->buffer_upload(render_mesh->vertex_buffer, vertex_data, vertex_buffer_size);
			MEMORY2_DEALLOC(*scene->allocator, vertex_data);

			// upload index data
			render_scene_state->device->buffer_upload(render_mesh->index_buffer, mesh->indices, index_buffer_size);

			render_scene_state->render_meshes.push_back(render_mesh);
			render_scene_state->render_mesh_by_handle.insert(std::pair<AssetHandle, RenderMeshInfo*>(mesh_handle, render_mesh));
		}
	} // render_scene_track_mesh


	uint32_t render_scene_add_animated_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform)
	{
		Mesh* mesh = mesh_from_handle(mesh_handle);
		if (!mesh)
		{
			LOGW("Mesh is not loaded for animated mesh!; ignoring\n");
			return 0;
		}

		AnimatedMeshComponent* component = MEMORY2_NEW(*scene->allocator, AnimatedMeshComponent)(*scene->allocator);
		component->entity_index = entity_index;
		component->mesh_handle = mesh_handle;
		component->model_matrix = model_transform;
		component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_transform)));
		component->bone_transforms = (glm::mat4*)MEMORY2_ALLOC(*scene->allocator, sizeof(glm::mat4) * MAX_BONES);
		component->current_sequence_index = 0;

		// iterate over all sequences and create sequence instances
		// for this AnimationMeshComponent.
		for (size_t sequence = 0; sequence < mesh->sequences.size(); ++sequence)
		{
			animation::AnimatedInstance* instance = animation::create_sequence_instance(*scene->allocator, mesh->sequences[sequence]);
			component->sequence_instances.push_back(instance);
		}

		for (size_t index = 0; index < mesh->skeleton.size(); ++index)
		{
			component->bone_transforms[index] = mesh->bind_poses[index] * glm::mat4(1.0f);
		}
		scene->animated_meshes.push_back(component);

		render_scene_track_mesh(scene, mesh_handle);

		return scene->animated_meshes.size();
	} // render_scene_add_animated_mesh

	uint32_t render_scene_add_static_mesh(RenderScene* scene, AssetHandle mesh_handle, uint16_t entity_index, const glm::mat4& model_transform)
	{
		Mesh* mesh = mesh_from_handle(mesh_handle);
		if (!mesh)
		{
			LOGW("Mesh is not loaded for static mesh!; ignoring\n");
			return 0;
		}

		Freelist<StaticMeshComponent*>::Handle component_handle = scene->static_meshes.acquire();
		StaticMeshComponent* component = MEMORY2_NEW(*scene->allocator, StaticMeshComponent);
		assert(component);
		scene->static_meshes.set(component_handle, component);
		component->entity_index = entity_index;
		component->mesh_handle = mesh_handle;
		component->model_matrix = model_transform;
		component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(model_transform)));

		render_scene_track_mesh(scene, mesh_handle);

		return component_handle;
	} // render_scene_add_static_mesh


	uint32_t render_scene_animation_play(RenderScene* scene, uint32_t component_id, const char* animation_name)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];

		Mesh* mesh = mesh_from_handle(component->mesh_handle);
		if (!mesh)
		{
			return 0;
		}

		if (!mesh->sequence_index_by_name.has_key(animation_name))
		{
			LOGW("No animation named '%s'\n", animation_name);
			return 0;
		}

		uint32_t instance_index = mesh->sequence_index_by_name[animation_name];
		animation::AnimatedInstance* instance = component->sequence_instances[instance_index];
		instance->flags = animation::AnimatedInstance::Flags::Playing;
		instance->local_time_seconds = 0.0f;
		//LOGV("playing animation: %s\n", animation_name);
		component->current_sequence_index = instance_index;

		return instance_index;
	}

	bool render_scene_animation_finished(RenderScene* scene, uint32_t component_id)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];
		animation::AnimatedInstance* instance = component->sequence_instances[component->current_sequence_index];
		assert(instance);
		if (instance)
		{
			return instance->is_finished();
		}

		return false;
	}

	bool render_scene_animation_is_playing(RenderScene* scene, uint32_t component_id, uint32_t instance_id)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];
		animation::AnimatedInstance* instance = component->sequence_instances[instance_id];
		// TODO: implement is playing!
		assert(0);
		return false;
	}

	uint32_t render_scene_animation_total_frames(RenderScene* scene, uint32_t component_id, uint32_t instance_index)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];
		animation::AnimatedInstance* instance = component->sequence_instances[component->current_sequence_index];
		assert(instance);
		if (instance)
		{
			animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
			return uint32_t(ceil(sequence->duration_seconds / sequence->frame_delay_seconds));
		}

		return 0;
	} // render_scene_animation_total_frames

	void render_scene_animation_set_frame(RenderScene* scene, uint32_t component_id, uint32_t frame)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];
		animation::AnimatedInstance* instance = component->sequence_instances[component->current_sequence_index];
		assert(instance);
		if (instance)
		{
			animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
			instance->local_time_seconds = sequence->frame_delay_seconds * frame;
		}
	} // render_scene_animation_set_frame

	uint32_t render_scene_animation_current_frame(RenderScene* scene, uint32_t component_id)
	{
		// If you hit this, an invalid component id was passed in
		assert(component_id > 0);
		component_id--;

		AnimatedMeshComponent* component = scene->animated_meshes[component_id];
		animation::AnimatedInstance* instance = component->sequence_instances[component->current_sequence_index];
		assert(instance);
		if (instance)
		{
			animation::Sequence* sequence = animation::get_sequence_by_index(instance->sequence_index);
			return uint32_t(instance->local_time_seconds / sequence->frame_delay_seconds);
		}

		return 0;
	} // render_scene_animation_current_frame


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

		// sky pipeline
		{
			render2::PipelineDescriptor desc;
			render2::VertexDescriptor& vertex_format = desc.vertex_description;
			vertex_format.add("in_position", render2::VD_FLOAT, 3);
			vertex_format.add("in_uv", render2::VD_FLOAT, 2);
			desc.shader = shader_load("sky");
			desc.primitive_type = render2::PrimitiveType::TriangleStrip;
			desc.input_layout = device->create_input_layout(desc.vertex_description, desc.shader);
			scene->sky_pipeline = device->create_pipeline(desc);
		}

		struct TestVertex
		{
			glm::vec3 position;
			glm::vec2 uv;
		};

		{
			const size_t total_vertex_bytes = sizeof(TestVertex) * 4;
			scene->screen_quad = device->create_vertex_buffer(total_vertex_bytes);

			const float width = static_cast<float>(device->default_render_target()->width);
			const float height = static_cast<float>(device->default_render_target()->height);

			//TestVertex vertex[6];
			//vertex[0].position = glm::vec3(-1.0, -1.0, 0.1f);
			//vertex[0].uv = glm::vec2(0.0f, 0.0f);

			//vertex[1].position = glm::vec3(1.0, -1.0, 0.1f);
			//vertex[1].uv = glm::vec2(width, 0.0f);

			//vertex[2].position = glm::vec3(1.0, 1.0, 0.1f);
			//vertex[2].uv = glm::vec2(width, height);

			//vertex[3].position = glm::vec3(1.0, 1.0, 0.1f);
			//vertex[3].uv = glm::vec2(height, height);

			//vertex[4].position = glm::vec3(-1.0, 1.0, 0.1f);
			//vertex[4].uv = glm::vec2(0.0f, height);

			//vertex[5].position = glm::vec3(-1.0, -1.0, 0.1f);
			//vertex[5].uv = glm::vec2(0.0f, 0.0f);

			TestVertex vertex[4];
			vertex[0].position = glm::vec3(-1.0, 1.0, 0.1f);
			vertex[0].uv = glm::vec2(0.0f, height);

			vertex[1].position = glm::vec3(-1.0, -1.0, 0.1f);
			vertex[1].uv = glm::vec2(0.0f, 0.0f);

			vertex[2].position = glm::vec3(1.0, 1.0, 0.1f);
			vertex[2].uv = glm::vec2(width, 0.0f);

			vertex[3].position = glm::vec3(1.0, -1.0, 0.1f);
			vertex[3].uv = glm::vec2(width, height);


			device->buffer_upload(scene->screen_quad, vertex, total_vertex_bytes);
		}

		return scene;
	} // render_scene_create

	void _render_scene_remove_animated_instances(RenderScene* scene, AnimatedMeshComponent* component)
	{
		if (!component)
		{
			return;
		}

		for (size_t sequence_index = 0; sequence_index < component->sequence_instances.size(); ++sequence_index)
		{
			animation::AnimatedInstance* instance = component->sequence_instances[sequence_index];
			animation::destroy_sequence_instance(*scene->allocator, instance);
		}

		MEMORY2_DEALLOC(*scene->allocator, component->bone_transforms);
	} // _render_scene_remove_animated_instances


	void render_scene_destroy(RenderScene* scene, render2::Device* device)
	{
		for (size_t index = 0; index < scene->static_meshes.size(); ++index)
		{
			StaticMeshComponent* component = scene->static_meshes.at(index);
			MEMORY2_DELETE(*scene->allocator, component);
		}

		for (size_t index = 0; index < scene->animated_meshes.size(); ++index)
		{
			AnimatedMeshComponent* component = scene->animated_meshes[index];
			_render_scene_remove_animated_instances(scene, component);
			MEMORY2_DELETE(*scene->allocator, component);
		}

		device->destroy_pipeline(scene->static_mesh_pipeline);
		device->destroy_pipeline(scene->animated_mesh_pipeline);
		device->destroy_pipeline(scene->sky_pipeline);

		device->destroy_buffer(scene->screen_quad);

		MEMORY2_DELETE(*scene->allocator, scene);
	} // render_scene_destroy


	void render_scene_draw(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::RenderTarget* render_target)
	{
		if (!render_target)
		{
			render_target = device->default_render_target();
		}

		scene->stat_static_meshes_drawn = 0;
		scene->stat_animated_meshes_drawn = 0;

		Color clear_color = Color::from_rgba(128, 128, 128, 255);

		// compute inverse projection and inverse view rotation matrix
		scene->inverse_view_rotation = glm::inverse(glm::mat3(view));
		scene->inverse_projection = glm::inverse(projection);


		//glm::vec3 xaxis = scene->inverse_view_rotation * glm::vec3(1.0f, 0.0f, 0.0f);
		//PRINT_VEC3(xaxis);

		render2::Pass sky_pass;
		sky_pass.target = render_target;
		sky_pass.color(clear_color.red, clear_color.blue, clear_color.green, clear_color.alpha);
		sky_pass.clear_color = true;
		sky_pass.clear_depth = true;
		sky_pass.depth_test = false;
		sky_pass.depth_write = true;
		sky_pass.cull_mode = render2::CullMode::None;
		render_sky(scene, device, view, projection, sky_pass);

		render2::Pass render_pass;
		render_pass.target = render_target;
		render_pass.color(clear_color.red, clear_color.blue, clear_color.green, clear_color.alpha);
		render_pass.clear_color = false;
		render_pass.clear_depth = false;
		render_pass.depth_test = true;
		render_pass.depth_write = true;
		render_pass.cull_mode = render2::CullMode::Backface;
		render_static_meshes(scene, device, view, projection, render_pass);

		render2::Pass animated_pass;
		animated_pass.target = render_target;
		animated_pass.clear_color = false;
		animated_pass.clear_depth = false;
		animated_pass.depth_test = true;
		animated_pass.depth_write = true;
		animated_pass.cull_mode = render2::CullMode::Backface;
		render_animated_meshes(scene, device, view, projection, animated_pass);
	} // render_scene_draw


	void _render_setup_material(render2::CommandSerializer* serializer, AssetHandle material_handle)
	{
		Material* material = material_from_handle(material_handle);
		for (size_t param_index = 0; param_index < material->parameters.size(); ++param_index)
		{
			renderer::MaterialParameter* parameter = &material->parameters[param_index];
			if (parameter->type == renderer::MP_SAMPLER_2D)
			{
				render2::Texture* texture = texture_from_handle(parameter->texture_handle);
				serializer->constant(parameter->name.c_str(), &parameter->texture_unit, sizeof(uint32_t));
				serializer->texture(texture, parameter->texture_unit);
			}
			else
			{
				LOGV("Unhandled material parameter: %i\n", parameter->type);
			}
		}
	} // _render_setup_material


	void render_static_meshes(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass)
	{
		render2::CommandQueue* queue = device->create_queue(pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		// render static meshes
		// setup the static mesh pipeline
		serializer->pipeline(scene->static_mesh_pipeline);
		scene->static_mesh_pipeline->constants().set("view_matrix", &view);
		scene->static_mesh_pipeline->constants().set("projection_matrix", &projection);
		scene->static_mesh_pipeline->constants().set("light_position_world", &scene->light_position_world);
		scene->static_mesh_pipeline->constants().set("camera_position_world", &scene->camera_position_world);
		scene->static_mesh_pipeline->constants().set("camera_view_direction", &scene->camera_view_direction);


		Freelist<StaticMeshComponent*>::Iterator iter = scene->static_meshes.begin();
		for (; iter != scene->static_meshes.end(); ++iter)
		{
			StaticMeshComponent* static_mesh = iter.data();
			assert(static_mesh);
			Mesh* mesh = mesh_from_handle(static_mesh->mesh_handle);
			if (mesh)
			{
				// If you hit this, the renderer has no reference to this mesh!
				// Are you sure it was uploaded?
				assert(render_scene_state->render_mesh_by_handle.has_key(static_mesh->mesh_handle));

				serializer->constant("model_matrix", &static_mesh->model_matrix, sizeof(glm::mat4));
				serializer->constant("normal_matrix", &static_mesh->normal_matrix, sizeof(glm::mat3));

				RenderMeshInfo* mesh_info = render_scene_state->render_mesh_by_handle[static_mesh->mesh_handle];
				serializer->vertex_buffer(mesh_info->vertex_buffer);

				// TODO: Convert into render blocks that can be re-sorted.
				for (size_t geo = 0; geo < mesh->geometry.size(); ++geo)
				{
					const GeometryDefinition* geometry = &mesh->geometry[geo];
					_render_setup_material(serializer, geometry->material_handle);
					serializer->draw_indexed_primitives(mesh_info->index_buffer, geometry->index_offset, geometry->total_indices);
				}

				++scene->stat_static_meshes_drawn;
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
		serializer->pipeline(scene->animated_mesh_pipeline);
		scene->animated_mesh_pipeline->constants().set("view_matrix", &view);
		scene->animated_mesh_pipeline->constants().set("projection_matrix", &projection);
		scene->animated_mesh_pipeline->constants().set("light_position_world", &scene->light_position_world);
		scene->animated_mesh_pipeline->constants().set("camera_position_world", &scene->camera_position_world);
		scene->animated_mesh_pipeline->constants().set("camera_view_direction", &scene->camera_view_direction);

		for (size_t index = 0; index < scene->animated_meshes.size(); ++index)
		{
			AnimatedMeshComponent* instance = scene->animated_meshes[index];
			if (!instance)
			{
				continue;
			}

			Mesh* mesh = mesh_from_handle(instance->mesh_handle);
			if (mesh)
			{
				// If you hit this, the renderer has no reference to this mesh!
				// Are you sure it was uploaded?
				assert(render_scene_state->render_mesh_by_handle.has_key(instance->mesh_handle));

				serializer->constant("model_matrix", &instance->model_matrix, sizeof(glm::mat4));
				serializer->constant("normal_matrix", &instance->normal_matrix, sizeof(glm::mat3));
				serializer->constant("node_transforms[0]", instance->bone_transforms, sizeof(glm::mat4) * MAX_BONES);
				serializer->constant("inverse_bind_transforms[0]", mesh->inverse_bind_poses, sizeof(glm::mat4) * MAX_BONES);

				RenderMeshInfo* mesh_info = render_scene_state->render_mesh_by_handle[instance->mesh_handle];
				serializer->vertex_buffer(mesh_info->vertex_buffer);

				// TODO: Convert into render blocks that can be re-sorted.
				for (size_t geo = 0; geo < mesh->geometry.size(); ++geo)
				{
					const GeometryDefinition* geometry = &mesh->geometry[geo];
					_render_setup_material(serializer, geometry->material_handle);
					serializer->draw_indexed_primitives(mesh_info->index_buffer, geometry->index_offset, geometry->total_indices);
				}

				++scene->stat_animated_meshes_drawn;

				// Enable this to debug bone transforms
#if 1
				if (mesh->skeleton.size() > 0)
				{
					glm::vec3 position; // render position
					glm::vec3 last_origin;
					glm::vec3 origins[MAX_BONES];
					const glm::mat4* model_poses = instance->bone_transforms;
					//const glm::vec3* hitboxes = model_instance->get_hitboxes();

					//for (size_t bone_index = 0; bone_index < mesh->skeleton.size(); ++bone_index)
					//{
					//	size_t transform_index = bone_index;
					//	const glm::mat4 world_pose = instance->model_matrix * model_poses[transform_index];
					//	debugdraw::axes(world_pose, 0.15f, 0.0f);
					//}

#if 1
					// draw individual links for each bone to represent the skeleton
					for (size_t bone_index = 0; bone_index < mesh->skeleton.size(); ++bone_index)
					{
						size_t transform_index = bone_index;
						Joint* joint = &mesh->skeleton[bone_index];
						if (joint->parent_index != -1)
						{
							last_origin = origins[joint->parent_index];
						}
						else
						{
							last_origin = glm::vec3(instance->model_matrix * glm::vec4(position, 1.0f));
						}

						const glm::mat4 world_pose = instance->model_matrix * model_poses[transform_index];
						glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));

						debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
						last_origin = origin;
						origins[bone_index] = origin;

						// this only displays local transforms
						debugdraw::axes(world_pose, 0.15f, 0.0f);

						//glm::vec3 box_center = origin;
						//const glm::vec3& positive_extents = hitboxes[bone_index];
						//debugdraw::box(box_center - positive_extents, box_center + positive_extents, gemini::Color(0.0f, 0.5f, 0.5f));
					}
#endif
				}
#endif
			}
		}

		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
	} // render_animated_meshes

	void render_scene_remove_static_mesh(RenderScene* scene, uint32_t component_id)
	{
		scene->static_meshes.release(component_id);
	} // remove_static_mesh

	void render_scene_remove_animated_mesh(RenderScene* scene, uint32_t component_id)
	{
		// We probably need to cleanup any instance associated with the component
		// as well. Implement that when you need it.
		AnimatedMeshComponent* component = scene->animated_meshes[component_id - 1];
		_render_scene_remove_animated_instances(scene, component);
		scene->animated_meshes[component_id - 1] = nullptr;
		MEMORY2_DELETE(*scene->allocator, component);
	} // remove_animated_mesh

	void render_sky(RenderScene* scene, render2::Device* device, const glm::mat4& view, const glm::mat4& projection, render2::Pass& pass)
	{
		render2::CommandQueue* queue = device->create_queue(pass);
		render2::CommandSerializer* serializer = device->create_serializer(queue);

		scene->viewport = glm::vec2(pass.target->width, pass.target->height);

		// render static meshes
		// setup the static mesh pipeline
		serializer->pipeline(scene->sky_pipeline);
		scene->sky_pipeline->constants().set("inverse_view_matrix", &scene->inverse_view_rotation);
		scene->sky_pipeline->constants().set("inverse_projection_matrix", &scene->inverse_projection);
		scene->sky_pipeline->constants().set("viewport", &scene->viewport);

		serializer->vertex_buffer(scene->screen_quad);
		serializer->draw(0, 4);

		device->queue_buffers(queue, 1);
		device->destroy_serializer(serializer);
	} // render_sky

	void _render_set_animation_pose(AnimatedMeshComponent* component, const animation::Pose& pose, float alpha)
	{
		// You've hit the upper bounds for skeletal bones for a single
		// model. Congrats.
		//assert(mesh->skeleton.size() < MAX_BONES);

		//Hitbox* hitboxes = &mesh->hitboxes[0];

		Mesh* mesh = mesh_from_handle(component->mesh_handle);
		if (!mesh)
		{
			return;
		}

		animation::Pose interpolated_pose;

		for (size_t index = 0; index < mesh->skeleton.size(); ++index)
		{
			Joint* joint = &mesh->skeleton[index];

#if 0
			interpolated_pose.pos[index] = pose.pos[index];
			interpolated_pose.rot[index] = pose.rot[index];
#else
			interpolated_pose.pos[index] = gemini::lerp(component->last_pose.pos[index], pose.pos[index], alpha);
			interpolated_pose.rot[index] = gemini::slerp(component->last_pose.rot[index], pose.rot[index], alpha);
#endif

			glm::mat4 parent_pose;
			glm::mat4 local_rotation = glm::toMat4(interpolated_pose.rot[index]);
			glm::mat4 local_transform = glm::translate(glm::mat4(1.0f), interpolated_pose.pos[index]);

			const glm::mat4 local_pose = local_transform * local_rotation;
			if (joint->parent_index > -1)
			{
				parent_pose = component->bone_transforms[joint->parent_index];
			}

			// this will be cached in local transforms
			glm::mat4 local_bone_pose = mesh->bind_poses[index] * local_pose;

			// this will be used for skinning in the vertex shader
			component->bone_transforms[index] = parent_pose * local_bone_pose;

			//glm::mat4 local_bbox_xf = glm::toMat4(glm::angleAxis(glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
			//Hitbox* hitbox = (hitboxes + index);
			//glm::vec3 pos(0.0f, 0.0f, 0.0f);
			//glm::vec3 dims(0.5f, 0.5f, 0.5f);
			//pos = mathlib::transform_point(local_bone_pose, pos);
			//debugdraw::box(-dims + pos, dims + pos, gemini::Color(0.0f, 1.0f, 1.0f));
			//debugdraw::axes(glm::mat4(hitbox->rotation) * model_pose, 1.0f, 0.0f);
		}

		component->last_pose = pose;
	} // _render_set_animation_pose


	//void mix_poses(glm::vec3* p0, glm::quat* r0, glm::vec3* p1, glm::quat* r1, glm::vec3* rpos, glm::quat* rrot, size_t total_bones, float alpha)
	//{
	//	for (size_t i = 0; i < total_bones; ++i)
	//	{
	//		glm::vec3& position = rpos[i];
	//		glm::quat& rotation = rrot[i];

	//		position = lerp(p0[i], p1[i], alpha);
	//		rotation = slerp(r0[i], r1[i], alpha);
	//	}
	//}


	void render_scene_update(RenderScene* scene, EntityRenderState* state, float step_alpha)
	{
		// extract data from static meshes
		Freelist<StaticMeshComponent*>::Iterator iter = scene->static_meshes.begin();
		for (; iter != scene->static_meshes.end(); ++iter)
		{
			StaticMeshComponent* component = iter.data();
			component->model_matrix = state->model_matrix[component->entity_index];
			component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(component->model_matrix)));
		}

		// extract data from animated meshes
		for (size_t index = 0; index < scene->animated_meshes.size(); ++index)
		{
			AnimatedMeshComponent* component = scene->animated_meshes[index];
			if (component)
			{
				animation::Pose pose;

				component->model_matrix = state->model_matrix[component->entity_index];
				component->normal_matrix = glm::transpose(glm::inverse(glm::mat3(component->model_matrix)));

				// TODO: determine how to get the blended pose; for now just use the first animated instance.
				animated_instance_get_pose(component->sequence_instances[component->current_sequence_index], pose);

				Mesh* mesh = mesh_from_handle(component->mesh_handle);
				if (!mesh)
				{
					return;
				}

				_render_set_animation_pose(component, pose, step_alpha);
			}
		}
	} // render_scene_update
} // namespace gemini
