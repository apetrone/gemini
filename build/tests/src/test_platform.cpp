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

#include <platform/platform.h>

// ---------------------------------------------------------------------
// dynamic library
// ---------------------------------------------------------------------
void test_dynamic_library()
{
	TEST_CATEGORY(dynamic_library);

	const char* library_extension = platform::dylib_extension();
	TEST_VERIFY(library_extension, dylib_extension);
}

// ---------------------------------------------------------------------
// filesystem
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// serial
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// thread
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// time
// ---------------------------------------------------------------------

int main(int, char**)
{
	TEST_CATEGORY(platform);
	
	platform::memory::startup();
	platform::Result result = platform::startup();
	TEST_VERIFY(result.success(), platform_startup);

	test_dynamic_library();
	
	platform::shutdown();
	platform::memory::shutdown();
	return 0;
}