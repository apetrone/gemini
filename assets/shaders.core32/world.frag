out vec4 out_color;

in vec3 ps_normal;
in vec2 ps_uv0;
in vec2 ps_uv1;

uniform sampler2D diffusemap;
uniform sampler2D lightmap;

in vec3 vertex_position_world;
in vec3 light_position_world;
in vec3 vertex_to_viewer;

void main()
{
	// light direction in view-space
	vec3 vertex_to_light_world = light_position_world - vertex_position_world;

	vec3 N = normalize(ps_normal);
	vec3 L = normalize(vertex_to_light_world);
	float ndl = max(0.1, dot(N, L));

	vec4 albedo = texture(diffusemap, ps_uv0);

	float spec_power = 500;

	float distance = length(vertex_to_light_world);
	vec3 light_attenuation = vec3(0, 0.5, 0.00001);
	float att = 1.0 / (light_attenuation.x + light_attenuation.y*distance + light_attenuation.z*distance*distance);

	vec3 R = -reflect(L, N);
	vec3 view_direction = normalize(vertex_to_viewer);
	float sp = pow(max(dot(R, view_direction), 0.0), spec_power);
	// out_color = sp *vec4(1.0);

	out_color = ndl*albedo + sp*vec4(1.0);
	// 
	// out_color = ndl * albedo;
	// out_color = vec4(vertex_to_viewer, 1.0);
	// out_color = vec4(ndl, ndl, ndl, 1.0);
	// out_color = vec4(N, 1.0);
	// out_color = vec4(L, 1.0);
	// out_color = albedo * texture(lightmap, ps_uv1);
}