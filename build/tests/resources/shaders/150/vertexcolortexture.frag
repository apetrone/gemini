precision highp float;
in vec4 vertex_color;
in vec2 frag_uv;

out vec4 out_color;

uniform sampler2D diffuse;

void main()
{
	out_color = texture(diffuse, frag_uv) * vertex_color;
}