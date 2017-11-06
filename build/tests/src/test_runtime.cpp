// -------------------------------------------------------------
// Copyright (C) 2015- Adam Petrone
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
#include "unit_test.h"

#include <core/core.h>
#include <core/logging.h>

#include <platform/platform.h>

#include <runtime/asset_handle.h>
#include <runtime/asset_library.h>
#include <runtime/assets.h>
#include <runtime/debug_event.h>
#include <runtime/runtime.h>
#include <runtime/filesystem.h>
#include <runtime/jobqueue.h>
#include <runtime/geometry.h>
#include <runtime/http.h>

#include <assert.h>

#include <core/mathlib.h>

using namespace gemini;

// ---------------------------------------------------------------------
// asset library
// ---------------------------------------------------------------------

struct CustomAsset
{
	uint32_t id;
	platform::PathString uri;
};

class CustomAssetLibrary : public AssetLibrary2<CustomAsset, CustomAssetLibrary>
{
public:
	CustomAssetLibrary(Allocator& allocator)
		: AssetLibrary2(allocator)
	{
	}

	void create_asset(LoadState& state, void* parameters)
	{
		state.asset = new CustomAsset();
	}

	bool is_same_asset(CustomAsset* asset, void* parameters)
	{
		return true;
	}

	AssetLoadStatus load_asset(LoadState& state, const platform::PathString& fullpath, void* parameters)
	{
		state.asset->uri = fullpath;
		return AssetLoad_Success;
	}

	void destroy_asset(LoadState& state)
	{
		delete state.asset;
	}
};

UNITTEST(asset_library)
{
	Allocator allocator = memory_allocator_default(MEMORY_ZONE_ASSETS);
	CustomAssetLibrary lib(allocator);

	// check default asset
	CustomAsset default_asset;
	default_asset.id = 42;
	lib.default_asset(&default_asset);
	TEST_ASSERT_EQUALS(default_asset.id, lib.default_asset()->id);

	lib.prefix_path("shaders/120");

	AssetHandle handle;
	handle = lib.load("test1");
	TEST_ASSERT_TRUE(lib.handle_is_valid(handle));
	CustomAsset* asset0 = lib.lookup(handle);
	TEST_ASSERT_TRUE(asset0 != nullptr);

	// try loading the same asset again
	handle = lib.load("test1");
	CustomAsset* asset1 = lib.lookup(handle);
	TEST_ASSERT_EQUALS(asset0, asset1);

	platform::PathString fullpath("shaders/120/test1");
	platform::path::normalize(&fullpath[0]);
	TEST_ASSERT_EQUALS(asset1->uri, fullpath);
}


// ---------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------
void print_string(const char* data)
{
	LOGV("thread: 0x%x, string: %s\n", (size_t)platform::thread_id(), data);
}

float mat3_determinant(const glm::mat3& xform)
{
	return
		+xform[0][0] * (xform[1][1] * xform[2][2] - xform[2][1] * xform[1][2])
		- xform[1][0] * (xform[0][1] * xform[2][2] - xform[2][1] * xform[0][2])
		+ xform[2][0] * (xform[0][1] * xform[1][2] - xform[1][1] * xform[0][2]);
}

UNITTEST(Geometry)
{
	using namespace gemini;
	glm::vec3 vertices[] = {
		glm::vec3(-2.0f, 3.0f, 0.0f),
		glm::vec3(-2.0f, 1.0f, 0.0f),
		glm::vec3(2.0f, 3.0f, 0.0f),
		glm::vec3(2.0f, 1.0f, 0.0f)
	};

	glm::mat3 xf(
		3, 2, 4,
		2, 0, 2,
		4, 2, 3);

	//
	assert(mat3_determinant(xf) == glm::determinant(xf));

	OrientedBoundingBox box;
	compute_oriented_bounding_box_by_points(box, vertices, 4);

	//AssetHandle mesh_handle = mesh_load("models/plane");
}

