#version 100

precision lowp float;

varying vec4 ex_color;

uniform sampler2D diffusemap;
varying vec2 out_uv;

void main()
{
	vec4 texel = texture2D( diffusemap, out_uv );
	gl_FragColor = vec4(ex_color.r, ex_color.g, ex_color.b, texel.a);
}