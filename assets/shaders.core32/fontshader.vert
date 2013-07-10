#version 150 core

precision highp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec4 in_position;
in vec2 in_uv;
in vec4 in_color;

out vec4 ex_color;
out vec2 out_uv;

void main()
{
	ex_color = in_color;
	out_uv = in_uv;
	
	gl_Position = (projection_matrix * modelview_matrix * in_position);
}