// ---------------------------------------------------------------------
// jobqueue
// ---------------------------------------------------------------------
UNITTEST(jobqueue)
{
	const size_t MAX_JOB_ITERATIONS = 6;
	const char* iterations[] = {
		"ALPHA: 0",
		"ALPHA: 1",
		"ALPHA: 2",
		"ALPHA: 3",
		"ALPHA: 4",
		"ALPHA: 5",
		"ALPHA: 6",
		"ALPHA: 7",
		"ALPHA: 8",
		"ALPHA: 9",
		"BETA: 0",
		"BETA: 1",
		"BETA: 2",
		"BETA: 3",
		"BETA: 4",
		"BETA: 5",
		"BETA: 6",
		"BETA: 7",
		"BETA: 8",
		"BETA: 9",
		"DELTA: 0",
		"DELTA: 1",
		"DELTA: 2",
		"DELTA: 3",
		"DELTA: 4",
		"DELTA: 5",
		"DELTA: 6",
		"DELTA: 7",
		"DELTA: 8",
		"DELTA: 9",
		"EPSILON: 0",
		"EPSILON: 1",
		"EPSILON: 2",
		"EPSILON: 3",
		"EPSILON: 4",
		"EPSILON: 5",
		"EPSILON: 6",
		"EPSILON: 7",
		"EPSILON: 8",
		"EPSILON: 9",
		"FOXTROT: 0",
		"FOXTROT: 1",
		"FOXTROT: 2",
		"FOXTROT: 3",
		"FOXTROT: 4",
		"FOXTROT: 5",
		"FOXTROT: 6",
		"FOXTROT: 7",
		"FOXTROT: 8",
		"FOXTROT: 9",
		"GAMMA: 0",
		"GAMMA: 1",
		"GAMMA: 2",
		"GAMMA: 3",
		"GAMMA: 4",
		"GAMMA: 5",
		"GAMMA: 6",
		"GAMMA: 7",
		"GAMMA: 8",
		"GAMMA: 9"
	};

	gemini::Allocator default_allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);
	gemini::JobQueue jq(default_allocator);
	jq.create_workers(3);

	for (size_t index = 0; index < MAX_JOB_ITERATIONS; ++index)
	{
		jq.push_back(print_string, iterations[(index * 10) + 0]);
		jq.push_back(print_string, iterations[(index * 10) + 1]);
		jq.push_back(print_string, iterations[(index * 10) + 2]);
		jq.push_back(print_string, iterations[(index * 10) + 3]);
		jq.push_back(print_string, iterations[(index * 10) + 4]);
		jq.push_back(print_string, iterations[(index * 10) + 5]);
		jq.push_back(print_string, iterations[(index * 10) + 6]);
		jq.push_back(print_string, iterations[(index * 10) + 7]);
		jq.push_back(print_string, iterations[(index * 10) + 8]);
		jq.push_back(print_string, iterations[(index * 10) + 9]);

		jq.wait_for_jobs_to_complete();

		platform::thread_sleep(250);
	}

	LOGV("destroying workers...\n");
	jq.destroy_workers();
}

