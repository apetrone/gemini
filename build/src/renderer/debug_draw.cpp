// -------------------------------------------------------------
// Copyright (C) 2012- Adam Petrone
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
#include <core/mem.h>

#include <core/logging.h>

#include <runtime/asset_handle.h>

#include <renderer/debug_draw.h>
#include <renderer/renderer.h>
#include <renderer/renderstream.h>
#include <renderer/font.h>
#include <renderer/color.h>
#include <renderer/shader_library.h>


// Enable this to temporarily make all debug draw functions a no-op.
//#define DISABLE_DEBUG_DRAW 1

namespace debugdraw
{
	const size_t DEBUGDRAW_PERSISTENT_PRIMITIVE_MAX = 16;

	// if you modify this, you must also update the buffer_primitive_table.
	enum
	{
		TYPE_BOX=1,
		TYPE_LINE,
		TYPE_AXES,
		TYPE_SPHERE,
		TYPE_OBB,
		TYPE_CAMERA,

		TYPE_TEXT,
		TYPE_TRIANGLE,

		TYPE_COUNT
	};

	struct DebugPrimitive
	{
		short type; // type of primitive
		short flags;
		float timeleft; // the time left until this object is no longer rendered
		float radius;
		glm::vec3 start;
		glm::vec3 end;
		glm::vec3 alt;
		gemini::Color color;

		// for text
		core::StackString<1024> buffer;
		glm::mat4 transform;

		DebugPrimitive();
		void reset();
	}; // DebugPrimitive

	struct DebugDrawVertex
	{
		glm::vec3 position;
		gemini::Color color;

		void set_position(float x, float y, float z)
		{
			position.x = x;
			position.y = y;
			position.z = z;
		}
	}; // DebugDrawVertex

	struct TexturedVertex : public DebugDrawVertex
	{
		glm::vec2 uv;

		void set_uv(float u, float v)
		{
			uv.s = u;
			uv.t = v;
		}
	}; // TexturedVertex

	DebugPrimitive::DebugPrimitive() :
		type(0),
		flags(0),
		timeleft(0.0f),
		radius(0.0f),
		start(glm::vec3(0.0f, 0.0f, 0.0f)),
		end(glm::vec3(0.0f, 0.0f, 0.0f)),
		alt(glm::vec3(0.0f, 0.0f, 0.0f)),
		color(gemini::Color()),
		buffer(""),
		transform(glm::mat4(1.0f))
	{
	} // DebugPrimitive

	void DebugPrimitive::reset()
	{

	} // DebugPrimitive

	namespace detail
	{
		template <class T>
		class VertexAccessor
		{
		public:
			VertexAccessor(Array<T>& vertices) :
				vertex_cache(vertices),
				current_index(0)
			{
			}

			// should not be assigning this.
			VertexAccessor<T>& operator=(VertexAccessor<T>& other) = delete;

			T* request(size_t vertices)
			{
				// determine if we can accommodate <vertices>
				if (current_index + vertices <= vertex_cache.size())
				{
					T* vertex = &vertex_cache[current_index];
					current_index += vertices;
					return vertex;
				}

				// If you hit this assert, there's no more room in the
				// vertex cache for vertices!
				assert(0);
				return nullptr;
			}

		private:
			Array<T>& vertex_cache;
			size_t current_index;
		};


		// The primitive cache keeps two arrays of primitives
		// Per-frame primitives: These are only kept active for a single
		// frame. This array is unbounded and grows to fill the need.
		// This can be used for debugging axes that move every frame
		// or text that changes frequently.

		// Persistent primitives: These survive for multiple frames,
		// but there are a limited number of these.

		class PrimitiveCache
		{
		public:
			PrimitiveCache(gemini::Allocator& allocator)
				: next_primitive(0)
				, per_frame_primitives(allocator)
				, persistent_primitives(allocator)
			{
				persistent_primitives.resize(
					DEBUGDRAW_PERSISTENT_PRIMITIVE_MAX);
			}

			~PrimitiveCache()
			{
				per_frame_primitives.clear();
				persistent_primitives.clear();
			}

			debugdraw::DebugPrimitive* request(bool persistent = false)
			{
				if (!persistent)
				{
					DebugPrimitive new_primitive;
					per_frame_primitives.push_back(new_primitive);
					return &per_frame_primitives.back();
				}

				// wrap this around
				if (next_primitive >= DEBUGDRAW_PERSISTENT_PRIMITIVE_MAX-1)
				{
					next_primitive = 0;
				}

				return &persistent_primitives[ next_primitive++ ];
			}

