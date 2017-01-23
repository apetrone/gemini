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
//		 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// -------------------------------------------------------------

#include <runtime/audio_library.h>
#include <runtime/audio_mixer.h>
#include <runtime/filesystem.h>

#include <core/logging.h>
#include <core/mathlib.h>
#include <core/mem.h>


namespace gemini
{
	// -------------------------------------------------------------
	// WAVE Format
	// -------------------------------------------------------------
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

	void load_mono_wave(Array<float>& samples, size_t total_samples, int16_t* sample_data)
	{
		for (size_t sample = 0; sample < total_samples; ++sample)
		{
			size_t sample_index = (sample * 2);
			samples[sample_index + 0] = glm::clamp(sample_data[sample] / audio::InMemorySampleValueMax, -1.0f, 1.0f);
			samples[sample_index + 1] = samples[sample_index + 0];
		}
	}

	void load_stereo_wave(Array<float>& samples, size_t total_samples, int16_t* sample_data)
	{
		for (size_t sample = 0; sample < total_samples; ++sample)
		{
			size_t sample_index = (sample * 2);
			samples[sample_index + 0] = glm::clamp(sample_data[sample_index + 0] / audio::InMemorySampleValueMax, -1.0f, 1.0f);
			samples[sample_index + 1] = glm::clamp(sample_data[sample_index + 1] / audio::InMemorySampleValueMax, -1.0f, 1.0f);
		}
	}

	// TODO: This could fail; add error checking/codes.
	int load_wave(gemini::Allocator& allocator, Array<float>& samples, size_t& channels, const char* path)
	{
		Array<unsigned char> filecontents(allocator);
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

		// Support mono and stereo sounds only.
		// We just duplicate mono sounds across to two channels.
		assert(format->total_channels == 1 || format->total_channels == 2);

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

		channels = audio::InMemoryChannelCount;
		samples.resize(total_samples * audio::InMemoryChannelCount);
		int16_t* sample_data = reinterpret_cast<int16_t*>(reinterpret_cast<char*>(data) + data->advance_size());
		if (format->total_channels == 1)
		{
			load_mono_wave(samples, total_samples, sample_data);
		}
		else if (format->total_channels == 2)
		{
			load_stereo_wave(samples, total_samples, sample_data);
		}

		LOGV("[WAVE] loaded '%s'\n", path);
		LOGV("[WAVE] file size: %i bytes\n", filecontents.size());
		LOGV("[WAVE] channels: %i, frequency: %i Hz, samples: %i\n", format->total_channels, format->sample_rate, total_samples);

		return 0;
	} // load_wave


#if 0
	void test_write_wav(Array<int16_t>& samples, const char* path)
	{
		FILE* f = fopen(path, "wb");

		wav::wave_chunk_descriptor desc;
		desc.chunk_id = MAKE_RIFF_CODE("RIFF");
		desc.format = MAKE_RIFF_CODE("WAVE");
		// TODO: fill out desc with the total file size - 8

		wav::wave_format_chunk format;
		format.chunk_id = MAKE_RIFF_CODE("fmt ");
		format.chunk_size = 16;
		format.format_code = wav::WAVE_FORMAT_PCM;
		format.total_channels = 2;
		format.sample_rate = 44100;
		format.bits_per_sample = 16;
		format.data_rate = format.sample_rate * format.total_channels * (format.bits_per_sample / 8);
		format.block_align = static_cast<uint16_t>((format.total_channels * format.bits_per_sample) / 8);

		wav::wave_data_chunk data;
		data.chunk_id = MAKE_RIFF_CODE("data");
		data.chunk_size = static_cast<uint32_t>((samples.size() * format.bits_per_sample) / 8);
		desc.chunk_size = data.chunk_size + 36;

		if (f)
		{
			fwrite(&desc, 1, desc.advance_size(), f);
			fwrite(&format, 1, format.advance_size(), f);
			fwrite(&data, 1, data.advance_size(), f);
			fwrite(&samples[0], 2, samples.size(), f);

			// see if we need to write a padding byte.
			if (((data.chunk_size + 1) & ~1) == 0)
			{
				char padding = '\0';
				fwrite(&padding, 1, 1, f);
			}
			fclose(f);
		}
	}
#endif

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
	// -------------------------------------------------------------

	Sound::Sound(Allocator& allocator)
		: channels(0)
		, pcmdata(allocator)
	{
	}

	Sound::~Sound()
	{
	}

	size_t Sound::get_frame(size_t frame, float* buffer)
	{
		return get_frame_callback(this, frame, buffer);
	}

	size_t Sound::get_total_frames() const
	{
		return pcmdata.size() / audio::InMemoryChannelCount;
	}


	// -------------------------------------------------------------
	// AudioLibrary
	// -------------------------------------------------------------
	AudioLibrary::AudioLibrary(Allocator& allocator)
		: AssetLibrary2(allocator)
	{
	}

	void AudioLibrary::create_asset(LoadState& state, void* parameters)
	{
		state.asset = MEMORY2_NEW(*state.allocator, Sound)(*state.allocator);
	}

	AssetLoadStatus AudioLibrary::load_asset(LoadState& state, platform::PathString& fullpath, void* parameters)
	{
		LOGV("loading audio \"%s\"\n", fullpath());

		platform::PathString asset_uri = fullpath;
		//kernel::KernelDeviceFlags device_flags = kernel::parameters().device_flags;
		//if ((device_flags & kernel::DeviceiPad) || (device_flags & kernel::DeviceiPhone))
		//{
		//	extension = ".caf";
		//}

		// TODO: packaged extension should always be compressed.
		//extension = ".ogg";
		asset_uri.append(".wav");

		// load the file into a memory buffer
		if (load_wave(*state.allocator, state.asset->pcmdata, state.asset->channels, asset_uri()) != 0)
		{
			LOGE("An error occurred while loading '%s'\n", asset_uri());
			return AssetLoad_Failure;
		}

		// TODO: determine which callback it should use here based on
		// file extension.
		state.asset->get_frame_callback = &get_frame_wave;

		return AssetLoad_Success;
	}

	void AudioLibrary::destroy_asset(LoadState& state)
	{
		MEMORY2_DELETE(*state.allocator, state.asset);
	}
} // namespace gemini
