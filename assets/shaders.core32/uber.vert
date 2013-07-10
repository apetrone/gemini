#version 150
/*
// uniform constant buffer
layout (shared) uniform cbuffer
{
	uniform mat4 modelview_matrix;
	uniform mat4 projection_matrix;
};
*/

uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 object_matrix;


in vec3 in_Position;

#if defined(D_VERTEX_NORMALS)
	in vec3 in_Normal;
	out vec3 ps_Normal;
#endif

#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) //&& !defined(D_DIFFUSE_MAP)
	in vec4 in_Color;
	out vec4 ps_Color;
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	in vec2 in_UV0;
	out vec2 ps_uv0;
#endif

#if defined(D_LIGHT_POSITION)
	uniform vec3 lightPosition;
	out vec3 lightDirection;
	uniform vec3 cameraPosition;
	out vec3 ps_View;
#endif

#if defined(D_CUBEMAP)
	out vec3 ps_CubeVertex;
#endif


void main()
{
#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) //&& !defined(D_DIFFUSE_MAP)
	ps_Color = in_Color;
#endif

#if defined(D_LIGHT_POSITION)
	vec3 eye_light = lightPosition;
	vec3 eye_vertex = vec3(object_matrix * vec4(in_Position, 1.0));
	lightDirection = (eye_light - eye_vertex);
	ps_View = cameraPosition - eye_vertex;
#endif

#if defined(D_VERTEX_NORMALS)
	ps_Normal = normalize( inverse(transpose(mat3(object_matrix))) * in_Normal );
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	ps_uv0 = in_UV0;
#endif

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * vec4(in_Position, 1.0));

#if defined(D_CUBEMAP)
	ps_CubeVertex = normalize(in_Position);
	gl_Position.z = gl_Position.w - 0.00001; // fix to far plane
#endif
}
