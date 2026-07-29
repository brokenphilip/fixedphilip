#pragma once

#define FIXEDPHILIP_BUILD_ARCHITECTURE_NUM (sizeof(void*) * 8)

// Active project build configuration
#ifdef NDEBUG
#define FIXEDPHILIP_BUILD_CONFIGURATION "Release"
#else
#define FIXEDPHILIP_BUILD_CONFIGURATION "Debug"
#endif

// Active project operating system/platform
#ifdef _WIN32
#define FIXEDPHILIP_BUILD_PLATFORM "Windows"
#endif
#ifdef __linux__
#define FIXEDPHILIP_BUILD_PLATFORM "Linux"
#endif
#ifndef FIXEDPHILIP_BUILD_PLATFORM
#define FIXEDPHILIP_BUILD_PLATFORM "(unknown platform)"
#endif

// Current build version, calculated from the git revision count
#include <fixedphilip/git.h>
#define FIXEDPHILIP_BUILD_VERSION_NUM (FIXEDPHILIP_GIT_REVISION_COUNT + 1)

namespace fixedphilip::build
{
	// Current build date/time (calculated in build.cpp, which is rebuilt every time)
	const char* date_time();
}