// ---------------------------------------------------------------------
// debug_event
// ---------------------------------------------------------------------
UNITTEST(debug_event)
{
	// Is there really anything to test here?
	//telemetry_host_startup("127.0.0.1", TELEMETRY_VIEWER_PORT);
	//telemetry_host_submit_frame();
	//telemetry_host_shutdown();
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------
UNITTEST(filesystem)
{
	core::filesystem::IFileSystem* fs = core::filesystem::instance();
	TEST_ASSERT(fs != nullptr, filesystem_instance_exists);

	//	platform::PathString content_path;
	//	content_path = fs->root_directory();
	//	content_path.append(PATH_SEPARATOR_STRING).append("builds").append(PATH_SEPARATOR_STRING).append(PLATFORM_NAME);
	//	fs->content_directory(content_path);


	//	platform::PathString absolute_path;
	//	TEST_ASSERT(fs->get_absolute_path_for_content(absolute_path, "conf/shaders.conf") == false, get_absolute_path_for_content_missing);
}


// ---------------------------------------------------------------------
// http
// ---------------------------------------------------------------------
UNITTEST(http)
{
	platform::net_startup();

	gemini::http_startup();

	//gemini::http_request_file("https://www.google.com/images/branding/googlelogo/2x/googlelogo_color_272x92dp.png", "./downloads/download.jpg", "httplib");
	//while (gemini::http_active_download_count() > 0)
	//{
	//	gemini::http_update();
	//}

	gemini::http_shutdown();

	platform::net_shutdown();
}


// ---------------------------------------------------------------------
// RTSP
// ---------------------------------------------------------------------
UNITTEST(rtsp)
{
	platform::net_startup();
	gemini::http_startup();



	http_connection connection;
	memset(&connection, 0, sizeof(http_connection));




	//gemini::rtsp_describe(&connection, &request, "192.168.0.157", "test");

	gemini::Allocator allocator = gemini::memory_allocator_default(MEMORY_ZONE_DEFAULT);

	gemini::string response = gemini::string_create(
"RTSP/1.0 401 Unauthorized\n\
CSeq: 1\n\
Server : Hipcam RealServer / V1.0\n\
WWW - Authenticate : Digest realm = \"Hipcam RealServer/V1.0\", nonce = \"35c91af69802517dd2d4413d836037e6\"");

	Array<gemini::string> lines(allocator);
	string_split(allocator, lines, response, "\n");

	LOGV("total lines: %i\n", lines.size());

	//for (size_t index = 0; index < lines.size(); ++index)
	//{
	//	LOGV("line: %i -> '%s'\n", index, lines[index].c_str());
	//	string_destroy(allocator, lines[index]);
	//}

	http_request request;
	memset(&request, 0, sizeof(http_request));
	HashSet<gemini::string, gemini::string> headers(allocator);
	gemini::rtsp_parse_response(allocator, &request, headers, lines);



	for (size_t index = 0; index < lines.size(); ++index)
	{
		//LOGV("line: %i -> '%s'\n", index, lines[index].c_str());
		string_destroy(allocator, lines[index]);
	}

	LOGV("nonce = '%s'\n", request.nonce_value.c_str());
	LOGV("CSeq = '%s'\n", request.client_sequence_id.c_str());




	//md5("username:realm:password");

	"Authentication: Digest username = \"admin\", realm = \"Hipcam RealServer/V1.0\", nonce = \"35c91af69802517dd2d4413d836037e6\", response = \"\"";


	// A1 = username:realm:password
	// A2 = <Method> ":" <digest-uri-value>

	//request - digest = "md5(md5(A1):nonce-value:md5(A2))"

	string_destroy(allocator, request.nonce_value);
	string_destroy(allocator, request.client_sequence_id);

	gemini::http_shutdown();
	platform::net_shutdown();
}



// ---------------------------------------------------------------------
// logging
// ---------------------------------------------------------------------
UNITTEST(logging)
{
	TEST_ASSERT(core::logging::instance() != nullptr, log_instance_is_valid);

	LOGV("This is a test of the logging system!\n");

	LOGW("This is a warning\n");
	LOGE("This is an error!\n");

	LOGW("Warning, %i parameters missing!\n", 3);
}

// https://tools.ietf.org/html/rfc4648
void base64_indices_from_bytes(uint8_t* tuple, const uint8_t* bytes)
{
	// encode index table (from 6-bit value to index)
	const uint8_t index_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	// 6-bit bitmask
	const uint32_t bitmask = (1 << 6) - 1;

	// extract 6-bits from each input octet and convert to index
	tuple[0] = index_table[ (bytes[0] >> 2) & bitmask ];
	tuple[1] = index_table[ ((bytes[0] & 0x03) << 4) + (bytes[1] >> 4) ];
	tuple[2] = index_table[ ((bytes[1] & 0xF) << 2) + (bytes[2] >> 6) ];
	tuple[3] = index_table[ bytes[2] & bitmask ];
} // base64_indices_from_bytes

void base64_encode(const void* data, size_t data_size, Array<char>& output)
{
	size_t triplets = data_size / 3;
	size_t extra_bytes = data_size % 3;
	size_t total_triplets = triplets;

	// If there is an uneven number of triplets, we increment the total.
	if (extra_bytes > 0)
	{
		total_triplets++;
	}
	output.resize(total_triplets * 4);

	size_t output_index = 0;
	for (size_t triplet = 0; triplet < triplets; ++triplet)
	{
		const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data) + (triplet * 3);
		uint8_t tuple[4];
		base64_indices_from_bytes(tuple, bytes);
		output[output_index++] = tuple[0];
		output[output_index++] = tuple[1];
		output[output_index++] = tuple[2];
		output[output_index++] = tuple[3];
	}

	// If there isn't an even number of triplets, we'll have to pad the output.
	if (extra_bytes == 1)
	{
		// We pad the extra two bytes with zeros and replace the indices with =
		uint8_t bytes[3];

		const char* chars = reinterpret_cast<const char*>(data) + (triplets * 3);
		bytes[0] = chars[0];
		bytes[1] = 0;
		bytes[2] = 0;

		uint8_t tuple[4];
		base64_indices_from_bytes(tuple, bytes);
		output[output_index++] = tuple[0];
		output[output_index++] = tuple[1];
		output[output_index++] = '=';
		output[output_index++] = '=';
	}
	else if (extra_bytes == 2)
	{
		// We pad the extra byte with a zero and replace the index with =
		uint8_t bytes[3];

		const char* chars = reinterpret_cast<const char*>(data) + (triplets * 3);
		bytes[0] = chars[0];
		bytes[1] = chars[1];
		bytes[2] = 0;

		uint8_t tuple[4];
		base64_indices_from_bytes(tuple, bytes);
		output[output_index++] = tuple[0];
		output[output_index++] = tuple[1];
		output[output_index++] = tuple[2];
		output[output_index++] = '=';
	}
} // base64_encode

