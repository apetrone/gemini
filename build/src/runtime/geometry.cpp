// -------------------------------------------------------------
// Copyright (C) 2016- Adam Petrone
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
#include <runtime/geometry.h>

//#include <core/logging.h>

namespace gemini
{
	bool AABB2::overlaps(const AABB2& other) const
	{
		if (left > other.right)
		{
			return false;
		}
		else if (right < other.left)
		{
			return false;
		}

		if (bottom < other.top)
		{
			return false;
		}
		else if (top > other.bottom)
		{
			return false;
		}

		return true;
	} // overlaps

	float covariance(size_t row, size_t column, float* values, size_t total_values, size_t stride)
	{
		// covariance(i, j) = 1/n * (sum[xi * xj] - (E[xi] * E[xj])

		float denom = 1.0f / static_cast<float>(total_values);

		float sum = 0.0f;
		uint8_t* row_pointer = reinterpret_cast<uint8_t*>(values + row);
		uint8_t* column_pointer = reinterpret_cast<uint8_t*>(values + column);
		float xi_sum = 0.0f;
		float xj_sum = 0.0f;
		for (size_t index = 0; index < total_values; ++index)
		{
			float* row_value = reinterpret_cast<float*>(row_pointer);
			float* column_value = reinterpret_cast<float*>(column_pointer);
			//LOGV("%2.2f, %2.2f\n", *row_value, *column_value);
			sum += (*row_value * *column_value);

			xi_sum += *row_value;
			xj_sum += *column_value;

			row_pointer += stride;
			column_pointer += stride;
		}

		xi_sum *= denom;
		xj_sum *= denom;

		return (sum * denom) - (xi_sum * xj_sum);
	} // covariance

	void compute_oriented_bounding_box_by_points(OrientedBoundingBox& box, glm::vec3* vertices, size_t total_vertices)
	{
		// https://hewjunwei.wordpress.com/2013/01/26/obb-generation-via-principal-component-analysis/
		// http://jamesgregson.blogspot.com/2011/03/latex-test.html

		glm::mat3 covariance_matrix;
		const size_t STRIDE = sizeof(float) * 3;
		float* vertex_pointer = reinterpret_cast<float*>(vertices);
		covariance_matrix[0][0] = covariance(0, 0, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[0][1] = covariance(0, 1, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[0][2] = covariance(0, 2, vertex_pointer, total_vertices, STRIDE);

		covariance_matrix[1][0] = covariance(1, 0, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[1][1] = covariance(1, 1, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[1][2] = covariance(1, 2, vertex_pointer, total_vertices, STRIDE);

		covariance_matrix[2][0] = covariance(2, 0, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[2][1] = covariance(2, 1, vertex_pointer, total_vertices, STRIDE);
		covariance_matrix[2][2] = covariance(2, 2, vertex_pointer, total_vertices, STRIDE);


		glm::vec3 basis[3];
		// TODO@apetrone: Compute the eigenvectors!
		// https://en.wikipedia.org/wiki/QR_algorithm
		basis[0] = glm::normalize(glm::column(covariance_matrix, 0));
		basis[1] = glm::normalize(glm::column(covariance_matrix, 1));
		basis[2] = glm::normalize(glm::column(covariance_matrix, 2));

		// loop through all coordinates and transform; keeping
		// min and maxs
		glm::vec3 mins(999.0f, 999.0f, 999.0f);
		glm::vec3 maxs(-999.0f, -999.0f, -999.0f);

		box.rotation = glm::column(box.rotation, 0, basis[0]);
		box.rotation = glm::column(box.rotation, 1, basis[1]);
		box.rotation = glm::column(box.rotation, 2, basis[2]);

		for (size_t index = 0; index < total_vertices; ++index)
		{
			glm::vec3 pt(vertex_pointer[index * 3], vertex_pointer[index * 3 + 1], vertex_pointer[index * 3 + 2]);
			float xval = glm::dot(pt, basis[0]);
			float yval = glm::dot(pt, basis[1]);
			float zval = glm::dot(pt, basis[2]);

			if (xval < mins.x)
			{
				mins.x = xval;
			}
			if (xval > maxs.x)
			{
				maxs.x = xval;
			}

			if (yval < mins.y)
			{
				mins.y = yval;
			}
			if (yval > maxs.y)
			{
				maxs.y = yval;
			}

			if (zval < mins.z)
			{
				mins.z = zval;
			}
			if (zval > maxs.z)
			{
				maxs.z = zval;
			}
		}

		// compute center and half extents
		box.positive_extents = (maxs - mins) * 0.5f;
		box.center = ((maxs + mins) * 0.5f);

#if 0
		glm::mat2 mx;

		mx[0][0] = covariance(0, 0, values, 4, sizeof(float) * 2);
		mx[0][1] = covariance(0, 1, values, 4, sizeof(float) * 2);
		mx[1][0] = covariance(1, 0, values, 4, sizeof(float) * 2);
		mx[1][1] = covariance(1, 1, values, 4, sizeof(float) * 2);

		glm::vec2 u = glm::normalize(glm::column(mx, 0));
		glm::vec2 v = glm::normalize(glm::column(mx, 1));

		// loop through all coordinates and transform; keeping
		// min and maxs
		glm::vec2 mins(999.0f, 999.0f), maxs(-999.0f, -999.0f);

		for (size_t index = 0; index < 4; ++index)
		{
			glm::vec2 pt(values[index * 2], values[index * 2 + 1]);
			float xval = glm::dot(pt, u);
			float yval = glm::dot(pt, v);

			if (xval < mins.x)
				mins.x = xval;

			if (xval > maxs.x)
				maxs.x = xval;

			if (yval < mins.y)
				mins.y = yval;

			if (yval > maxs.y)
				maxs.y = yval;
		}

		glm::vec2 half_extents = (maxs - mins) * 0.5f;
		glm::vec2 center = (maxs + mins) * 0.5f;

		// multiply center by T to get the world space center of the box.
#endif
	} // compute_oriented_bounding_box_by_points
} // namespace gemini
