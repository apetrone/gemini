// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
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
#include "navigation.h"

#include <runtime/logging.h>

#include <RecastAlloc.h> // so we can override the default allocator
#include <Recast.h>
#include <RecastDebugDraw.h>
#include <DebugDraw.h>
#include <RecastDump.h>

#include <DetourAlloc.h> // so we can override the default allocator
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourNavMeshBuilder.h>
#include <DetourDebugDraw.h>

#include <renderer/debug_draw.h>
#include <core/color.h>

#include <core/mathlib.h>
#include <core/util.h>

using core::Color;

namespace debugdraw = ::renderer::debugdraw;

namespace gemini
{
	namespace navigation
	{
		typedef core::memory::HeapAllocator<core::memory::DefaultTrackingPolicy> NavigationAllocatorType;
		NavigationAllocatorType* _nav_allocator;

		void* navigation_allocate(int size, rcAllocHint hint)
		{
			return MEMORY_ALLOC(size, *_nav_allocator);
		}

		void* navigation_allocate(int size, dtAllocHint hint)
		{
			return MEMORY_ALLOC(size, *_nav_allocator);
		}

		void navigation_deallocate(void* ptr)
		{
			MEMORY_DEALLOC(ptr, *_nav_allocator);
		}

		static rcPolyMesh* poly_mesh = 0;
		static rcPolyMeshDetail* detail_mesh = 0;
		static rcContourSet* contour_set = 0;
		static dtNavMesh* nav_mesh = 0;
		static dtNavMeshQuery* nav_query = 0;



		// debug draw implementation
		// TODO: This still needs to implement debug drawing of triangles
		// with a checkerboard texture.
		// The DebugDraw interface doesn't support this, yet.
		class NavigationDebugDraw : public duDebugDraw
		{
			int primitive_state;
			int vertex_offset;


			glm::vec3 vertices[4];
			Color colors[4];

		public:
			NavigationDebugDraw() :
				primitive_state(-1),
				vertex_offset(0)
			{
			}

			virtual void depthMask(bool state)
			{
//				LOGV("depth_mask: %i\n", state);
			}

			virtual void texture(bool state)
			{
//				LOGV("texture: %i\n", state);
			}

			virtual void begin(duDebugDrawPrimitives prim, float size)
			{
				primitive_state = prim;
				vertex_offset = 0;
			}

			virtual void vertex(const float* pos, unsigned int color) override
			{
				glm::vec3 position = glm::vec3(pos[0], pos[1], pos[2]);
				Color c = Color::from_int(color);

				switch(primitive_state)
				{
					case DU_DRAW_POINTS:
						debugdraw::point(position, c, 0.1f);
						break;

					case DU_DRAW_TRIS:
						vertices[vertex_offset] = position;
						colors[vertex_offset] = c;
						vertex_offset++;

						if (vertex_offset == 3)
						{
							debugdraw::triangle(vertices[0], vertices[1], vertices[2], c);
							vertex_offset = 0;
						}
						break;
				}

			}

			virtual void vertex(const float x, const float y, const float z, unsigned int color) override
			{
				glm::vec3 position = glm::vec3(x, y, z);
				Color c = Color::from_int(color);

				switch(primitive_state)
				{
					case DU_DRAW_LINES:
						vertices[vertex_offset] = position;
						colors[vertex_offset] = c;
						vertex_offset++;

						// every second vertex we hit, draw a line
						if (vertex_offset == 2)
						{
							debugdraw::line(vertices[0], vertices[1], c);
							vertex_offset = 0;
						}
						break;

					case DU_DRAW_TRIS:
						vertices[vertex_offset] = position;
						colors[vertex_offset] = c;
						vertex_offset++;

						if (vertex_offset == 3)
						{
							debugdraw::triangle(vertices[0], vertices[1], vertices[2], c);
							vertex_offset = 0;
						}
						break;
				}
			}

			virtual void vertex(const float* pos, unsigned int color, const float* uv) override
			{
				assert(0); // not implemented
			}

			virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override
			{
				assert(0); // not implemented
			}

			virtual void end()
			{
				assert(vertex_offset == 0);

				primitive_state = -1;
				vertex_offset = 0;
			}
		};


		static NavigationDebugDraw debug_draw;

