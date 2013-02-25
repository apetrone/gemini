#version 150 core
precision highp float;

in vec4 ex_color;
out vec4 out_color;

uniform sampler2D diffusemap;
in vec2 out_uv;

void main()
{
	vec4 texel = texture( diffusemap, out_uv );
	out_color = (texel * ex_color);
}