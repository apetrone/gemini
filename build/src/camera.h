// -------------------------------------------------------------
// Copyright (C) 2013- Adam Petrone

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM,OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// -------------------------------------------------------------
#pragma once


#include <gemini/typedefs.h>
#include <gemini/mathlib.h>


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
	
	// camera variables
	real yaw, pitch;
	glm::vec3 pos;
	
	glm::mat4 matProj;
	glm::mat4 matCam;
	glm::mat4 matCamProj; // matCam * matProj
	
	// directional vectors
	glm::vec3 view;
	glm::vec3 side;
	
	glm::vec3 target_lookatOffset;
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
	
	Camera(CameraType _type = FIRST_PERSON);
	
	// sets world position of the camera
	void set_absolute_position( const glm::vec3 & position );
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
}; // Camera
