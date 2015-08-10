precision lowp float;

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;

attribute vec4 in_position;
attribute vec4 in_color;
attribute vec2 in_uv;

varying vec4 vertex_color;
varying vec2 frag_uv;

void main()
{
	gl_Position = (projection_matrix * modelview_matrix * in_position);
	vertex_color = in_color;
	frag_uv = in_uv;
}