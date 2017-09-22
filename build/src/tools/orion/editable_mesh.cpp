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
#include "editable_mesh.h"

//#include <core/datastream.h>
#include <core/logging.h>
//#include <core/mathlib.h>
//#include <runtime/filesystem.h>

using namespace gemini;

EditableMesh::EditableMesh(Allocator& allocator)
	: attachments(allocator)
{
}

void EditableMesh::load_from(Allocator& allocator, Mesh* mesh)
{
	const size_t total_attachments = mesh->attachments.size();

	//for (size_t index = 0; index < total_attachments; ++index)
	//{
	//	const ModelAttachment* attachment = mesh->attachments[index];
	//	ModelAttachment* attachment_instance = MEMORY2_NEW(allocator, ModelAttachment);
	//	attachment_instance->bone_index = attachment->bone_index;
	//	attachment_instance->local_orientation_offset = attachment->local_orientation_offset;
	//	attachment_instance->local_translation_offset = attachment->local_translation_offset;
	//	attachment_instance->name = attachment->name;
	//	attachments.push_back(attachment_instance);
	//}

}

void EditableMesh::save_to(Mesh* mesh)
{
}

void EditableMesh::purge(gemini::Allocator& allocator)
{
	for (size_t index = 0; index < attachments.size(); ++index)
	{
		ModelAttachment* attachment = attachments[index];
		MEMORY2_DELETE(allocator, attachment);
	}

	attachments.clear();
}

Array<gemini::ModelAttachment*> EditableMesh::get_attachments()
{
	return attachments;
}