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
#include <core/stackstring.h>
#include <platform/platform.h>
#include <runtime/filesystem.h>
#include <core/mathlib.h> // for glm

#include <core/color.h>
#include <renderer/image.h>
#include <renderer/renderer.h>

#include "assets.h"
#include "assets/asset_texture.h"

#include "kernel.h" // for device_flags

using namespace renderer;

namespace gemini
{
	namespace assets
	{
		Texture::Texture() : texture(0) {}
		Texture::~Texture() {}

		void Texture::release()
		{
			driver()->texture_destroy(this->texture);
		} // release


		AssetLoadStatus texture_load_callback(const char* path, Texture* texture, const TextureParameters& parameters)
		{
			unsigned int width = 0;
			unsigned int height = 0;
			bool load_result = 0;

			if ( !(parameters.flags & image::F_CUBEMAP) ) // load 2d texture
			{
				texture->texture = load_texture_from_file(path, parameters, texture->image);
				load_result = texture->texture != 0;
			}
			else // load cubemap
			{
				core::StackString< MAX_PATH_SIZE > fullpath[6];
				const char ext[][4] = { "_rt", "_lt", "_up", "_dn", "_ft", "_bk" };
				const char * names[6];
				for( int i = 0; i < 6; ++i )
				{
					fullpath[i] = path;
					fullpath[i].remove_extension();
					fullpath[i].append( ext[i] );
					assets::textures()->append_extension( fullpath[i] );
					names[i] = fullpath[i]();
				}

				//			load_result = renderlib::LoadCubemap( names, texture_id, flags, &width, &height );
				assert( 0 );
			}

			if (load_result)
			{
				texture->image.flags = parameters.flags;
				texture->image.width = width;
				texture->image.height = height;
				return assets::AssetLoad_Success;
			}

			return assets::AssetLoad_Failure;
		} // texture_load_callback


		void texture_construct_extension( core::StackString<MAX_PATH_SIZE> & extension )
		{
			kernel::KernelDeviceFlags device_flags = kernel::parameters().device_flags;
			const char * ext = 0;

			if ( device_flags & kernel::DeviceDesktop || (device_flags & kernel::DeviceAndroid) )
			{
				// ...
				ext = "png";
			}
			else if ( device_flags & kernel::DeviceiPhone )
			{
				ext = "png";
				extension.append( "-iphone" );
			}
			else if ( device_flags & kernel::DeviceiPad )
			{
				ext = "png";
				extension.append( "-ipad" );
			}

			if ( device_flags & kernel::DeviceSupportsRetinaDisplay )
			{
				extension.append( "@2x" );
			}

			extension.append(".");
			extension.append(ext);
		} // texture_construct_extension

		::renderer::Texture* load_texture_from_file(const char* filename, const assets::TextureParameters& parameters, image::Image& image)
		{
			Array<unsigned char> buffer;
			::renderer::Texture* render_texture = nullptr;
			core::filesystem::instance()->virtual_load_file(buffer, filename);

			if (!buffer.empty())
			{
				unsigned char* pixels = image::load_image_from_memory(&buffer[0], buffer.size(), &image.width, &image.height, &image.channels);
				if (pixels)
				{
	//				may need to actually flip the image vertically here
	//				flip_image_vertically( width, height, components, pixels );

					image.pixels = pixels;
					image.filter = parameters.filter_type;

					render_texture = ::renderer::driver()->texture_create(image);

					LOGV("Loaded texture \"%s\"; (%i x %i @ %ibpp)\n", filename, image.width, image.height, image.channels);

					image::free_image(pixels);
					image.pixels = 0;
				}
				else
				{
					LOGE("Unable to load image %s\n", filename);
				}

				return render_texture;
			}
			else
			{
				LOGE("Couldn't load file: %s\n", filename);
				return nullptr;
			}

			return nullptr;
		} // load_texture_from_file

	} // namespace assets
} // namespace gemini
