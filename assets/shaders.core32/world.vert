uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 object_matrix;

// viewer world-space position
uniform vec3 viewer_direction;
uniform vec3 viewer_position;

// light position world-space
uniform vec3 light_position;

layout (location = 0) in vec4 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv0;
layout (location = 3) in vec2 in_uv1;

out vec3 ps_normal;
out vec2 ps_uv0;
out vec2 ps_uv1;

out vec3 vertex_position_world;
out vec3 light_position_world;
out vec3 vertex_to_viewer_worldspace;
out vec3 vertex_to_viewer_viewspace;

// summary of different coordinate spaces in the context of shaders
// local, or object-space
// world-space
// view, or camera-space
// surface, or tangent-space
// clip, or normalized-device-space
// screen-space

void compute_vertex_to_camera(
		in mat4 view,
		in mat4 world,
		in vec3 world_light_position,
		in vec4 vertex_objectspace,
		out vec3 surface_to_camera
	)
{
	vec3 light_position_viewspace = vec3((view * world) * vec4(world_light_position, 1.0));
	vec3 vertex_viewspace = vec3(((view * world) * in_position));

	surface_to_camera = light_position_viewspace - vertex_viewspace;
}

void main()
{
	// convert the vertex to world-space
	vertex_position_world = vec3(object_matrix * in_position);

	// create the normal matrix; this should be moved to the cpu
	mat3 normal_matrix = inverse(transpose(mat3(object_matrix)));

	compute_vertex_to_camera(modelview_matrix, object_matrix, light_position, in_position, vertex_to_viewer_viewspace);

	// transfer vars to the fragment shader
	vertex_to_viewer_worldspace = viewer_position - vertex_position_world;
	ps_normal = normal_matrix * in_normal;
	ps_uv0 = in_uv0;
	ps_uv1 = in_uv1;
	light_position_world = light_position;

	gl_Position = (projection_matrix * modelview_matrix * object_matrix * in_position);	
}