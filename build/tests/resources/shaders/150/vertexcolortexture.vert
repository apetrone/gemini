precision highp float;
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;

in vec3 in_position;
in vec4 in_color;
in vec2 in_uv;

out vec4 vertex_color;
out vec2 frag_uv;

void main()
{
	gl_Position = (projection_matrix * modelview_matrix * vec4(in_position, 1.0));
	vertex_color = in_color;
	frag_uv = in_uv;
}
