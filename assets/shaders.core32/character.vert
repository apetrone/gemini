#version 150

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 object_matrix;
uniform mat4 node_transforms[60];

in vec4 in_position;
in vec3 in_normal;
in vec2 in_uv0;
in vec4 in_blendindices;
in vec4 in_blendweights;

out vec3 ps_normal;
out vec2 ps_uv0;

void main()
{
	ps_normal = normalize(inverse(transpose(mat3(object_matrix))) * in_normal);
	ps_uv0 = in_uv0;

	vec4 final_vertex = vec4(0.0);
	final_vertex += (node_transforms[int(in_blendindices.x)] * in_position) * in_blendweights.x;
	final_vertex += (node_transforms[int(in_blendindices.y)] * in_position) * in_blendweights.y;
	final_vertex += (node_transforms[int(in_blendindices.z)] * in_position) * in_blendweights.z;
	final_vertex += (node_transforms[int(in_blendindices.w)] * in_position) * in_blendweights.w;

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * final_vertex);	
}