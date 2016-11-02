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

#include <core/typedefs.h>
#include <core/util.h> // for std::function
#include <platform/platform.h>

// TODO: for test slerp: move this elsewhere
#include <core/mathlib.h>
#include <core/interpolation.h>

#include <string>
#include <vector>

namespace platform
{
	struct Result;
}

namespace core
{
	namespace argparse
	{
		class ArgumentParser;
	}
}

namespace gemini
{
	// runtime startup sequence
	// Optionally pass a lambda to setup the filesystem paths
	// This is called after the filesystem instance is created, but before
	// any other operations. It allows the caller to setup the filesystem
	// paths in a custom fashion.
	platform::Result runtime_startup(const char* application_data_path, std::function<void(const char*)> custom_path_setup = nullptr);
	void runtime_shutdown();


	void runtime_load_arguments(std::vector<std::string>& arguments, ::core::argparse::ArgumentParser& parser);

	int32_t runtime_decompose_url(const char* url, char* filename, char* hostname, char* service_type, uint16_t* port);



	template <class T>
	class Test
	{
	private:
		T initial;
		T goal;
		T& current;
		float alpha;
		float current_time;
		float lerp_time;

	public:
		Test(T& _current)
			: current(_current)
			, alpha(0.0f)
			, current_time(0.0f)
		{
			initial = T();
			goal = T();
			lerp_time = 0.0f;
		}

		~Test()
		{}

		Test(const Test& other) = delete;
		Test(const Test&& other) = delete;
		const Test& operator=(const Test& other) = delete;
		const Test&& operator=(const Test&& other) = delete;

		// used to initiate a transition from initial_value to target_value
		void start_lerp(const T& initial_value, const T& target_value, float transition_time_seconds)
		{
			initial = initial_value;
			goal = target_value;
			current_time = 0.0f;
			lerp_time = transition_time_seconds;
		}

		void tick(float step_interval_seconds)
		{
			current_time += step_interval_seconds;
			alpha = glm::clamp((current_time / lerp_time), 0.0f, 1.0f);
			current = gemini::lerp(initial, goal, alpha);
		}

		void reset()
		{
			current_time = 0.0f;
		}
	};


	// quaternion
	template <>
	class Test<glm::quat>
	{
	private:
		glm::quat initial;
		glm::quat goal;
		glm::quat& current;
		float alpha;
		float current_time;
		float lerp_time;

	public:
		Test(glm::quat& _current)
			: current(_current)
			, alpha(0.0f)
			, current_time(0.0f)
		{
			initial = glm::quat();
			goal = glm::quat();
			lerp_time = 0.0f;
		}

		~Test()
		{}

		Test(const Test& other) = delete;
		Test(const Test&& other) = delete;
		const Test& operator=(const Test& other) = delete;
		const Test&& operator=(const Test&& other) = delete;

		// used to initiate a transition from initial_value to target_value
		void start_lerp(const glm::quat& initial_value, const glm::quat& target_value, float transition_time_seconds)
		{
			initial = initial_value;
			goal = target_value;
			current_time = 0.0f;
			lerp_time = transition_time_seconds;
		}

		void tick(float step_interval_seconds)
		{
			current_time += step_interval_seconds;
			alpha = glm::clamp((current_time / lerp_time), 0.0f, 1.0f);
			current = gemini::slerp(initial, goal, alpha);
		}

		void reset()
		{
			current_time = 0.0f;
		}
	};


} // namespace gemini
