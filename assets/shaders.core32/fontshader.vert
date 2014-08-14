#version 150 core

precision highp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec4 in_position;
in vec2 in_uv0;
in vec4 in_color;

out vec4 ps_color;
out vec2 ps_uv0;

void main()
{
	ps_color = in_color;
	ps_uv0 = in_uv0;
	
	gl_Position = (projection_matrix * modelview_matrix * in_position);
}