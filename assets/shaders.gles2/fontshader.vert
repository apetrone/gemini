#version 100

precision lowp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

attribute vec4 in_position;
attribute vec2 in_uv;
attribute vec4 in_color;

varying vec4 ex_color;
varying vec2 out_uv;

void main()
{
	ex_color = in_color;
	out_uv = in_uv;
	
	gl_Position = (projection_matrix * modelview_matrix * in_position);
}