UNITTEST(base64_encoding)
{
	Allocator default_allocator = memory_allocator_default(MEMORY_ZONE_DEFAULT);

	// base 64 encode multiple input octets.
	const size_t test_encodings = 3;
	const char* input[] = { "M", "Ma", "Man" };

	size_t input_sizes[] = { 1, 2, 3 };

	const char* encoded[] = { "TQ==", "TWE=", "TWFu" };

	for (size_t index = 0; index < test_encodings; ++index)
	{
		Array<char> output(default_allocator);

		// base64 encode it
		base64_encode(input[index], input_sizes[index], output);

		// compare it
		TEST_ASSERT_TRUE(output.size() > 0);

		bool matches = true;
		for (size_t byte = 0; byte < output.size(); ++byte)
		{
			matches = matches && (encoded[index][byte] == output[byte]);
		}
		TEST_ASSERT_TRUE(matches);

		// string is not NULL terminated, so we use this format.
		if (!matches)
		{
			LOGV("[%s] -> output is '%.*s'\n", input[index], output.size(), &output[0]);
		}
	}
} // base64_encoding


struct Hash128
{
	uint32_t a, b, c, d;

	Hash128(uint32_t _a = 0, uint32_t _b = 0, uint32_t _c = 0, uint32_t _d = 0)
		: a(_a)
		, b(_b)
		, c(_c)
		, d(_d)
	{
	}

	void zero()
	{
		a = b = c = d = 0;
	}

	bool operator==(const Hash128& other) const
	{
		return (a == other.a && b == other.b && c == other.c && d == other.d);
	}
}; // Hash128

// left-rotate value by bits.
inline uint32_t left_rotate(uint32_t value, uint8_t bits)
{
	return (value << bits) | (value >> (32 - bits));
}

// We define the functions:
// F(x, y, z) = (X & Y) | ~X Z
// G(x, y, z) = (X & Z) | Y ~Z
// H(x, y, z) = X ^ Y ^ Z
// I(x, y, z) = Y ^ (X | ~Z)

inline uint32_t md5_round_f(
	uint32_t a, uint32_t b, uint32_t c, uint32_t d,
	uint32_t x, uint32_t t, uint32_t s)
{
	a += ((b & c) | ~b & d) + x + t;
	return b + left_rotate(a, s);
}

inline uint32_t md5_round_g(
	uint32_t a, uint32_t b, uint32_t c, uint32_t d,
	uint32_t x, uint32_t t, uint32_t s)
{
	a += ((b & d) | c & ~d) + x + t;
	return b + left_rotate(a, s);
}

inline uint32_t md5_round_h(
	uint32_t a, uint32_t b, uint32_t c, uint32_t d,
	uint32_t x, uint32_t t, uint32_t s)
{
	a += (b ^ c ^ d) + x + t;
	return b + left_rotate(a, s);
}

inline uint32_t md5_round_i(
	uint32_t a, uint32_t b, uint32_t c, uint32_t d,
	uint32_t x, uint32_t t, uint32_t s)
{
	a += (c ^ (b | ~d)) + x + t;
	return b + left_rotate(a, s);
}

//#define MD5_ROUND_F(a, b, c, d, k, s, i)\
//	a = b + left_rotate(a + ((b & c) | ~b & d) + X[k] + table[i], s)
//
//#define MD5_ROUND_G(a, b, c, d, k, s, i)\
//	a = b + left_rotate(a + ((b & d) | c & ~d) + X[k] + table[i], s)
//
//#define MD5_ROUND_H(a, b, c, d, k, s, i)\
//	a = b + left_rotate(a + (b ^ c ^ d) + X[k] + table[i], s)
//
//#define MD5_ROUND_I(a, b, c, d, k, s, i)\
//	a = b + left_rotate(a + (c ^ (b | ~d)) + X[k] + table[i], s)

