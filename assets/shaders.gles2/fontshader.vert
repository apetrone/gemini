#version 100

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

attribute vec3 in_position;
attribute vec4 in_color;
attribute vec2 in_uv;

varying vec4 ex_color;
varying vec2 out_uv;

void main()
{
	ex_color = in_color;
	out_uv = in_uv;
	
	gl_Position = (projection_matrix * modelview_matrix * vec4(in_position, 1.0));
}