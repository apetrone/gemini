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
#include "project.h"

#include <core/datastream.h>
#include <core/logging.h>
#include <core/mathlib.h>
#include <runtime/filesystem.h>

Project::Project()
	: modified_flag(0)
{

}

Project::~Project()
{
}

const String& Project::get_name() const
{
	return name;
}

void Project::set_name(const String& new_name)
{
	name = new_name;
}

platform::Result Project::save_project()
{
	platform::PathString absolute_path = root_path.c_str();
	absolute_path.append(PATH_SEPARATOR_STRING);
	absolute_path.append("project.conf");

	save_project_as(absolute_path());

	return platform::Result::success();
}

platform::Result Project::save_project_as(const String& absolute_path)
{
	core::util::ResizableMemoryStream stream;

	uint32_t name_length = name.length();
	stream.write(&name_length, sizeof(uint32_t));
	stream.write(&name[0], name_length);

	stream.write(&camera_position, sizeof(glm::vec3));
	stream.write(&camera_yaw, sizeof(float));
	stream.write(&camera_pitch, sizeof(float));

	platform::File handle = platform::fs_open(absolute_path.c_str(), platform::FileMode_Write);
	if (!handle.is_open())
	{
		LOGW("Unable to open file: %s\n", absolute_path.c_str());
		return platform::Result::failure("Couldn't open file");
	}

	platform::fs_write(handle, stream.get_data(), 1, stream.get_data_size());
	platform::fs_close(handle);

	return platform::Result::success();
}

Project* Project::open_project(const String& absolute_path)
{
	Project* project = new Project();

	platform::File handle = platform::fs_open(absolute_path.c_str(), platform::FileMode_Read);
	if (!handle.is_open())
	{
		LOGW("Unable to open file: %s\n", absolute_path.c_str());
		return nullptr;
	}

	platform::fs_seek(handle, 0, platform::FileSeek_End);
	size_t file_size = static_cast<size_t>(platform::fs_tell(handle));
	platform::fs_seek(handle, 0, platform::FileSeek_Begin);

	gemini::Allocator allocator = gemini::memory_allocator_default(gemini::MEMORY_ZONE_DEFAULT);

	char* buffer = (char*)MEMORY2_ALLOC(allocator, file_size);

	platform::fs_read(handle, buffer, file_size, 1);
	platform::fs_close(handle);

	core::util::MemoryStream stream;
	stream.init(buffer, file_size);

	uint32_t name_length;
	stream.read(name_length);
	project->name.resize(name_length);
	stream.read(&project->name[0], name_length);
	stream.read(&project->camera_position, sizeof(glm::vec3));
	stream.read(project->camera_yaw);
	stream.read(project->camera_pitch);

	MEMORY2_DEALLOC(allocator, buffer);

	platform::PathString root_path = absolute_path.c_str();
	project->root_path = root_path.dirname()();

	return project;
}