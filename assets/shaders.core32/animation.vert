#version 150 core

precision highp float;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 object_matrix;
// uniform mat4 node_transforms[2];

in vec4 in_position;
in vec3 in_normal;
in vec2 in_uv;
in ivec4 in_blendindices;
in vec4 in_blendweights;

out vec2 ex_uv;

out vec4 ex_color;

void main()
{
	ex_uv = in_uv;
	ex_color = vec4(1.0, 1.0, 1.0, 1.0);

	if (in_blendweights.x > 0)
	{
		ex_color = in_blendweights;
	}

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * in_position);
}