		void create_from_geometry(const FixedArray<glm::vec3>& vertices, const FixedArray<::renderer::IndexType>& indices, const glm::vec3& mins, const glm::vec3& maxs)
		{
			float agent_height = 2.0f;
			float agent_radius = 0.6f;
			float agent_max_climb = 0.9f;
			float max_edge_length = 12.0f;

			float bounds_min[3] = {mins[0], mins[1], mins[2]};
			float bounds_max[3] = {maxs[0], maxs[1], maxs[2]};

			const int total_vertices = vertices.size();
			const int total_triangles = indices.size() / 3;

			// first, setup our allocators
			rcAllocSetCustom(navigation_allocate, navigation_deallocate);
			dtAllocSetCustom(navigation_allocate, navigation_deallocate);


			// 1. setup build config

			rcConfig config;
			memset(&config, 0, sizeof(rcConfig));

			config.cs = 0.3f; // cell size
			config.ch = 0.2f; // cell height
			config.walkableSlopeAngle = 45.0f;
			config.walkableHeight = static_cast<int>(ceilf(agent_height / config.ch));
			config.walkableClimb = static_cast<int>(floorf(agent_max_climb / config.ch));
			config.walkableRadius = static_cast<int>(ceilf(agent_radius / config.cs));
			config.maxEdgeLen = static_cast<int>(max_edge_length / config.cs);
			config.maxSimplificationError = 1.3f;
			config.minRegionArea = static_cast<int>(rcSqr(8));
			config.mergeRegionArea = static_cast<int>(rcSqr(20));
			config.maxVertsPerPoly = 6;
			config.detailSampleDist = 6.0 * config.cs;
			config.detailSampleMaxError = config.ch * 1.0f;

			// set the area where the navigation will be built
			rcVcopy(config.bmin, bounds_min);
			rcVcopy(config.bmax, bounds_max);
			rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

			// rasterize input polygon soup

			rcContext context;
			context.resetTimers();
			context.startTimer(RC_TIMER_TOTAL);

			LOGV("begin generating nav mesh data\n");

			rcHeightfield* solid = rcAllocHeightfield();
			if (!rcCreateHeightfield(&context, *solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch))
			{
				LOGV("nav: unable to create solid heightfield!\n");
			}

			// allocate an array which can hold the max number of triangles you need to process
			// across all meshes

			unsigned char* triangle_areas =	MEMORY_NEW_ARRAY(unsigned char, total_triangles, *_nav_allocator);
			if (!triangle_areas)
			{
				LOGE("unable to allocate triangle areas\n");
			}


			memset(triangle_areas, 0, total_triangles*sizeof(unsigned char));
			rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, (const float*)&vertices[0], total_vertices, (const int*)&indices[0], total_triangles, triangle_areas);
			rcRasterizeTriangles(&context, (const float*)&vertices[0], total_vertices, (const int*)&indices[0], triangle_areas, total_triangles, *solid, config.walkableClimb);

			// at this point, we could delete triangle_areas
			MEMORY_DELETE_ARRAY(triangle_areas, *_nav_allocator);
			triangle_areas = 0;

