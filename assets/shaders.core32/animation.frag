#version 150 core

precision highp float;

in vec2 ex_uv;
out vec4 out_color;

in vec4 ex_color;

uniform sampler2D diffusemap;

void main()
{
	//out_color = vec4(1.0, 0.0, 1.0, 1.0);
	out_color = ex_color * texture(diffusemap, ex_uv);
}