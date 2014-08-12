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

#if defined(D_HARDWARE_SKINNING)
	uniform mat4 node_transforms[60];
#endif


in vec4 in_Position;

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

#if defined(D_VERTEX_UV1) && defined(D_LIGHTMAP)
	in vec2 in_UV1;
	out vec2 ps_uv1;
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

#if defined(D_HARDWARE_SKINNING)
	in vec4 in_blendindices;
	in vec4 in_blendweights;
#endif

void main()
{
#if defined(D_VERTEX_COLORS) && !defined(D_DIFFUSE_COLOR) //&& !defined(D_DIFFUSE_MAP)
	ps_Color = in_Color;
#endif

#if defined(D_LIGHT_POSITION)
	vec3 eye_light = lightPosition;
	vec3 eye_vertex = vec3(object_matrix * in_Position);
	lightDirection = (eye_light - eye_vertex);
	ps_View = cameraPosition - eye_vertex;
#endif

#if defined(D_VERTEX_NORMALS)
	ps_Normal = normalize( inverse(transpose(mat3(object_matrix))) * in_Normal );
#endif

#if defined(D_VERTEX_UV0) && defined(D_DIFFUSE_MAP) || defined(D_SPECULAR_MAP)
	ps_uv0 = in_UV0;
#endif

#if defined(D_VERTEX_UV1) && defined(D_LIGHTMAP)
	ps_uv1 = in_UV1;
#endif

#if defined(D_HARDWARE_SKINNING)
	vec4 final_vertex = vec4(0.0);
	final_vertex += (node_transforms[int(in_blendindices.x)] * in_Position) * in_blendweights.x;
	final_vertex += (node_transforms[int(in_blendindices.y)] * in_Position) * in_blendweights.y;
	final_vertex += (node_transforms[int(in_blendindices.z)] * in_Position) * in_blendweights.z;
	final_vertex += (node_transforms[int(in_blendindices.w)] * in_Position) * in_blendweights.w;
#else
	vec4 final_vertex = in_Position;
#endif

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * final_vertex);

#if defined(D_CUBEMAP)
	ps_CubeVertex = normalize(vec3(in_Position));
	gl_Position.z = gl_Position.w - 0.00001; // fix to far plane
#endif
}