			// 3. filter walkable surfaces
			rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *solid);
			rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *solid);
			rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *solid);

			// 4. partition walkable surface to simple regions

			// compact the heightfield
			// improves cache coherency data and the neighbors between walkable cells
			// will be calculated.
			rcCompactHeightfield* chf = rcAllocCompactHeightfield();
			if (!chf)
			{
				LOGE("Unable to create compact height field\n");
			}

			if (!rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *solid, *chf))
			{
				LOGE("Unable to build compact data!\n");
			}

			// at this point, we can remove the original heightfield.
			rcFreeHeightField(solid);
			solid = 0;


			if (!rcErodeWalkableArea(&context, config.walkableRadius, *chf))
			{
				LOGE("Unable to erode walkable data\n");
			}

			// TODO: optionally mark areas



			// partition the heightfield
			if (!rcBuildDistanceField(&context, *chf))
			{
				LOGE("build navigation could not build distance field\n");
			}

			if (!rcBuildRegions(&context, *chf, 0, config.minRegionArea, config.mergeRegionArea))
			{
				LOGE("Unable to build watershed regions\n");
			}


			// trace and simplify region contours
			contour_set = rcAllocContourSet();
			if (!contour_set)
			{
				LOGE("unable to create contour set\n");
			}
			if (!rcBuildContours(&context, *chf, config.maxSimplificationError, config.maxEdgeLen, *contour_set))
			{
				LOGE("failed to build contours\n");
			}


			// build polygon mesh from contours
			poly_mesh = rcAllocPolyMesh();
			if (!poly_mesh)
			{
				LOGE("Unable to create poly_mesh\n");
			}

			if (!rcBuildPolyMesh(&context, *contour_set, config.maxVertsPerPoly, *poly_mesh))
			{
				LOGE("Unable to build poly mesh\n");
			}

			//			for (int index = 0; index < poly_mesh->maxpolys; ++index)
			//			{
			//				LOGV("flags: %i\n", poly_mesh->flags[index]);
			//			}
			//			assert(poly_mesh->nverts > 0);


			// create detail mesh which allows access to approximate height on each polygon
			detail_mesh = rcAllocPolyMeshDetail();
			if (!detail_mesh)
			{
				LOGE("Unable to create detail mesh\n");
			}

			if (!rcBuildPolyMeshDetail(&context, *poly_mesh, *chf, config.detailSampleDist, config.detailSampleMaxError, *detail_mesh))
			{
				LOGE("Unable to create detail mesh\n");
			}



			// free compact height field and contour set
			rcFreeCompactHeightfield(chf);


			// at this point the nav mesh is ready.
			// access it via poly_mesh.

			for (int i = 0; i < poly_mesh->npolys; ++i)
			{
				if (poly_mesh->areas[i] == RC_WALKABLE_AREA)
				{
					poly_mesh->areas[i] = 1;
					poly_mesh->flags[i] = 1;
				}
				else
				{
					poly_mesh->flags[i] = 0;
				}
			}



			// TODO: optionally, create detour data from the recast poly mesh
			dtNavMeshCreateParams params;
			memset(&params, 0, sizeof(dtNavMeshCreateParams));
			params.verts = poly_mesh->verts;
			params.vertCount = poly_mesh->nverts;
			params.polys = poly_mesh->polys;
			params.polyAreas = poly_mesh->areas;
			params.polyFlags = poly_mesh->flags;
			params.polyCount = poly_mesh->npolys;
			params.nvp = poly_mesh->nvp;
			params.detailMeshes = detail_mesh->meshes;
			params.detailVerts = detail_mesh->verts;
			params.detailVertsCount = detail_mesh->nverts;
			params.detailTris = detail_mesh->tris;
			params.detailTriCount = detail_mesh->ntris;
			params.walkableHeight = agent_height;
			params.walkableRadius = agent_radius;
			params.walkableClimb = agent_max_climb;
			rcVcopy(params.bmin, poly_mesh->bmin);
			rcVcopy(params.bmax, poly_mesh->bmax);
			params.cs = config.cs;
			params.ch = params.ch;
			params.buildBvTree = true;


			unsigned char* nav_data = 0;
			int nav_data_size = 0;
			if (!dtCreateNavMeshData(&params, &nav_data, &nav_data_size))
			{
				LOGV("unable to create detour navigation mesh!\n");
			}

			nav_mesh = dtAllocNavMesh();
			if (!nav_mesh)
			{
				dtFree(nav_data);
				LOGV("Unable to create detour nav mesh\n");
			}

			dtStatus status;
			status = nav_mesh->init(nav_data, nav_data_size, DT_TILE_FREE_DATA);
			if (dtStatusFailed(status))
			{
				dtFree(nav_data);
				LOGV("Could not init detour nav mesh\n");
			}

			nav_query = dtAllocNavMeshQuery();
			status = nav_query->init(nav_mesh, 2048);
			if (dtStatusFailed(status))
			{
				LOGV("Could not init navmesh query\n");
			}

			context.stopTimer(RC_TIMER_TOTAL);

			float timer_milliseconds = context.getAccumulatedTime(RC_TIMER_TOTAL) * MillisecondsPerMicrosecond;
			LOGV("finished generating navmesh: %2.2fms\n", timer_milliseconds);
		}

		void startup()
		{
			core::memory::Zone* nav_zone = MEMORY_NEW(core::memory::Zone, core::memory::global_allocator())("navigation");
			_nav_allocator = MEMORY_NEW(NavigationAllocatorType, core::memory::global_allocator())(nav_zone);
		}

		void shutdown()
		{
			// free Recast data
			rcFreePolyMeshDetail(detail_mesh);
			rcFreePolyMesh(poly_mesh);
			rcFreeContourSet(contour_set);

			// free Detour data
			dtFreeNavMesh(nav_mesh);
			dtFreeNavMeshQuery(nav_query);

			core::memory::Zone* nav_zone = _nav_allocator->get_zone();
			MEMORY_DELETE(_nav_allocator, core::memory::global_allocator());
			_nav_allocator = nullptr;

			MEMORY_DELETE(nav_zone, core::memory::global_allocator());
		}

		void debugdraw()
		{
			if (!nav_mesh || !poly_mesh)
			{
				return;
			}

			unsigned char flags = DU_DRAWNAVMESH_OFFMESHCONS;
//			unsigned char flags = DU_DRAWNAVMESH_COLOR_TILES;
			duDebugDrawNavMesh(&debug_draw, *nav_mesh, flags);
//			duDebugDrawPolyMesh(&debug_draw, *poly_mesh);

//			duDebugDrawCompactHeightfieldRegions(&debug_draw, *compact_heightfield);
//			duDebugDrawRawContours(&debug_draw, *contour_set, 0.25f);
//			duDebugDrawContours(&debug_draw, *contour_set);
		}

		bool find_poly(NavMeshPolyRef* ref, const glm::vec3& position, const glm::vec3& extents)
		{
			const float* cen = glm::value_ptr(position);
			const float* ext = glm::value_ptr(extents);

			float* nearest_point = 0;
			dtQueryFilter filter;
			nav_query->findNearestPoly(cen, ext, &filter, ref, nearest_point);

			return (nearest_point != 0);
		}

		void find_path(NavMeshPath* path, const glm::vec3& start, const glm::vec3& end)
		{
			const float* start_position = glm::value_ptr(start);
			const float* end_position = glm::value_ptr(end);

			path->start_position = start;
			path->end_position = end;

			NavMeshPolyRef start_poly = 0;
			NavMeshPolyRef end_poly = 0;

			find_poly(&start_poly, start, path->extents);
			find_poly(&end_poly, end, path->extents);

			dtQueryFilter filter;
			dtStatus status = nav_query->findPath(start_poly, end_poly, start_position, end_position, &filter, path->links, &path->link_count, MAX_NAVMESH_PATH_LINKS);
			if (dtStatusFailed(status))
			{
				LOGW("nav_query failed to find path\n");
			}
		}

		void debugdraw_path(NavMeshPath* path)
		{
			for (int poly = 0; poly < path->link_count; ++poly)
			{
				duDebugDrawNavMeshPoly(&debug_draw, *nav_mesh, path->links[poly], duRGBA(255, 255, 0, 128));
			}
		}

		void find_straight_path(NavMeshPath* path, glm::vec3* positions, uint32_t* total_positions)
		{
			float* waypoints = reinterpret_cast<float*>(&positions[0]);
			unsigned char straight_path_flags[MAX_NAVMESH_POINTS];
			dtPolyRef straight_path_refs[MAX_NAVMESH_POINTS];
			int straight_path_count = 0;

			/*dtStatus status = */nav_query->findStraightPath(
												glm::value_ptr(path->start_position),
												glm::value_ptr(path->end_position),
												path->links,
												 path->link_count,
												 waypoints,
												 straight_path_flags,
												 straight_path_refs,
												 &straight_path_count,
												 MAX_NAVMESH_POINTS,
												 0);

			assert(straight_path_count >= 0);
			*total_positions = straight_path_count;
		}

		float random_float()
		{
			return core::util::random_range(0.0f, 1.0f);
		}

		glm::vec3 find_random_location()
		{
			glm::vec3 location;
			dtQueryFilter filter;
			dtPolyRef poly_ref;
			float point[3];

			nav_query->findRandomPoint(&filter, random_float, &poly_ref, point);
			location.x = point[0];
			location.y = point[1];
			location.z = point[2];
			fprintf(stdout, "random point: %2.2f, %2.2f, %2.2f\n", location.x, location.y, location.z);

			return location;
		}
	} // namespace navigation
} // namespace gemini
