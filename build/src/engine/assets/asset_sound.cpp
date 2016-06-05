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
#include <runtime/filesystem.h>
#include <core/stackstring.h>

#include "assets.h"
#include "assets/asset_sound.h"

#include <platform/kernel.h> // for choosing the extension

using namespace renderer;

namespace gemini
{
	namespace assets
	{
		// code for reading and writing WAVE files
		// http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/wave.html
		// http://soundfile.sapp.org/doc/WaveFormat/
#define MAKE_RIFF_CODE(a) static_cast<uint32_t>(((a[0]) << 0 | (a[1]) << 8 | (a[2]) << 16 | (a[3]) << 24))
#ifndef WAVE_FORMAT_PCM
		const size_t WAVE_FORMAT_PCM = 0x001;
		const size_t WAVE_FORMAT_IEEE_FLOAT = 0x0003;
		const size_t WAVE_FORMAT_ALAW = 0x0006;
		const size_t WAVE_FORMAT_MULAW = 0x0007;
		const size_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;
#endif

		const uint32_t RIFF_CHUNK_ID = MAKE_RIFF_CODE("RIFF");
		const uint32_t RIFF_WAVE_FORMAT = MAKE_RIFF_CODE("WAVE");
		const uint32_t WAVE_FORMAT_CHUNK_ID = MAKE_RIFF_CODE("fmt ");
		const uint32_t WAVE_DATA_CHUNK_ID = MAKE_RIFF_CODE("data");

		// riff header
		struct wave_chunk_descriptor
		{
			uint32_t chunk_id; // should be 'RIFF'
			uint32_t chunk_size;
			uint32_t format; // should be 'WAVE'

			uint32_t advance_size() const
			{
				return 8 + 4;
			}
		};

		struct wave_format_chunk
		{
			uint32_t chunk_id; // should be 'fmt '
			uint32_t chunk_size; // should be 16, 18, or 40.
			uint16_t format_code; // format tag
			uint16_t total_channels; // number of interleaved channels
			uint32_t sample_rate; // blocks per second
			uint32_t data_rate; // avg bytes per sec
			uint16_t block_align; // data block size (bytes)
			uint16_t bits_per_sample;
			uint16_t extended_size; // size of extension (0 or 22)
			uint16_t valid_bits_per_sample; // number of valid bits
			uint32_t channel_mask; // speaker position mask
			uint8_t subformat[16]; // GUID including the data format code

			uint32_t advance_size() const
			{
				return 8 + chunk_size;
			}
		};

		struct wave_data_chunk
		{
			uint32_t chunk_id; // should be 'data'
			uint32_t chunk_size; // == num_samples * num_channels * bits_per_sample / 8

			uint32_t advance_size() const
			{
				return 8;
			}
		};

		// TODO: This could fail; add error checking/codes.
		int load_wave(Array<audio::InMemorySampleType>& samples, size_t& channels, const char* path)
		{
			Array<unsigned char> filecontents;
			core::filesystem::instance()->virtual_load_file(filecontents, path);

			unsigned char* wavedata = static_cast<unsigned char*>(&filecontents[0]);
			wave_chunk_descriptor* desc = reinterpret_cast<wave_chunk_descriptor*>(wavedata);
			if (desc->chunk_id != RIFF_CHUNK_ID)
			{
				LOGV("[WAVE] Is not a valid RIFF file\n");
				return -1;
			}

			if (desc->format != RIFF_WAVE_FORMAT)
			{
				LOGV("[WAVE] Is not a valid WAVE file format\n");
				return -2;
			}

			wave_format_chunk* format = reinterpret_cast<wave_format_chunk*>(
				reinterpret_cast<char*>(desc) + desc->advance_size()
			);
			if (format->chunk_id != WAVE_FORMAT_CHUNK_ID)
			{
				LOGV("[WAVE] Expected WAVE 'fmt ' chunk!\n");
				return -3;
			}

			// We only support PCM.
			assert(format->format_code == WAVE_FORMAT_PCM);

			// Only support our sample rate of 44.1KHz.
			assert(format->sample_rate == audio::AUDIO_FREQUENCY_HZ);

			// TODO: Support mono and stereo sounds.
			// Currently, only stereo WAVE files are supported.
			assert(format->total_channels == 2);

			channels = format->total_channels;

			wave_data_chunk* data = reinterpret_cast<wave_data_chunk*>(
				reinterpret_cast<char*>(format) + format->advance_size()
			);

			if (data->chunk_id != WAVE_DATA_CHUNK_ID)
			{
				LOGV("[WAVE] Expected WAVE 'data' chunk!\n");
				return -4;
			}

			// length of the file in seconds: data->chunk_size / format->data_rate
			const bool has_pad_byte = ((data->chunk_size + 1) & ~1) == 0;

			// TODO: support padding byte
			assert(has_pad_byte == false);

			const uint32_t total_samples = (data->chunk_size / format->block_align);

			samples.resize(total_samples * format->total_channels);
			int16_t* sample_data = reinterpret_cast<int16_t*>(reinterpret_cast<char*>(data) + data->advance_size());
			for (size_t sample = 0; sample < total_samples * 2; ++sample)
			{
				samples[sample] = glm::clamp(sample_data[sample] / audio::InMemorySampleValueMax, -1.0f, 1.0f);
			}

			LOGV("[WAVE] loaded '%s'\n", path);
			LOGV("[WAVE] file size: %i bytes\n", filecontents.size());
			LOGV("[WAVE] channels: %i, frequency: %i Hz, samples: %i\n", format->total_channels, format->sample_rate, total_samples);

			return 0;
		} // load_wave


		// -------------------------------------------------------------
		size_t get_frame_wave(Sound* sound, size_t frame, float* buffer)
		{
			const size_t frame_index = (frame * sound->channels);
			buffer[0] = sound->pcmdata[frame_index + 0];
			buffer[1] = sound->pcmdata[frame_index + 1];
			return frame;
		}

		// -------------------------------------------------------------
		// Sound

		Sound::Sound()
			: channels(0)
		{
		}

		Sound::~Sound()
		{
		}

		void Sound::release()
		{
			// cleanup
		}

		size_t Sound::get_frame(size_t frame, float* buffer)
		{
			return get_frame_callback(this, frame, buffer);
		}

		AssetLoadStatus sound_load_callback(const char* path, Sound* sound, const AssetParameters& parameters)
		{
			// load the file into a memory buffer
			if (load_wave(sound->pcmdata, sound->channels, path) != 0)
			{
				LOGW("An error occurred while loading '%s'\n", path);
				assert(0);
				return AssetLoad_Failure;
			}

			// TODO: determine which callback it should use here based on
			// file extension.
			sound->get_frame_callback = &get_frame_wave;

			return AssetLoad_Success;
		} // font_load_callback

		void sound_construct_extension(core::StackString<MAX_PATH_SIZE>& extension)
		{
			//kernel::KernelDeviceFlags device_flags = kernel::parameters().device_flags;
			//if ((device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone))
			//{
			//	extension = ".caf";
			//}

			// TODO: packaged extension should always be compressed.
			//extension = ".ogg";

			extension = ".wav";
		} // sound_construct_extension
	} // namespace assets
} // namespace gemini
