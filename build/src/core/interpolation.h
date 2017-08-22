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
#include <core/mathlib.h>

namespace gemini
{
	template <class Type>
	Type lerp(const Type & a, const Type & b, float t)
	{
		return glm::mix( a, b, t );
	}

	// -------------------------------------------------------------
	template <class Type>
	struct Interpolator
	{
		Type operator()( const Type & start, const Type & end, float t )
		{
			// return the linearly interpolated value
			return lerp( start, end, t );
		}
	};

	inline glm::quat custom_slerp(const glm::quat& q1, const glm::quat& q2, float t)
	{
		const double SLERP_TOLERANCE = 1.0e-5;
		glm::quat out;
		glm::quat q2b;

		float sq1, sq2;
		float cosom = q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2] + q1[3] * q2[3];

		if (cosom < 0.0f)
		{
			cosom = -cosom;
			q2b[0] = -q2[0];
			q2b[1] = -q2[1];
			q2b[2] = -q2[2];
			q2b[3] = -q2[3];
		}
		else
		{
			q2b = q2;
		}

		if ((1.0f + cosom) > SLERP_TOLERANCE)
		{
			if((1.0f - cosom) > SLERP_TOLERANCE)
			{
				float om = (float) acos(cosom);
				float rsinom = (float)(1.0f / sin(om));

				sq1 = (float)sin( (1.0f - t) * om) * rsinom;
				sq2 = (float)sin(t * om) * rsinom;
			}
			else
			{
				sq1 = (float)(1.0f - t);
				sq2 = t;
			}

			out[3] = sq1 * q1[3] + sq2 * q2b[3];
			out[0] = sq1 * q1[0] + sq2 * q2b[0];
			out[1] = sq1 * q1[1] + sq2 * q2b[1];
			out[2] = sq1 * q1[2] + sq2 * q2b[2];
		}
		else
		{
			sq1 = (float)sin( (1.0f - t) * 0.5f * mathlib::PI);
			sq2 = (float)sin(t * 0.5f * mathlib::PI);

			out[3] = sq1 * q1[3] + sq2 * q1[2];
			out[0] = sq1 * q1[0] + sq2 * q1[1];
			out[1] = sq1 * q1[1] + sq2 * q1[0];
			out[2] = sq1 * q1[2] + sq2 * q1[3];
		}

		return out;
	}


	template <class Type>
	Type slerp(const Type & a, const Type & b, float t)
	{
		return custom_slerp(a, b, t);

		// glm::mix has a bug where if the angles of the quaternions are too close;
		// they 'mix' to an invalid quaternion (NaN, NaN, NaN, NaN)
		//return glm::mix( a, b, t );

		// Groovounet: If you need a slerp that always take the short path, let me recommend to you to use shortMix.
		//return glm::shortMix(a, b, t);
	}


	// -------------------------------------------------------------
	template <>
	struct Interpolator<glm::quat>
	{
		glm::quat operator()( const glm::quat& start, const glm::quat& end, float t )
		{
			// return the linearly interpolated value
			return slerp(start, end, t);
		}
	}; // Interpolator
} // namespace gemini




