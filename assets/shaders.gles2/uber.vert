uniform mat4 modelviewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 objectMatrix;


attribute vec3 in_Position;

#if defined(D_VERTEX_NORMALS)
	attribute vec3 in_Normal;
	varying vec3 ps_Normal;
#endif

#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) && !defined(D_DIFFUSE_MAP)
	attribute vec4 in_Color;
	varying vec4 ps_Color;
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	attribute vec2 in_UV0;
	varying vec2 ps_uv0;
#endif

#if defined(D_LIGHT_POSITION)
	uniform vec3 lightPosition;
	varying vec3 lightDirection;
	uniform vec3 cameraPosition;
	varying vec3 ps_View;
#endif

#if defined(D_CUBEMAP)
	varying vec3 ps_CubeVertex;
#endif

void main()
{
#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) && !defined(D_DIFFUSE_MAP)
	ps_Color = in_Color;
#endif

#if defined(D_LIGHT_POSITION)
	vec3 eye_light = lightPosition;
	vec3 eye_vertex = vec3(objectMatrix * vec4(in_Position, 1.0));
	lightDirection = (eye_light - eye_vertex);
	ps_View = cameraPosition - eye_vertex;
#endif

#if defined(D_VERTEX_NORMALS)
	ps_Normal = normalize( mat3(objectMatrix) * in_Normal );
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	ps_uv0 = in_UV0;
#endif

	gl_Position = (projectionMatrix * modelviewMatrix * objectMatrix * vec4(in_Position, 1.0));

#if defined(D_CUBEMAP)
	ps_CubeVertex = normalize(in_Position);
	gl_Position.z = gl_Position.w - 0.00001; // fix to far plane
#endif
}
