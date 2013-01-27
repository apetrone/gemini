#pragma once

#include "platform.hpp"

namespace platform
{
	core::Error osx_startup();
	void osx_shutdown();
	
	core::Error osx_programDirectory( char * path, size_t size );
};