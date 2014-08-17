out vec4 out_color;

in vec3 ps_normal;
in vec2 ps_uv0;
in vec2 ps_uv1;

uniform sampler2D diffusemap;
uniform sampler2D lightmap;

in vec3 vertex_position_world;
in vec3 light_position_world;
in vec3 vertex_to_viewer_worldspace;
in vec3 view_direction_worldspace;

#define M_PI 3.14159265359

float shlick_fresnel(in float n2, in vec3 H, in vec3 L)
{
	float f0 = (1.0-n2) / (1.0+n2);
	float r0 = f0 * f0;
	float cos_theta = dot(H, L);

	return max(0.0, r0 + (1.0 - r0) * pow((1.0 - cos_theta), 5.0));
}

float lambertian_diffuse(in vec3 L, in vec3 N)
{
	return max(0.0, dot(N, L));
}

float oren_nayer_diffuse(in float roughness, in vec3 L, in vec3 N)
{
	float ndotl = max(0.0, dot(N,L));

	float r2 = roughness * roughness;
	float A = 1.0 - 0.5 * ( r2 / (r2 + 0.33));
	float B = 0.45 * (r2 / (r2 + 0.09) );

	vec3 R = normalize(reflect(L, N));
	float angle_i = dot( L, N );
	float angle_r = dot( R, N );

	float alpha = max(angle_i, angle_r);
	float beta = min(angle_i, angle_r);

	return ndotl * (A + (B * max(0.0, cos(angle_i - angle_r)) * sin(alpha) * tan(beta) ));
}

void blinn_phong_lighting()
{

}

void per_pixel_lighting(in vec3 vertex_to_light_world)
{
	vec3 N = normalize(ps_normal);
	vec3 L = normalize(vertex_to_light_world);
	// float ndl = lambertian_diffuse(L, N);
	float ndl = oren_nayer_diffuse(0.0, L, N);

	vec4 albedo = texture(diffusemap, ps_uv0);

	// calculate attenuation
	float distance = length(vertex_to_light_world);
	vec3 light_attenuation = vec3(0, 0.1, 0.00001);
	float att = 1.0 / (light_attenuation.x + light_attenuation.y*distance + light_attenuation.z*distance*distance);

	// calculate specular
	float spec_power = 500;
	vec3 R = -reflect(L, N);
	vec3 VtoV = normalize(vertex_to_viewer_worldspace);
	float sp = pow(max(dot(R, VtoV), 0.0), spec_power);


	vec3 V = normalize(view_direction_worldspace);

	// try to compute the fresnel term
	vec3 LV = L+V;
	vec3 H = (LV/length(LV));
	float fresnel = shlick_fresnel(0.1, H, L);
	// vec3 specular = fresnel_shlick(vec3(1.0, 1.0, 1.0), eye_position_worldspace, H);

	// lambert diffuse, with attenuation and specular
	// out_color = att*ndl*albedo + sp*vec4(1.0);
	

	// out_color = vec4(vertex_to_viewer_worldspace, 1.0);
	// out_color = vec4(ndl, ndl, ndl, 1.0);
	// out_color = vec4(sp, sp, sp, 1.0);
	// out_color = vec4(ndl*N, 1.0);
	// out_color = vec4(N, 1.0);
	// out_color = vec4(H, 1.0);	
	// out_color = vec4(L, 1.0);
	// 
	
	// out_color = vec4(fresnel, fresnel, fresnel, 1.0);
	out_color = vec4(LV, 1.0);
	// out_color = albedo * texture(lightmap, ps_uv1);
}





void pbs_main()
{

}

void main()
{
	// light direction in view-space
	vec3 vertex_to_light_world = light_position_world - vertex_position_world;

	per_pixel_lighting(vertex_to_light_world);

	// pbs_main();
}