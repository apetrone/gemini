// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include <algorithm>

#include "scenelink.h"

#include <renderer/renderstream.h>
#include <renderer/debug_draw.h>


#include <assets/asset_material.h>

#include <sdk/engine_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>

// for MAX_BONES
#include <shared/shared_constants.h>

#include <renderer/shader_library.h>

namespace gemini
{
	SceneLink::SceneLink(gemini::Allocator& _allocator)
		: allocator(_allocator)
	{
		queue = MEMORY2_NEW(allocator, ::renderer::RenderQueue);
	}

	SceneLink::~SceneLink()
	{
		MEMORY2_DELETE(allocator, queue);
	}

	void SceneLink::clear()
	{
		queue->clear();
	}

	void SceneLink::sort()
	{
		queue->sort();
	}

	void SceneLink::draw(::renderer::RenderStream& stream, glm::mat4* modelview_matrix, glm::mat4* projection_matrix)
	{
		// finally, draw from the queue
		for(::renderer::RenderBlock& block : queue->render_list)
		{
#if 0
			render2::Shader* shader = render2::shaders()->lookup(block.shader_id);
			assert(shader);

			// Must implement the new renderer!
			assert(0);
			//assets::Shader* shader = assets::shaders()->find_with_id(block.shader_id);

			assets::Material* material = assets::materials()->find_with_id(block.material_id);

			stream.add_shader(shader->program);

			stream.add_uniform_matrix4(shader->program->get_uniform_location("modelview_matrix"), modelview_matrix);
			stream.add_uniform_matrix4(shader->program->get_uniform_location("projection_matrix"), projection_matrix);
			stream.add_uniform_matrix4(shader->program->get_uniform_location("object_matrix"), block.object_matrix);

			if (block.total_transforms > 0)
			{
				// http://gamedev.stackexchange.com/questions/77854/how-can-i-reliably-implement-gpu-skinning-in-android
				// lovely, Android. There's a bug in Adreno where indexing into Uniform Matrices is wonky.
				// The solution is to index like a vec4.
				stream.add_uniform_matrix4(shader->program->get_uniform_location("node_transforms"), block.node_transforms, block.total_transforms);
				stream.add_uniform_matrix4(shader->program->get_uniform_location("inverse_bind_transforms"), block.inverse_bind_transforms, block.total_transforms);
			}
			else
			{
#if 0
				if (constant_buffer.viewer_direction)
				{
					rs.add_uniform3f(shader->program->get_uniform_location("viewer_direction"), constant_buffer.viewer_direction);
				}

				if (constant_buffer.viewer_position)
				{
					rs.add_uniform3f(shader->program->get_uniform_location("viewer_position"), constant_buffer.viewer_position);
				}

				if (constant_buffer.light_position)
				{
					rs.add_uniform3f(shader->program->get_uniform_location("light_position"), constant_buffer.light_position);
				}
#endif
			}

			stream.add_material(material, shader->program);

			stream.add_draw_call(block.object->vertexbuffer);
#endif
		}

		stream.run_commands();
	}

	void SceneLink::queue_entities(gemini::IEngineEntity** entity_list, uint32_t max_entities, uint32_t render_flags)
	{
		queue->clear();

		for (uint32_t i = 0; i < max_entities; ++i)
		{
			gemini::IEngineEntity* e = entity_list[i];

			// draw visible, non view model entities
			if (!e)
			{
				continue;
			}

			uint32_t eflags = e->get_render_flags();


			bool is_view_model = eflags & RENDER_VIEWMODEL;
			bool is_world_model = !is_view_model;
			bool is_visible = eflags & RENDER_VISIBLE;

			if (is_view_model && !(render_flags & RENDER_VIEWMODEL))
			{
				is_visible = false;
			}

			if ((is_visible && is_view_model && !is_world_model) || (is_visible && is_world_model && !is_view_model))
			{
				int32_t model_index = e->get_model_index();
				if (model_index > -1)
				{
//					LOGV("ent [%i], model_index = %i\n", i, model_index);
					// fetch model instance data

					gemini::IModelInstanceData* model_instance = engine::instance()->models()->get_instance_data(model_index);
					if (model_instance)
					{
						// TODO: determine if this is a static or animated mesh.

//						LOGV("model_instance: %i\n", model_instance->asset_index());
						assets::Mesh* mesh = assets::meshes()->find_with_id(model_instance->asset_index());
						if (mesh)
						{
							// queue this mesh
							for (size_t geometry_index = 0; geometry_index < mesh->geometry.size(); ++geometry_index)
							{
								assets::Geometry* geometry = mesh->geometry[geometry_index];

								// TODO: compute render key
								::renderer::RenderKey key = 0;

								// setup render block
								::renderer::RenderBlock block(key, geometry);

								// TODO: this should really happen in another step?
								glm::mat4 transform;
								glm::vec3 position;
								glm::quat orientation;
								glm::vec3 pivot_point;

								glm::vec3 physics_position;

								e->get_world_transform(physics_position, orientation);
								e->get_render_position(position);
								e->get_pivot_point(pivot_point);

								glm::mat4 rotation = glm::toMat4(orientation);
								glm::mat4 translation = glm::translate(transform, position);
								glm::mat4 to_pivot = glm::translate(glm::mat4(1.0f), -pivot_point);
								glm::mat4 from_pivot = glm::translate(glm::mat4(1.0f), pivot_point);
								transform = translation * from_pivot * rotation * to_pivot;

								model_instance->set_local_transform(transform);

								block.object_matrix = &model_instance->get_local_transform();
								block.material_id = geometry->material_id;
								block.shader_id = geometry->shader_id;
								block.node_transforms = model_instance->get_model_bone_transforms(geometry_index);
								block.total_transforms = model_instance->get_total_transforms();
								block.inverse_bind_transforms = model_instance->get_inverse_bind_transforms(geometry_index);

								queue->insert(block);

// Enable this to debug bone transforms
#if 0
								glm::vec3 last_origin;
								glm::vec3 origins[MAX_BONES];
								const glm::mat4* model_poses = model_instance->get_model_bone_transforms(geometry_index);
								const glm::vec3* hitboxes = model_instance->get_hitboxes();
								if (model_instance->get_total_transforms())
								{
									// draw individual links for each bone to represent the skeleton
									for (size_t index = 0; index < mesh->skeleton.size(); ++index)
									{
										size_t transform_index = (geometry_index * mesh->skeleton.size()) + index;
										assets::Joint* joint = &mesh->skeleton[index];
										if (joint->parent_index != -1)
										{
											last_origin = origins[joint->parent_index];
										}
										else
										{
											last_origin = position;
										}


										const glm::mat4 world_pose = transform * model_poses[transform_index];
										glm::vec3 origin = glm::vec3(glm::column(world_pose, 3));

										debugdraw::line(last_origin, origin, Color::from_rgba(255, 128, 0, 255));
										last_origin = origin;
										origins[index] = origin;

										// this only displays local transforms
										debugdraw::axes(world_pose, 0.15f, 0.0f);

										glm::vec3 box_center = origin;
										const glm::vec3& positive_extents = hitboxes[index];
										debugdraw::box(box_center - positive_extents, box_center + positive_extents, gemini::Color(0.0f, 0.5f, 0.5f));
									}
								}
#endif
							}

						}
					}
				}
			}
		}

		queue->sort();
	}
} // namespace gemini
