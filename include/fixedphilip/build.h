#pragma once

#define FIXEDPHILIP_BUILD_ARCHITECTURE_NUM (sizeof(void*) * 8)

#ifdef NDEBUG
#define FIXEDPHILIP_BUILD_CONFIGURATION "Release"
#else
#define FIXEDPHILIP_BUILD_CONFIGURATION "Debug"
#endif

#ifdef _WIN32
#define FIXEDPHILIP_BUILD_PLATFORM "Windows"
#endif
#ifdef __linux__
#define FIXEDPHILIP_BUILD_PLATFORM "Linux"
#endif
#ifndef FIXEDPHILIP_BUILD_PLATFORM
#define FIXEDPHILIP_BUILD_PLATFORM "(unknown platform)"
#endif

#include <fixedphilip/git.h>
#define FIXEDPHILIP_BUILD_VERSION_NUM (FIXEDPHILIP_GIT_REVISION_COUNT + 1)

namespace fixedphilip::build
{
	const char* date_time();
}