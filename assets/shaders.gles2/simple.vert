#version 100

precision lowp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

attribute vec4 in_position;

void main()
{
	gl_Position = (projection_matrix * modelview_matrix * in_position);
}