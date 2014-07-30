#version 150 core

precision highp float;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 object_matrix;
uniform mat4 node_transforms[2];

in vec4 in_position;
in vec3 in_normal;
in vec4 in_color;
in vec2 in_uv;
in vec4 in_blendindices;
in vec4 in_blendweights;

out vec2 ex_uv;

out vec4 ex_color;

// http://mmmovania.blogspot.com/2012/11/skeletal-animation-and-gpu-skinning.html

void main()
{
	ex_uv = in_uv;
	ex_color = vec4(1.0);
	// ex_color = in_blendweights;

	// vec4 final_vertex = in_position;
	// ex_color = in_blendweights;

	vec4 final_vertex = vec4(0.0);
	final_vertex += (node_transforms[int(in_blendindices.x)] * in_position) * in_blendweights.x;
	final_vertex += (node_transforms[int(in_blendindices.y)] * in_position) * in_blendweights.y;
	final_vertex += (node_transforms[int(in_blendindices.z)] * in_position) * in_blendweights.z;
	final_vertex += (node_transforms[int(in_blendindices.w)] * in_position) * in_blendweights.w;

	// TODO: transform normals + recompute tangents/binnormals

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * final_vertex);
}