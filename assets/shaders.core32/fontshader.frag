#version 150 core

precision highp float;

out vec4 out_color;

in vec4 ps_color;
in vec2 ps_uv0;

uniform sampler2D diffusemap;

void main()
{
	vec4 texel = texture(diffusemap, ps_uv0);
	out_color = vec4(ps_color.r, ps_color.g, ps_color.b, texel.r);
}