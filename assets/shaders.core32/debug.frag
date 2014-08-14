out vec4 out_color;

in vec4 ps_color;

void main()
{
	out_color = vec4(ps_color.r, ps_color.g, ps_color.b, ps_color.r);
}