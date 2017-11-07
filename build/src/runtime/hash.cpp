// -------------------------------------------------------------
// Copyright (C) 2017- Adam Petrone
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
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------
#include <runtime/hash.h>


namespace gemini
{
	#define ENDIAN_SWAP_UINT32(value) \
		(value & 0xff) << 24 | (value & 0xff00) << 8 | (value & (0xff << 16)) >> 8 | (value & (0xff << 24)) >> 24;

	// left-rotate value by bits.
	inline uint32_t left_rotate(uint32_t value, uint8_t bits)
	{
		return (value << bits) | (value >> (32 - bits));
	}

	inline uint32_t md5_round_f(
		uint32_t a, uint32_t b, uint32_t c, uint32_t d,
		uint32_t x, uint32_t t, uint32_t s)
	{
		return b + left_rotate(a + ((b & c) | ~b & d) + x + t, s);
	}

	inline uint32_t md5_round_g(
		uint32_t a, uint32_t b, uint32_t c, uint32_t d,
		uint32_t x, uint32_t t, uint32_t s)
	{
		return b + left_rotate(a + ((b & d) | c & ~d) + x + t, s);
	}

	inline uint32_t md5_round_h(
		uint32_t a, uint32_t b, uint32_t c, uint32_t d,
		uint32_t x, uint32_t t, uint32_t s)
	{
		return b + left_rotate(a + (b ^ c ^ d) + x + t, s);
	}

	inline uint32_t md5_round_i(
		uint32_t a, uint32_t b, uint32_t c, uint32_t d,
		uint32_t x, uint32_t t, uint32_t s)
	{
		return b + left_rotate(a + (c ^ (b | ~d)) + x + t, s);
	}

	Hash128 md5_digest(const char* data, uint32_t data_size)
	{
		// Algorithm based on the wikipedia entry and rfc1321 docs.
		// https://en.wikipedia.org/wiki/MD5
		// https://tools.ietf.org/html/rfc1321

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
		// pre-processing: add a single 1 bit to the message.
		// pre-processing: pad message with zeros until (bits % 512) is 448.
		// pre-processing: append original length in 64 bits to message.
		// pre-processing: the result a multiple of 512-bits.
		uint32_t total_bits = (new_bits + appended_bits) + 64;
		const uint32_t total_bytes = total_bits / 8;

		const uint32_t extra_bytes = (appended_bits + 1) / 8;
		const uint32_t new_message_size = data_size + extra_bytes + 8;

		uint8_t* message = reinterpret_cast<uint8_t*>(malloc(new_message_size));
		memset(message, 0, new_message_size);
		memcpy(message, data, data_size); // copy original message
		message[data_size] = 128; // appended bit

		const uint32_t data_size_bits = 8 * data_size;
		memcpy(&message[data_size + extra_bytes], &data_size_bits, 4);

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

		hash.a = ENDIAN_SWAP_UINT32(a0);
		hash.b = ENDIAN_SWAP_UINT32(b0);
		hash.c = ENDIAN_SWAP_UINT32(c0);
		hash.d = ENDIAN_SWAP_UINT32(d0);

		free(message);
		return hash;
	} // md5_digest
} // namespace gemini