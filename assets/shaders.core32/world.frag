out vec4 out_color;

in vec3 ps_normal;
in vec2 ps_uv0;
in vec2 ps_uv1;

uniform sampler2D diffusemap;
uniform sampler2D lightmap;

in vec3 vertex_position_world;
in vec3 light_position_world;

void main()
{
	// light direction in view-space
	vec3 vertex_to_light_world = light_position_world - vertex_position_world;

	vec3 N = normalize(ps_normal);
	vec3 L = normalize(vertex_to_light_world);
	float ndl = max(0.1, dot(N, L));

	vec4 albedo = texture(diffusemap, ps_uv0);
	out_color = ndl * albedo;
	// out_color = vec4(N, 1.0);
	// out_color = albedo * texture(lightmap, ps_uv1);
}