			void update(float delta_msec)
			{
				// run an update for each active primitive
				for(size_t index = 0; index < persistent_primitives.size(); ++index)
				{
					DebugPrimitive* primitive = &persistent_primitives[index];
					// if timeleft has expired, reset it and disable the primitive by
					// setting an invalid type
					if ((primitive->type > 0) && (primitive->timeleft < 0))
					{
						primitive->timeleft = -1;
						primitive->type = 0;
					}

					if ((primitive->type > 0) && (primitive->timeleft >= 0))
					{
						// timeleft has a value, subtract deltatime
						primitive->timeleft -= delta_msec;
					}
				}
			}

			// called at the end of a frame
			void reset()
			{
				// reset all of these for the next frame
				for (size_t index = 0; index < per_frame_primitives.size(); ++index)
				{
					per_frame_primitives[index] = DebugPrimitive();
				}

				per_frame_primitives.clear(false);
			}

			Array<DebugPrimitive>& get_per_frame_primitives() { return per_frame_primitives; }
			Array<DebugPrimitive>& get_persistent_primitives() { return persistent_primitives; }

		private:
			size_t next_primitive;

			Array<DebugPrimitive> per_frame_primitives;
			Array<DebugPrimitive> persistent_primitives;
		};
	}


	const size_t MAX_CIRCLE_SIDES = 12;
	const size_t TOTAL_CIRCLE_VERTICES = 2 * MAX_CIRCLE_SIDES;

	gemini::Allocator* _allocator = nullptr;

	detail::PrimitiveCache* line_list = nullptr;
	detail::PrimitiveCache* tris_list = nullptr;
	detail::PrimitiveCache* text_list = nullptr;

	render2::Device* device = nullptr;

	render2::Pipeline* line_pipeline = nullptr;
	render2::Buffer* line_buffer = nullptr;
	Array<DebugDrawVertex>* line_vertex_cache;

	render2::Pipeline* tris_pipeline = nullptr;
	render2::Buffer* tris_buffer = nullptr;
	Array<TexturedVertex>* tris_vertex_cache;
	render2::Texture* white_texture = nullptr;

	render2::Pipeline* text_pipeline = nullptr;
	render2::Buffer* text_buffer = nullptr;
	Array<font::FontVertex>* text_vertex_cache;

	font::Handle text_handle;
	glm::mat4 orthographic_projection;

	glm::mat4 modelview_matrix;
	glm::mat4 projection_matrix;

	// this needs to be persistent due to the way
	// values are stored in the constant maps
	int diffuse_texture_unit = 0;

	// PLANE = 0: (XY plane)
	// PLANE = 1: (XZ plane)
	// PLANE = 2: (YZ plane)
	static void generate_circle(const glm::vec3& origin, glm::vec3* vertices, int num_sides, float radius, int plane)
	{
		float degree_step = (360/(float)num_sides);
		float radians;
		glm::vec3 * vertex;
		glm::vec3 * second = 0;
		int circle_step = 0;
		num_sides *= 2;
		float ca, sa;

		for( int v = 0; v < num_sides; ++v )
		{
			vertex = &vertices[v];

			if ( second )
			{
				memcpy( vertex, second, sizeof(glm::vec3) );
				second = 0;
				v++;
				vertex = &vertices[v];
			}

			radians = mathlib::degrees_to_radians(circle_step * degree_step);
			ca = (radius * cos(radians));
			sa = (radius * sin(radians));
			if ( plane == 0 )
			{
				vertex->x = ca + origin[0];
				vertex->y = sa + origin[1];
				vertex->z = origin[2];
			}
			else if ( plane == 1 )
			{
				vertex->x = ca + origin[0];
				vertex->y = origin[1];
				vertex->z = sa + origin[2];
			}
			else if ( plane == 2 )
			{
				vertex->x = origin[0];
				vertex->y = ca + origin[1];
				vertex->z = sa + origin[2];
			}
			if ( v > 0 )
			{
				second = vertex;
			}
			circle_step += 1;
		}
	} // generate_circle

	// should return the number of vertices used
	typedef void (*buffer_primitive_fn)(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access);

	void buffer_box(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = 24;
		DebugDrawVertex* vertices = access.request(vertex_count);

		glm::vec3 mins = primitive->start;
		glm::vec3 maxs = primitive->end;

		for(size_t index = 0; index < vertex_count; ++index)
		{
			vertices[index].color = primitive->color;
		}

		// -Z face
		vertices[0].position = mins;
		vertices[1].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[2].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[3].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[4].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[5].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[6].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[7].position = mins;

		// +Z face
		vertices[8].position = glm::vec3( mins[0], mins[1], maxs[2] );
		vertices[9].position = glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[10].position = glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[11].position = glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[12].position = glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[13].position = glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[14].position = glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[15].position = glm::vec3( mins[0], mins[1], maxs[2] );

		// lower left
		vertices[16].position = mins;
		vertices[17].position = glm::vec3( mins[0], mins[1], maxs[2] );

		// lower right
		vertices[18].position = glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[19].position = glm::vec3( maxs[0], mins[1], maxs[2] );

		// upper right
		vertices[20].position = glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[21].position = glm::vec3( maxs[0], maxs[1], maxs[2] );

		// upper left
		vertices[22].position = glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[23].position = glm::vec3( mins[0], maxs[1], maxs[2] );
	} // buffer_box

