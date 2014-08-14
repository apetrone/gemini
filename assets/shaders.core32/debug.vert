#version 150 core

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 object_matrix;

in vec4 in_position;
in vec4 in_color;

out vec4 ps_color;

void main()
{
	ps_color = in_color;
	
	gl_Position = (projection_matrix * modelview_matrix * object_matrix * in_position);
}