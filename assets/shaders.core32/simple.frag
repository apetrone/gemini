#version 150 core

precision highp float;

in vec4 ex_color;
out vec4 out_color;

void main()
{
	out_color = ex_color;
}