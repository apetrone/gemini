// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//		* Redistributions of source code must retain the above copyright notice,
//		this list of conditions and the following disclaimer.

//		* Redistributions in binary form must reproduce the above copyright notice,
//		this list of conditions and the following disclaimer in the documentation
//		and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#pragma once

#include <core/mathlib.h>
#include <platform/typedefs.h>

// REFERENCE LINKS

//
// TUTORIALS / REFERENCE
//
//http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
//http://www.gamedev.net/reference/articles/article1927.asp - Camera Class Tutorial
//http://www.flipcode.com/cgi-bin/fcarticles.cgi?show=64156 - OpenGL camera
//http://www.gamedev.net/reference/articles/article1997.asp - A Simple vec4-based Camera
//http://www.gamedev.net/reference/articles/article2160.asp - Creating a Useful Camera Class
//http://www.devmaster.net/articles/viewing-systems/ - Cameras for 3D engines (UVN)
//http://www.flipcode.com/documents/matrfaq.html - Matrix FAQ (huge)
//http://www.gamasutra.com/features/19980703/quaternions_01.htm - rotating objects using quaternions
//http://www.gamedev.net/reference/articles/article2023.asp - virtual camera position
//http://www.gamedev.net/reference/articles/article1591.asp - a simple third person camera using the polar coordinate system


//
// FORUM THREADS
//
//http://www.gamedev.net/community/forums/topic.asp?topic_id=321402 - last post by jyk; his code works.
//http://www.gamedev.net/community/forums/topic.asp?topic_id=421529 - manual alternative to gluLookAt
//http://www.gamedev.net/community/forums/topic.asp?topic_id=419777 - reconstruct viewing matrix
//http://www.gamedev.net/community/forums/topic.asp?topic_id=418443 - Odd Quaternion Camera, jyk posts helpful info on cameras
//http://www.gamedev.net/community/forums/topic.asp?topic_id=321442 - Pitch/Yaw problem
//http://www.gamedev.net/community/forums/topic.asp?topic_id=432017 - Pitch / Yaw around mesh while matching roll
//http://www.gamedev.net/community/forums/topic.asp?topic_id=431476 - Quaternions, standard X Y Z rotation
//http://www.gamedev.net/community/forums/topic.asp?topic_id=413071 - from direction vector to complete matrix
//http://www.gamedev.net/community/forums/topic.asp?topic_id=418024 - Quaternions step by step
//http://www.gamedev.net/community/forums/topic.asp?topic_id=311086 - Use Quaternions for spaceship orientations
//http://www.gamedev.net/community/forums/topic.asp?topic_id=334484 - Camera question (Space Sim)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=428775 - Camera for spaceship games
//http://www.gamedev.net/community/forums/topic.asp?topic_id=414736 - 3D Vector Rotation Issue
//http://www.gamedev.net/community/forums/topic.asp?topic_id=313276 - Vector rotation + trigonometry question
//http://www.gamedev.net/community/forums/topic.asp?topic_id=423165 - Rotation values from Tangent to Curve (vector)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=146259 - Camera Code?
//http://www.gamedev.net/community/forums/topic.asp?topic_id=449625 - camera class
//http://www.scotboyd.net/90percent/2007/05/quaternion-camera-in-xna.html - Quaternion Camera in XNA
//http://www.gamedev.net/community/forums/topic.asp?topic_id=376956 - direction vector in FPS style games
//http://www.gamedev.net/community/forums/topic.asp?topic_id=437766 - How to rotate a vector (relative)
//http://www.gamedev.net/community/forums/topic.asp?topic_id=261028 - camera problems

// TargetCamera References
// http://www.gamedev.net/community/forums/topic.asp?topic_id=473371 - cursor moving faster than object
// http://www.gamedev.net/community/forums/topic.asp?topic_id=480789 - Rotating a group of points around a pivot


struct Camera
{
	enum CameraType
	{
		FIRST_PERSON,
		TARGET
	};
		
	Camera(CameraType _type = FIRST_PERSON);
	
	// sets world position of the camera

	void invert_yaxis( bool invert ) { invert_y_axis = invert; }
	
	// called when the mouse moves to update the view
	void move_view( int32_t dx, int32_t dy );
	
	// position-based movement
	void move_left( real dt );
	void move_right( real dt );
	void move_forward( real dt );
	void move_backward( real dt );
	void move_along_vector( const glm::vec3 & v, real dt );
	
	// internal functions
	virtual void update_view();
	
	// projection type functions
	void perspective( real fovy, int32_t width, int32_t height, real nearz, real farz );
	void ortho( real left, real right, real bottom, real top, real nearz, real farz );
	
	const glm::mat4& get_inverse_world_transform() const { return inverse_world_transform; }
	const glm::mat4& get_inverse_rotation() const { return inverse_rotation; }
	
	
	void set_yaw(real y) { yaw = y; }
	void set_pitch(real p) { pitch = p; }
	void set_position(const glm::vec3& position);
	const glm::vec3& get_position() const { return pos; }
		
	void set_view(const glm::vec3& view_direction) { view = view_direction; }
	void set_type(const CameraType cameratype) { type = cameratype; }
	void set_target_offset(const glm::vec3& in_target_offset) { target_offset = in_target_offset; }
	
	const glm::mat4& get_modelview() const { return modelview; }
	const glm::mat4& get_projection() const { return projection; }
	const glm::vec3& get_view() const { return view; }
	const glm::vec3& get_eye_position() const { return eye_position; }
	
private:
	// camera variables
	real yaw;
	real pitch;
	glm::vec3 pos;
	
	glm::mat4 projection;
	glm::mat4 modelview;
	
	// components as matrices
	glm::mat4 inverse_rotation;
	glm::mat4 inverse_translation;
	
	// final inverted world matrix
	glm::mat4 inverse_world_transform;
	
	// directional vectors
	glm::vec3 view;
	glm::vec3 side;
	
	glm::vec3 target_offset;
	glm::vec3 eye_position;
	
	CameraType type;
	
	//
	// options
	real move_speed;
	real sensitivity;
	bool invert_y_axis;
	bool is_ortho;
	float aspect_ratio;
	float fovy;
	
	float near_clip;
	float far_clip;
}; // Camera
