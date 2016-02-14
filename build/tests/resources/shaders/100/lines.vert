precision lowp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

attribute vec4 in_position;
attribute vec4 in_color;

varying vec4 vertex_color;

void main()
{
	gl_Position = (projection_matrix * modelview_matrix * in_position);
	vertex_color = in_color;
}