Hash128 md5_digest(const char* data, uint32_t data_size)
{
	// Algorithm based on the wikipedia entry.
	// https://en.wikipedia.org/wiki/MD5

	// https://tools.ietf.org/html/rfc1321
	// http://onlinemd5.com/

	// 'word' in this context is 32-bits.

	uint32_t round_shifts[] = {
		7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
		5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
		4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
		6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
	};

	uint32_t table[] = {
		0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
		0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
		0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
		0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
		0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
		0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
		0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
		0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
		0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
		0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
		0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
		0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
		0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
		0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
		0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
		0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
	};


	uint32_t initial_bits = (data_size * 8);

	uint32_t new_bits = initial_bits + 1;
	uint32_t appended_bits = 0;

	while (true)
	{
		if ((new_bits + appended_bits) % 512 == 448)
		{
			break;
		}
		else
		{
			appended_bits += 1;
		}
	}

	// minimum of 56 bytes (448 bits)
	uint32_t total_bits = (new_bits + appended_bits) + 64;
	const uint32_t total_bytes = total_bits / 8;
	LOGV("total bytes: %i\n", total_bytes);

	const uint32_t extra_bytes = (appended_bits + 1) / 8;
	const uint32_t new_message_size = data_size + extra_bytes + 8;

	uint8_t* message = reinterpret_cast<uint8_t*>(malloc(new_message_size));
	memset(message, 0, new_message_size);
	memcpy(message, data, data_size); // copy original message
	message[data_size] = 128; // appended bit

	const uint32_t data_size_bits = 8 * data_size;
	memcpy(&message[data_size + extra_bytes], &data_size_bits, 4);

	// pre-processing: add a single 1 bit to the message.
	// message[length] = 128;
	// message[pad_bits...] = 0;

	// pre-processing: pad message with zeros until (bits % 512) is 448.
	// pre-processing: append original length in 64 bits to message.
	// pre-processing: the result a multiple of 512-bits.



	// initialize the hash
	Hash128 hash(0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476);

	// message is multiple of 16 blocks, where each block is 32-bits wide.
	uint32_t total_blocks = total_bits / 16;

	uint32_t a0 = hash.a;
	uint32_t b0 = hash.b;
	uint32_t c0 = hash.c;
	uint32_t d0 = hash.d;

	// process in chunks of 512-bits (or sixteen 32-bit values), or 64 bytes.
	for (size_t block_index = 0; block_index < new_message_size; block_index += 64)
	{
		// chunk is converted to sixteen 32-bit words.
		uint32_t X[16];
		for (size_t j = 0; j < 16; ++j)
		{
			uint8_t* p = &message[block_index + 4 * j];
			X[j] = p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
		}

		uint32_t a = a0;
		uint32_t b = b0;
		uint32_t c = c0;
		uint32_t d = d0;

		for (size_t index = 0; index < 64; ++index)
		{
			uint32_t F, g;
			if (index < 16)
			{
				F = (b & c) | ((~b) & d);
				g = index;
			}
			else if (index < 32)
			{
				F = (d & b) | ((~d) & c);
				g = (5 * index + 1) % 16;
			}
			else if (index < 48)
			{
				F = (b ^ c ^ d);
				g = (3 * index + 5) % 16;
			}
			else
			{
				F = c ^ (b | (~d));
				g = (7 * index) % 16;
			}

			F = F + a + table[index] + X[g];
			a = d;
			d = c;
			c = b;
			b = b + left_rotate(F, round_shifts[index]);
		}

		a0 += a;
		b0 += b;
		c0 += c;
		d0 += d;
	}

	hash.a = a0;
	hash.b = b0;
	hash.c = c0;
	hash.d = d0;

	free(message);
	return hash;
}

int main(int, char**)
{
	gemini::core_startup();
	gemini::runtime_startup("arcfusion.net/gemini/test_runtime");

	using namespace gemini;

	Hash128 hash;

	//assert(left_rotate(224, 2) == 131);

	uint32_t mv = 0x1234567; // 0: big endian, 1: little endian.
	uint32_t a = (*((uint8_t*)(&mv))) == 0x67;

	const Hash128 arcfusion(0x740367D8, 0x051FD5FD, 0xCE3E906A, 0xAB34FEBF);
	hash = md5_digest("arcfusion", 9);
	assert(hash == arcfusion);

	//hash = md5_digest("a", 1);
	//const Hash128 a_hash(0xCC175B9C, 0x0F1B6A83, 0x1C399E26, 0x9772661);
	//assert(hash == a_hash);

	//hash = md5_digest("", 0);
	//const Hash128 empty_hash(0xd41d8cd9, 0x8f00b204, 0xe9800998, 0xecf8427e);
	//assert(hash == empty_hash);

	//const Hash128 quick_brown_fox(0xe4d909c2, 0x90d0fb1c, 0xa068ffad, 0xdf22cbd0);
	//hash = md5_digest("The quick brown fox jumps over the lazy dog", 43);
	//assert(hash == quick_brown_fox);

	//unittest::UnitTest::execute();
	//UNITTEST_EXECUTE(rtsp);
	gemini::runtime_shutdown();
	gemini::core_shutdown();
	return 0;
}
