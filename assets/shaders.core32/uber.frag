#version 150

out vec4 out_Color;


#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) && !defined(D_DIFFUSE_MAP)
	in vec4 ps_Color;
#endif

#ifdef D_VERTEX_NORMALS
	in vec3 ps_Normal;
#endif

#if defined(D_LIGHT_POSITION)
	in vec3 lightDirection;
	in vec3 ps_View;
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	in vec2 ps_uv0;
#endif

#ifdef D_DIFFUSE_MAP
	uniform sampler2D diffusemap;
#endif

#ifdef D_SPECULAR_MAP
	uniform sampler2D specularmap;
#endif

#ifdef D_DIFFUSE_COLOR
	uniform vec4 diffusecolor;
#endif

#if defined(D_CUBEMAP)
	in vec3 ps_CubeVertex;
	uniform samplerCube cubemap;
#endif


void main()
{
	vec4 color = vec4( 0.0, 1.0, 1.0, 1.0 );
	vec4 specularColor = vec4(1.0);
	vec4 specularContribution = vec4(0.0, 0.0, 0.0, 0.0);

#if defined(D_LIGHT_POSITION)
	vec3 L = normalize(lightDirection);
	float specularPower = 100.0;
	float specularLevel = 0.45;
#else
	vec3 L = vec3(0,1,0);
#endif

#if defined(D_VERTEX_NORMALS)
	vec3 N = normalize(ps_Normal);
	float ndotl = max(0.0, dot(N,L));

	float roughness = 0.25;
	float r2 = roughness * roughness;
	float A = 1.0 - 0.5 * ( r2 / (r2 + 0.33));
	float B = 0.45 * (r2 / (r2 + 0.09) );

	vec3 reflection = -normalize(reflect( L, N ));
	float angle_i = dot( L, N );
	float angle_r = dot( reflection, N );

	float alpha = max( angle_i, angle_r );
	float beta = min( angle_i, angle_r );

	ndotl = ndotl * (A + (B * max(0.0, cos(angle_i - angle_r)) * sin(alpha) * tan(beta) ));
#endif




#if defined(D_DIFFUSE_MAP)
	color = texture( diffusemap, ps_uv0 );
#elif defined(D_CUBEMAP)
	color = texture( cubemap, ps_CubeVertex );
#endif

#if defined(D_DIFFUSE_COLOR)
	color = diffusecolor;
#endif


#if defined(D_SPECULAR_MAP)
	specularColor = texture( specularmap, ps_uv0 );
	specularLevel = specularColor.a;
	specularColor.a = 1.0;
#endif


#if defined(D_LIGHT_POSITION)
	// calculate the reflection vector
	vec3 r = -normalize(reflect( L, N ));
	specularContribution = specularColor * specularLevel * pow( clamp(dot( normalize(ps_View), r ), 0.0, 1.0), specularPower );
#endif


#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) && !defined(D_DIFFUSE_MAP)
	color = ps_Color;
#endif

#if defined(D_VERTEX_NORMALS) && defined(D_LIGHT_POSITION) && !defined(D_CUBEMAP)
	color *= ndotl;
#endif


	out_Color = color + specularContribution;
}