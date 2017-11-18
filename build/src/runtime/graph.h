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

#include <core/typedefs.h>
#include <core/stackstring.h>

// Graph Operations
//- adjacent(G, x, y): tests whether there is an edge from vertex x to the vertex y
//- neighbors(G, x): lists all vertices y with an edge from vertex x to the vertex y
//- add_vertex(G, x): adds a vertex x
//- remove_vertex(G, x): removes a vertex x from the graph if exists
//- add_edge(G, x, y): adds the edge from vertex x to the vertex y
//- remove_edge(G, x, y): removes the edge from vertex x to the vertex y
//- get_vertex_value(G, x): returns the value associated with the vertex x
//- set_vertex_value(G, x, v): sets the value associated with the vertex x to v.

// References
// http://web.cecs.pdx.edu/~sheard/course/Cs163/Doc/Graphs.html

namespace gemini
{
	struct GraphNode
	{
		uint32_t index;

		// subset of data
		core::StackString<32> name;
	}; // GraphNode

	struct GraphEdge
	{
		uint32_t first;
		uint32_t second;

		GraphEdge(uint32_t _first = 0, uint32_t _second = 0)
			: first(_first)
			, second(_second)
		{
		}
	}; // GraphEdge

	struct GraphContainer
	{
		gemini::Allocator* allocator;
		Array<GraphNode*> nodes;
		Array<GraphEdge> edges;

		GraphContainer(gemini::Allocator& _allocator)
			: allocator(&_allocator)
			, nodes(_allocator)
			, edges(_allocator)
		{
		}
	}; // GraphContainer

	GraphNode* graph_create_node(GraphContainer* graph);
	void graph_purge(GraphContainer* graph);
	void graph_add_edge(GraphContainer* graph, const GraphEdge& edge);
	void graph_next_node(GraphContainer* graph, GraphNode* start);
	void graph_dijkstra_search(GraphContainer* graph, GraphNode* source);

	// returns true if an edge (u, v) exists.
	bool graph_node_is_adjacent(GraphContainer* graph, uint32_t u, uint32_t v);
} // namespace gemini