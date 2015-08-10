precision lowp float;

varying vec4 vertex_color;
varying vec2 frag_uv;

uniform sampler2D diffuse;

void main()
{
	gl_FragColor = texture2D(diffuse, frag_uv) * vertex_color;
}