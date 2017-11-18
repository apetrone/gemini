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

#include <runtime/graph.h>
#include <core/logging.h>

namespace gemini
{
	GraphNode* graph_create_node(GraphContainer* graph)
	{
		GraphNode* node = MEMORY2_NEW(*graph->allocator, GraphNode);
		node->index = graph->nodes.size();
		graph->nodes.push_back(node);
		return node;
	} // graph_create_node

	void graph_purge(GraphContainer* graph)
	{
		for (size_t index = 0; index < graph->nodes.size(); ++index)
		{
			MEMORY2_DELETE(*graph->allocator, graph->nodes[index]);
		}
		graph->nodes.clear();

		graph->edges.clear();
	} // graph_purge

	void graph_add_edge(GraphContainer* graph, const GraphEdge& edge)
	{
		graph->edges.push_back(edge);
	} // graph_add_edge

	void graph_next_node(GraphContainer* graph, GraphNode* start)
	{
		for (size_t index = 0; index < graph->edges.size(); ++index)
		{
			GraphEdge& edge = graph->edges[index];
			if (edge.first == start->index)
			{
				GraphNode* next = graph->nodes[edge.second];
				if (next)
				{
					LOGV("next node is: \"%s\" (%i)\n", next->name(), next->index);
					return;
				}
			}
		}

		LOGV("Unable to find edges starting from: \"%s\" (%i)\n", start->name(), start->index);
	} // graph_next_node

	void graph_dijkstra_search(GraphContainer* graph, GraphNode* source)
	{
		// https://en.wikipedia.org/wiki/Dijkstra%27s_algorithm
		// source to distance array
		Array<uint32_t> distance(*graph->allocator);
		Array<uint32_t> last_node(*graph->allocator);
		Array<GraphNode*> set(*graph->allocator);

		const size_t node_count = graph->nodes.size();
		distance.resize(node_count);
		last_node.resize(node_count);
		set.resize(node_count);

		for (size_t index = 0; index < node_count; ++index)
		{
			distance[index] = UINT32_MAX;
			last_node[index] = UINT32_MAX;
		}

		distance[source->index] = 0;


		while (!set.empty())
		{
		}
	} // graph_dijkstra_search

	bool graph_node_is_adjacent(GraphContainer* graph, uint32_t u, uint32_t v)
	{
		for (size_t index = 0; index < graph->edges.size(); ++index)
		{
			const GraphEdge& edge = graph->edges[index];
			if (edge.first == u && edge.second == v)
			{
				return true;
			}
		}

		return false;
	} // graph_node_is_adjacent
} // namespace gemini