	void buffer_line(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = 2;
		DebugDrawVertex* vertices = access.request(vertex_count);
		vertices[0].color = primitive->color;
		vertices[0].position = primitive->start;
		vertices[1].color = primitive->color;
		vertices[1].position = primitive->end;
	} // buffer_line

	void buffer_axes(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = 6;
		DebugDrawVertex* vertices = access.request(vertex_count);

		const gemini::Color X_AXIS_COLOR(1.0f, 0.0f, 0.0f);
		const gemini::Color Y_AXIS_COLOR(0.0f, 1.0f, 0.0f);
		const gemini::Color Z_AXIS_COLOR(0.0f, 0.0f, 1.0f);

		glm::vec3 right = glm::vec3(primitive->transform * glm::vec4(primitive->radius, 0.0f, 0.0f, 1.0f));
		vertices[0].position = primitive->start;
		vertices[1].position = right;
		vertices[0].color = X_AXIS_COLOR;
		vertices[1].color = X_AXIS_COLOR;

		glm::vec3 up = glm::vec3(primitive->transform * glm::vec4(0.0f, primitive->radius, 0.0f, 1.0f));
		vertices[2].position = primitive->start;
		vertices[3].position = up;
		vertices[2].color = Y_AXIS_COLOR;
		vertices[3].color = Y_AXIS_COLOR;

		glm::vec3 view = glm::vec3(primitive->transform * glm::vec4(0.0f, 0.0f, primitive->radius, 1.0f));
		vertices[4].position = primitive->start;
		vertices[5].position = view;
		vertices[4].color = Z_AXIS_COLOR;
		vertices[5].color = Z_AXIS_COLOR;
	} // buffer_axes

