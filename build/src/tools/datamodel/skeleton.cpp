// -------------------------------------------------------------
// Copyright (C) 2014- Adam Petrone
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

#include "datamodel/skeleton.h"

#include <assert.h>

namespace gemini
{
	namespace datamodel
	{
		Skeleton::Skeleton()
		{
		}

		Skeleton::~Skeleton()
		{
			for (Bone* bone : bones)
			{
				MEMORY_DELETE(bone, core::memory::global_allocator());
			}
		}

		Bone* Skeleton::add_bone(int32_t parent_index, const String& name)
		{
			Bone* bone = MEMORY_NEW(Bone, core::memory::global_allocator());
			bone->index = static_cast<int32_t>(bones.size());
			bone->parent = parent_index;
			bone->name = name;
			bones.push_back(bone);
			return bone;
		} // add_bone

		Bone* Skeleton::find_bone_named(const String& name)
		{
			for(Bone* bone : bones)
			{
				if (bone->name == name)
				{
					return bone;
				}
			}
			return 0;
		} // find_bone_named

		Bone* Skeleton::get_bone_at_index(uint32_t index)
		{
			assert(index < bones.size());
			return bones[index];
		} // get_bone_at_index

	} // namespace datamodel
} // namespace gemini
