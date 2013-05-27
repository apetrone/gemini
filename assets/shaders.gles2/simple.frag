#version 100

precision lowp float;

varying vec4 ex_color;

void main()
{
	gl_FragColor = ex_color;
}