	void buffer_sphere(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = (TOTAL_CIRCLE_VERTICES * 3);
		DebugDrawVertex* vertices = access.request(vertex_count);

		glm::vec3 vlist[ TOTAL_CIRCLE_VERTICES ];

		// XY plane
		generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 0 );
		for(size_t i = 0; i < TOTAL_CIRCLE_VERTICES; ++i)
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}

		// XZ plane
		generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 1 );
		for(size_t i = 0; i < TOTAL_CIRCLE_VERTICES; ++i)
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}

		// YZ plane
		generate_circle( primitive->start, vlist, MAX_CIRCLE_SIDES, primitive->radius, 2 );
		for(size_t i = 0; i < TOTAL_CIRCLE_VERTICES; ++i)
		{
			vertices[0].position = vlist[ i ];
			vertices[0].color = primitive->color;
			++vertices;
		}
	} // buffer_sphere

	void buffer_obb(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = 24;
		DebugDrawVertex* vertices = access.request(vertex_count);

		glm::vec3 mins = primitive->start + -primitive->end;
		glm::vec3 maxs = primitive->start + primitive->end;

		for(size_t index = 0; index < vertex_count; ++index)
		{
			vertices[index].color = primitive->color;
		}

		glm::mat3 transform = glm::mat3(primitive->transform);

		// -Z face
		vertices[0].position = transform * mins;
		vertices[1].position = transform * glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[2].position = transform * glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[3].position = transform * glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[4].position = transform * glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[5].position = transform * glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[6].position = transform * glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[7].position = transform * mins;

		// +Z face
		vertices[8].position = transform * glm::vec3( mins[0], mins[1], maxs[2] );
		vertices[9].position = transform * glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[10].position = transform * glm::vec3( maxs[0], mins[1], maxs[2] );
		vertices[11].position = transform * glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[12].position = transform * glm::vec3( maxs[0], maxs[1], maxs[2] );
		vertices[13].position = transform * glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[14].position = transform * glm::vec3( mins[0], maxs[1], maxs[2] );
		vertices[15].position = transform * glm::vec3( mins[0], mins[1], maxs[2] );

		// lower left
		vertices[16].position = transform * mins;
		vertices[17].position = transform * glm::vec3( mins[0], mins[1], maxs[2] );

		// lower right
		vertices[18].position = transform * glm::vec3( maxs[0], mins[1], mins[2] );
		vertices[19].position = transform * glm::vec3( maxs[0], mins[1], maxs[2] );

		// upper right
		vertices[20].position = transform * glm::vec3( maxs[0], maxs[1], mins[2] );
		vertices[21].position = transform * glm::vec3( maxs[0], maxs[1], maxs[2] );

		// upper left
		vertices[22].position = transform * glm::vec3( mins[0], maxs[1], mins[2] );
		vertices[23].position = transform * glm::vec3( mins[0], maxs[1], maxs[2] );
	} // buffer_obb

	void buffer_camera(DebugPrimitive* primitive, detail::VertexAccessor<DebugDrawVertex>& access)
	{
		const size_t vertex_count = 16;
		DebugDrawVertex* vertices = access.request(vertex_count);

		// for now; just a 16:9 ratio.
		const glm::vec2 frame_dimensions(1.77f, 1.0f);
		const glm::vec2 half_dim = (frame_dimensions * 0.5f);
		const gemini::Color frame_color(1.0f, 0.0f, 0.0f, 1.0f);
		const gemini::Color frustum_color(0.0f, 1.0f, 0.0f, 1.0f);

		const glm::vec3& origin = primitive->start;
		const glm::vec3& view = -glm::normalize(primitive->end);

		// First we need to establish the basis matrix given the view vector.
		const glm::vec3 right = glm::normalize(glm::cross(view, glm::vec3(0.0f, 1.0f, 0.0f)));

		// Calculate the true up vector.
		const glm::vec3 up = glm::normalize(glm::cross(right, view));

		const float forward_distance = -1.0f; // distance from origin to near clip plane

		glm::mat4 basis;

		basis[0] = glm::vec4(right.x, right.y, right.z, 0);
		basis[1] = glm::vec4(up.x, up.y, up.z, 0);
		basis[2] = glm::vec4(view.x, view.y, view.z, 0);
		basis[3] = glm::vec4(origin.x, origin.y, origin.z, 0);

		// ccw draw the frame, starting in the lower left corner from the camera's
		// point of view looking into the scene.

		// lower left to lower right
		vertices[0].position = glm::vec3(-half_dim.x, -half_dim.y, forward_distance);
		vertices[0].color = frame_color;
		vertices[1].position = glm::vec3(half_dim.x, -half_dim.y, forward_distance);
		vertices[1].color = frame_color;

		// lower right to upper right
		vertices[2].position = glm::vec3(half_dim.x, -half_dim.y, forward_distance);
		vertices[2].color = frame_color;
		vertices[3].position = glm::vec3(half_dim.x, half_dim.y, forward_distance);
		vertices[3].color = frame_color;

		// upper right to upper left
		vertices[4].position = glm::vec3(half_dim.x, half_dim.y, forward_distance);
		vertices[4].color = frame_color;
		vertices[5].position = glm::vec3(-half_dim.x, half_dim.y, forward_distance);
		vertices[5].color = frame_color;

		// upper left to lower left
		vertices[6].position = glm::vec3(-half_dim.x, half_dim.y, forward_distance);
		vertices[6].color = frame_color;
		vertices[7].position = glm::vec3(-half_dim.x, -half_dim.y, forward_distance);
		vertices[7].color = frame_color;


		// draw corners
		vertices[8].position = glm::vec3(-half_dim.x, -half_dim.y, forward_distance);
		vertices[8].color = frustum_color;
		vertices[9].position = glm::vec3(0.0f, 0.0f, 0.0f);
		vertices[9].color = frustum_color;

		vertices[10].position = glm::vec3(half_dim.x, -half_dim.y, forward_distance);
		vertices[10].color = frustum_color;
		vertices[11].position = glm::vec3(0.0f, 0.0f, 0.0f);
		vertices[11].color = frustum_color;

		vertices[12].position = glm::vec3(half_dim.x, half_dim.y, forward_distance);
		vertices[12].color = frustum_color;
		vertices[13].position = glm::vec3(0.0f, 0.0f, 0.0f);
		vertices[13].color = frustum_color;

		vertices[14].position = glm::vec3(-half_dim.x, half_dim.y, forward_distance);
		vertices[14].color = frustum_color;
		vertices[15].position = glm::vec3(0.0f, 0.0f, 0.0f);
		vertices[15].color = frustum_color;

		// transform all vertices
		for (size_t index = 0; index < vertex_count; ++index)
		{
			vertices[index].position = glm::vec3(basis * glm::vec4(vertices[index].position, 1.0f));
		}
	} // buffer_camera

	void buffer_triangle(DebugPrimitive* primitive, detail::VertexAccessor<TexturedVertex>& access)
	{
		const size_t vertex_count = 3;
		TexturedVertex* vertices = access.request(vertex_count);
		vertices[0].position = primitive->start;
		vertices[0].set_uv(0, 0);

		vertices[1].position = primitive->end;
		vertices[1].set_uv(1.0, 0.0f);

		vertices[2].position = primitive->alt;
		vertices[2].set_uv(1.0f, 1.0f);

		vertices[0].color = primitive->color;
		vertices[1].color = primitive->color;
		vertices[2].color = primitive->color;
	} // buffer_triangle

	void startup(gemini::Allocator& allocator, render2::Device* render_device)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		_allocator = &allocator;
		line_list = MEMORY2_NEW(allocator, detail::PrimitiveCache)(allocator);
		tris_list = MEMORY2_NEW(allocator, detail::PrimitiveCache)(allocator);
		text_list = MEMORY2_NEW(allocator, detail::PrimitiveCache)(allocator);

		line_vertex_cache = MEMORY2_NEW(allocator, Array<DebugDrawVertex>)(allocator);
		tris_vertex_cache = MEMORY2_NEW(allocator, Array<TexturedVertex>)(allocator);
		text_vertex_cache = MEMORY2_NEW(allocator, Array<font::FontVertex>)(allocator);

		device = render_device;

		Array<unsigned char> fontdata(allocator);
		const render2::ResourceProvider* resource_provider = render2::get_resource_provider();
		resource_provider->load_file(fontdata, "fonts/debug.ttf");
		text_handle = font::load_from_memory(&fontdata[0], fontdata.size(), 16);
		assert(text_handle.is_valid());

		// create buffers for line, triangles, and font
		line_buffer = device->create_vertex_buffer(0);
		tris_buffer = device->create_vertex_buffer(0);
		text_buffer = device->create_vertex_buffer(0);

		// line pipeline
		{
			render2::PipelineDescriptor descriptor;
			gemini::AssetHandle handle = render2::shaders()->load("debug");
			descriptor.shader = render2::shaders()->lookup(handle);
			descriptor.vertex_description.add("in_position", render2::VD_FLOAT, 3);
			descriptor.vertex_description.add("in_color", render2::VD_FLOAT, 4);
			descriptor.input_layout = device->create_input_layout(descriptor.vertex_description, descriptor.shader);
			descriptor.enable_blending = true;
			descriptor.blend_source = render2::BlendOp::SourceAlpha;
			descriptor.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
			descriptor.primitive_type = render2::PrimitiveType::Lines;
			line_pipeline = device->create_pipeline(descriptor);
		}

		// triangle pipeline
		{
			render2::PipelineDescriptor descriptor;
			gemini::AssetHandle handle = render2::shaders()->load("vertexcolortexture");
			descriptor.shader = render2::shaders()->lookup(handle);
			descriptor.vertex_description.add("in_position", render2::VD_FLOAT, 3);
			descriptor.vertex_description.add("in_color", render2::VD_FLOAT, 4);
			descriptor.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
			descriptor.input_layout = device->create_input_layout(descriptor.vertex_description, descriptor.shader);
			descriptor.enable_blending = true;
			descriptor.blend_source = render2::BlendOp::SourceAlpha;
			descriptor.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
			descriptor.primitive_type = render2::PrimitiveType::Triangles;
			tris_pipeline = device->create_pipeline(descriptor);

			render2::Image white_image(allocator);
			white_image.create(4, 4, 3);
			white_image.filter = image::FILTER_NONE;
			white_image.flags = image::F_CLAMP_BORDER;
			white_image.fill(gemini::Color());
			white_texture = device->create_texture(white_image);
		}

		// font pipeline
		{
			render2::PipelineDescriptor descriptor;
			gemini::AssetHandle handle = render2::shaders()->load("font");
			descriptor.shader = render2::shaders()->lookup(handle);
			descriptor.vertex_description.add("in_position", render2::VD_FLOAT, 2);
			descriptor.vertex_description.add("in_color", render2::VD_FLOAT, 4);
			descriptor.vertex_description.add("in_uv", render2::VD_FLOAT, 2);
			descriptor.input_layout = device->create_input_layout(descriptor.vertex_description, descriptor.shader);
			descriptor.enable_blending = true;
			descriptor.blend_source = render2::BlendOp::SourceAlpha;
			descriptor.blend_destination = render2::BlendOp::OneMinusSourceAlpha;
			descriptor.primitive_type = render2::PrimitiveType::Triangles;
			text_pipeline = device->create_pipeline(descriptor);
		}
	}

	void shutdown()
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		device->destroy_texture(white_texture);

		if (line_buffer)
		{
			device->destroy_buffer(line_buffer);
		}

		if (tris_buffer)
		{
			device->destroy_buffer(tris_buffer);
		}

		if (text_buffer)
		{
			device->destroy_buffer(text_buffer);
		}

		if (line_pipeline)
		{
			device->destroy_pipeline(line_pipeline);
		}

		if (tris_pipeline)
		{
			device->destroy_pipeline(tris_pipeline);
		}

		if (text_pipeline)
		{
			device->destroy_pipeline(text_pipeline);
		}

		device = nullptr;

		MEMORY2_DELETE(*_allocator, line_vertex_cache);
		MEMORY2_DELETE(*_allocator, tris_vertex_cache);
		MEMORY2_DELETE(*_allocator, text_vertex_cache);

		MEMORY2_DELETE(*_allocator, line_list);
		MEMORY2_DELETE(*_allocator, tris_list);
		MEMORY2_DELETE(*_allocator, text_list);

		_allocator = nullptr;
	}

	void update(float delta_msec)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		line_list->update(delta_msec);
		tris_list->update(delta_msec);
		text_list->update(delta_msec);
	}

	size_t draw_lines(const render2::Pass& pass)
	{
		buffer_primitive_fn buffer_primitive_table[] =
		{
			0,
			buffer_box,
			buffer_line,
			buffer_axes,
			buffer_sphere,
			buffer_obb,
			buffer_camera
		};

		size_t primitive_sizes[] =
		{
			0,
			24, // box
			2, // line
			6, // axes
			(TOTAL_CIRCLE_VERTICES * 3), // sphere
			24, // obb
			16, // camera
		};

		// If you hit this assert, you have a mismatch between the above
		// tables.
		assert(sizeof(buffer_primitive_table) == sizeof(primitive_sizes));

		// step 1: tally up the total vertex cache size we'll need
		size_t total_vertices_required = 0;

		// persistent primitives; if any exist
		Array<DebugPrimitive>& persistent_primitives = line_list->get_persistent_primitives();
		for (size_t index = 0; index < persistent_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &persistent_primitives[index];
			if (primitive->type != 0)
			{
				total_vertices_required += primitive_sizes[primitive->type];
			}
		}

		// per frame primitives
		Array<DebugPrimitive>& per_frame_primitives = line_list->get_per_frame_primitives();
		for(size_t index = 0; index < per_frame_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &per_frame_primitives[index];
			if (primitive->type != 0)
			{
				total_vertices_required += primitive_sizes[primitive->type];
			}
		}

		if (total_vertices_required > 0)
		{
			// step 2: resize the vertex cache
			line_vertex_cache->resize(total_vertices_required);

			// and the vertex buffer
			device->buffer_resize(line_buffer, sizeof(DebugDrawVertex) * total_vertices_required);

			detail::VertexAccessor<DebugDrawVertex> accessor(*line_vertex_cache);

			// step 3: build the vertex cache
			// persistent primitives
			for (size_t index = 0; index < persistent_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &persistent_primitives[index];
				if (primitive->type > 0 && primitive->type <= TYPE_CAMERA)
				{
					buffer_primitive_table[primitive->type](primitive,
						accessor);
				}
			}

			// per frame
			for(size_t index = 0; index < per_frame_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &per_frame_primitives[index];
				if (primitive->type > 0 && primitive->type <= TYPE_CAMERA)
				{
					buffer_primitive_table[primitive->type](primitive,
						accessor);
				}
			}

			assert(total_vertices_required % 2 == 0);

			device->buffer_upload(
				line_buffer,
				&(*line_vertex_cache)[0],
				sizeof(DebugDrawVertex) * total_vertices_required);

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);

			// serialize all lines first
			serializer->pipeline(line_pipeline);
			serializer->vertex_buffer(line_buffer);
			serializer->draw(0, total_vertices_required);
			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}

		line_vertex_cache->clear(false);
		line_list->reset();

		return total_vertices_required / 2;
	} // draw_lines

	size_t draw_triangles(const render2::Pass& pass)
	{
		size_t total_vertices_required = 0;

		// step 1: loop through all primitives and count total vertices needed
		Array<DebugPrimitive>& persistent_primitives = tris_list->get_persistent_primitives();
		for (size_t index = 0; index < persistent_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &persistent_primitives[index];
			if (primitive->type == TYPE_TRIANGLE)
			{
				total_vertices_required += 3;
			}
		}

		Array<DebugPrimitive>& per_frame_primitives = tris_list->get_per_frame_primitives();
		for (size_t index = 0; index < per_frame_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &per_frame_primitives[index];
			if (primitive->type == TYPE_TRIANGLE)
			{
				total_vertices_required += 3;
			}
		}

		if (total_vertices_required > 0)
		{
			// step 2: resize the cache
			tris_vertex_cache->resize(total_vertices_required);

			assert(total_vertices_required % 3 == 0);
			const size_t new_vertexbuffer_size = sizeof(TexturedVertex) * total_vertices_required;

			// resize the buffer
			device->buffer_resize(tris_buffer, new_vertexbuffer_size);
			detail::VertexAccessor<TexturedVertex> accessor(*tris_vertex_cache);

			// step 3: build the vertex cache
			// persistent primitives
			for (size_t index = 0; index < persistent_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &persistent_primitives[index];
				if (primitive->type != 0)
				{
					buffer_triangle(primitive, accessor);
				}
			}

			// per frame
			for(size_t index = 0; index < per_frame_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &per_frame_primitives[index];
				if (primitive->type != 0)
				{
					buffer_triangle(primitive, accessor);
				}
			}

			device->buffer_upload(
				tris_buffer,
				&(*tris_vertex_cache)[0],
				new_vertexbuffer_size);

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);

			serializer->pipeline(tris_pipeline);
			serializer->vertex_buffer(tris_buffer);
			serializer->texture(white_texture, 0);
			serializer->draw(0, total_vertices_required);
			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}

		tris_vertex_cache->clear(false);
		tris_list->reset();

		return total_vertices_required / 3;
	} // draw_triangles

	size_t draw_text(const render2::Pass& pass)
	{
		size_t total_vertices_required = 0;

		// loop through all primitives and count total vertices needed
		Array<DebugPrimitive>& persistent_primitives = text_list->get_persistent_primitives();
		for (size_t index = 0; index < persistent_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &persistent_primitives[index];
			if (primitive->type == TYPE_TEXT)
			{
				// 6 vertices per character
				// this is not even utf-8; so this will probably fail.
				total_vertices_required += (6 * primitive->buffer.size());
			}
		}

		Array<DebugPrimitive>& per_frame_primitives = text_list->get_per_frame_primitives();
		for (size_t index = 0; index < per_frame_primitives.size(); ++index)
		{
			DebugPrimitive* primitive = &per_frame_primitives[index];
			if (primitive->type == TYPE_TEXT)
			{
				// 6 vertices per character
				// this is not even utf-8; so this will probably fail.
				total_vertices_required += (6 * primitive->buffer.size());
			}
		}

		text_vertex_cache->resize(total_vertices_required);

		if (total_vertices_required > 0)
		{
			size_t offset_index = 0;
			for (size_t index = 0; index < persistent_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &persistent_primitives[index];
				if (primitive->type == TYPE_TEXT)
				{
					size_t prev_offset = offset_index;
					offset_index += font::draw_string(text_handle, &(*text_vertex_cache)[offset_index], primitive->buffer.c_str(), primitive->buffer.size(), primitive->color);

					for (size_t vertex_index = prev_offset; vertex_index < offset_index; ++vertex_index)
					{
						font::FontVertex* vertex = &(*text_vertex_cache)[vertex_index];
						vertex->position.x += primitive->start.x;
						vertex->position.y += primitive->start.y;
					}
				}
			}

			for (size_t index = 0; index < per_frame_primitives.size(); ++index)
			{
				DebugPrimitive* primitive = &per_frame_primitives[index];
				if (primitive->type == TYPE_TEXT)
				{
					size_t prev_offset = offset_index;
					offset_index += font::draw_string(text_handle, &(*text_vertex_cache)[offset_index], primitive->buffer.c_str(), primitive->buffer.size(), primitive->color);

					for (size_t vertex_index = prev_offset; vertex_index < offset_index; ++vertex_index)
					{
						font::FontVertex* vertex = &(*text_vertex_cache)[vertex_index];
						vertex->position.x += primitive->start.x;
						vertex->position.y += primitive->start.y;
					}
				}
			}
		}

		assert(total_vertices_required % 3 == 0);
		const size_t new_vertexbuffer_size = sizeof(font::FontVertex) * total_vertices_required;
		if (new_vertexbuffer_size > 0)
		{
			device->buffer_resize(text_buffer, new_vertexbuffer_size);
			device->buffer_upload(text_buffer, &(*text_vertex_cache)[0], new_vertexbuffer_size);

			render2::CommandQueue* queue = device->create_queue(pass);
			render2::CommandSerializer* serializer = device->create_serializer(queue);

			serializer->pipeline(text_pipeline);
			serializer->vertex_buffer(text_buffer);
			render2::Texture* texture = font::get_font_texture(text_handle);
			assert(texture);
			serializer->texture(texture, 0);
			serializer->draw(0, total_vertices_required);
			device->queue_buffers(queue, 1);
			device->destroy_serializer(serializer);
		}

		text_vertex_cache->clear(false);
		text_list->reset();

		return total_vertices_required / 6;
	} // draw_text

	void render(const glm::mat4& modelview, const glm::mat4& projection, int viewport_width, int viewport_height, render2::RenderTarget* render_target)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		render2::Pass pass;
		pass.depth_test = false;
		pass.cull_mode = render2::CullMode::None;
		if (render_target == nullptr)
		{
			render_target = device->default_render_target();
		}
		pass.target = render_target;

		modelview_matrix = modelview;
		projection_matrix = projection;

		line_pipeline->constants().set("projection_matrix", &projection_matrix);
		line_pipeline->constants().set("modelview_matrix", &modelview_matrix);

		tris_pipeline->constants().set("projection_matrix", &projection_matrix);
		tris_pipeline->constants().set("modelview_matrix", &modelview_matrix);
		tris_pipeline->constants().set("diffuse", &diffuse_texture_unit);

		orthographic_projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
		text_pipeline->constants().set("projection_matrix", &orthographic_projection);
		text_pipeline->constants().set("diffuse", &diffuse_texture_unit);

		draw_lines(pass);
		draw_triangles(pass);
		draw_text(pass);
	}


	//void reset()
	//{
	//	// reset all render data.
	//	line_vertex_cache->clear(false);
	//	line_list->reset();

	//	tris_vertex_cache->clear(false);
	//	tris_list->reset();

	//	text_vertex_cache->clear(false);
	//	text_list->reset();
	//}


	void axes(const glm::mat4& transform, float axis_length, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_AXES;
			glm::vec4 col = glm::column( transform, 3 );
			primitive->start = glm::vec3( col );
			primitive->transform = transform;
			primitive->timeleft = duration;
			primitive->radius = axis_length;
		}
	}

	void basis(const glm::vec3& origin, const glm::vec3& basis, float axis_length, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if (primitive)
		{
			primitive->type = TYPE_AXES;
			glm::mat4 transform;
			transform[0] = glm::vec4(basis.x, 0, 0, 0);
			transform[1] = glm::vec4(0, basis.y, 0, 0);
			transform[2] = glm::vec4(0, 0, basis.z, 0);
			transform[3] = glm::vec4(origin.x, origin.y, origin.z, 0);
			primitive->start = origin;
			primitive->transform = transform;
			primitive->timeleft = duration;
			primitive->radius = axis_length;
		}
	}

	void box(const glm::vec3& mins, const glm::vec3& maxs, const gemini::Color& color, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_BOX;
			primitive->start = mins;
			primitive->end = maxs;
			primitive->color = color;
			primitive->timeleft = duration;
		}
	}

	void point(const glm::vec3& pt, const gemini::Color& color, float size, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_BOX;
			primitive->start = glm::vec3( pt[0] - size, pt[1] - size, pt[2] - size );
			primitive->end = glm::vec3( pt[0] + size, pt[1] + size, pt[2] + size );
			primitive->color = color;
			primitive->timeleft = duration;
		}
	}

	void line(const glm::vec3& start, const glm::vec3& end, const gemini::Color& color, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_LINE;
			primitive->start = start;
			primitive->end = end;
			primitive->color = color;
			primitive->timeleft = duration;
		}
	}

	void sphere(const glm::vec3& center, const gemini::Color& color, float radius, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_SPHERE;
			primitive->start = center;
			primitive->radius = radius;
			primitive->color = color;
			primitive->timeleft = duration;
		}
	}

	void text(int x, int y, const char* string, const gemini::Color& color, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = text_list->request(duration > 0.0f);
		if ( primitive )
		{
			primitive->type = TYPE_TEXT;
			primitive->start = glm::vec3(x, y, 0);
			primitive->color = color;
			primitive->timeleft = duration;
			primitive->buffer = string;
		}
	}

	void triangle(const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2, const gemini::Color& color, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = tris_list->request(duration > 0.0f);
		if (primitive)
		{
			primitive->type = TYPE_TRIANGLE;
			primitive->start = v0;
			primitive->end = v1;
			primitive->color = color;
			primitive->timeleft = duration;
			primitive->alt = v2;
		}
	}

	void camera(const glm::vec3& origin, const glm::vec3& view, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if (primitive)
		{
			primitive->type = TYPE_CAMERA;
			primitive->start = origin;
			primitive->end = view;
			primitive->timeleft = duration;
			primitive->radius = 1.0f;
		}
	}

	void oriented_box(const glm::mat3& orientation, const glm::vec3& origin, const glm::vec3& extents, const gemini::Color& color, float duration)
	{
#if defined(DISABLE_DEBUG_DRAW)
		return;
#endif

		DebugPrimitive* primitive = line_list->request(duration > 0.0f);
		if (primitive)
		{
			primitive->type = TYPE_OBB;
			primitive->start = origin;
			primitive->end = extents;
			primitive->transform = glm::mat4(orientation);
			primitive->timeleft = duration;
			primitive->color = color;
		}
	} // oriented box
} // namespace debugdraw
