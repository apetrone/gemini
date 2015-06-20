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


#include <assets/asset_shader.h>
#include <assets/asset_material.h>

#include <sdk/debugdraw_api.h>
#include <sdk/engine_api.h>
#include <sdk/iengineentity.h>
#include <sdk/model_api.h>

namespace gemini
{
	SceneLink::SceneLink()
	{
		queue = MEMORY_NEW(::renderer::RenderQueue, platform::memory::global_allocator());
	}
	
	SceneLink::~SceneLink()
	{
		MEMORY_DELETE(queue, platform::memory::global_allocator());
	}
	
	void SceneLink::clear()
	{
		queue->clear();
	}
	
	void SceneLink::sort()
	{
		queue->sort();
	}
	
	void SceneLink::draw(::renderer::ConstantBuffer& constant_buffer)
	{
		::renderer::RenderStream rs;
		
		// finally, draw from the queue
		for(auto& block : queue->render_list)
		{
			assets::Shader* shader = assets::shaders()->find_with_id(block.shader_id);
			assets::Material* material = assets::materials()->find_with_id(block.material_id);
			
			//render_utilities::queue_geometry(rs, block, constant_buffer, 0, 0);
			
			
			
			rs.add_shader(shader->program);
			
			rs.add_uniform_matrix4(shader->program->get_uniform_location("modelview_matrix"), constant_buffer.modelview_matrix);
			rs.add_uniform_matrix4(shader->program->get_uniform_location("projection_matrix"), constant_buffer.projection_matrix);
			rs.add_uniform_matrix4(shader->program->get_uniform_location("object_matrix"), block.object_matrix);
			
			if (block.total_transforms > 0)
			{
				// http://gamedev.stackexchange.com/questions/77854/how-can-i-reliably-implement-gpu-skinning-in-android
				// lovely, Android. There's a bug in Adreno where indexing into Uniform Matrices is wonky.
				// The solution is to index like a vec4.
				rs.add_uniform_matrix4(shader->program->get_uniform_location("node_transforms"), block.node_transforms, block.total_transforms);
			}
			else
			{
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
			}
			
			rs.add_material(material, shader->program);
			
			rs.add_draw_call(block.object->vertexbuffer);
		}
		
		rs.run_commands();
	}
	
	void SceneLink::queue_entities(::renderer::ConstantBuffer& constant_buffer, gemini::IEngineEntity** entity_list, uint32_t max_entities, uint32_t render_flags)
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
								assets::Geometry* geometry = &mesh->geometry[geometry_index];
								
								// TODO: compute render key
								::renderer::RenderKey key = 0;
								
								// setup render block
								::renderer::RenderBlock block(key, geometry);
								
								// TODO: this should really happen in another step?
								glm::mat4 transform;
								glm::vec3 position;
								glm::quat orientation;
								
								glm::vec3 physics_position;
								
								e->get_world_transform(physics_position, orientation);
								e->get_render_position(position);

								glm::mat4 rotation = glm::toMat4(orientation);
								glm::mat4 translation = glm::translate(transform, position);
								transform = translation*rotation;
								
								model_instance->set_local_transform(transform);
								
								block.object_matrix = &model_instance->get_local_transform();
								block.material_id = geometry->material_id;
								block.shader_id = geometry->shader_id;
								block.node_transforms = model_instance->get_bone_transforms(geometry_index);
								block.total_transforms = model_instance->get_total_transforms();
									
								queue->insert(block);
							}


							// draw bone transforms as axes
							// TODO: this should be moved elsewhere or turned into a special debug render block?
#if 0
							const glm::mat4* debug_skeletal_pose = model_instance->get_debug_bone_transforms();
							for (size_t i = 0; i < model_instance->get_total_transforms(); ++i)
							{
								debugdraw::instance()->axes(debug_skeletal_pose[i], 0.05f, 0.0f);
							}
#endif
						}
					}
				}
			}
		}
		
		queue->sort();
	}
} // namespace gemini