precision highp float;

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec4 in_position;
in vec4 in_color;

out vec4 vertex_color;

void main()
{
	gl_Position = (projection_matrix * modelview_matrix * in_position);
	vertex_color = in_color;
}
