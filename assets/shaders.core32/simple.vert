#version 150 core

precision highp float;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

in vec2 in_position;
in vec4 in_color;
out vec4 ex_color;

void main()
{
	ex_color = in_color;
	gl_Position = (projection_matrix * modelview_matrix * vec4(in_position, 0.0, 1.0));
}