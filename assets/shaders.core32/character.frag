out vec4 out_color;

in vec3 ps_normal;
in vec2 ps_uv0;

uniform sampler2D diffusemap;

void main()
{
	out_color = texture(diffusemap, ps_uv0);
}