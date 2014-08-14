#version 150

out vec4 out_color;

in vec3 ps_normal;
in vec2 ps_uv0;
in vec2 ps_uv1;

uniform sampler2D diffusemap;
uniform sampler2D lightmap;

void main()
{
	vec4 albedo = texture(diffusemap, ps_uv0);

	out_color = albedo * texture(lightmap, ps_uv1);
}