#version 150

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 object_matrix;

in vec4 in_position;
in vec3 in_normal;
in vec2 in_uv0;
in vec2 in_uv1;

out vec3 ps_normal;
out vec2 ps_uv0;
out vec2 ps_uv1;

void main()
{
	ps_normal = normalize(inverse(transpose(mat3(object_matrix))) * in_normal);
	ps_uv0 = in_uv0;
	ps_uv1 = in_uv1;

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * in_position);	
}