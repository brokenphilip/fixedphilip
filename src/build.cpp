#include <fixedphilip/build.h>
#include <fixedphilip/git.h>

namespace fixedphilip::build
{
	const char* architecture()
	{
		switch (sizeof(void*))
		{
		case 4: return "32-bit";
		case 8: return "64-bit";
		default: return "?-bit";
		}
	}

	const char* configuration()
	{
#ifdef NDEBUG
		return "Release";
#else
		return "Debug";
#endif
	}

	const char* date_time()
	{
		return __DATE__ " " __TIME__;
	}

	const char* platform()
	{
#ifdef _WIN32
		return "Windows";
#endif
#ifdef __linux__
		return "Linux";
#endif
	}

	int version()
	{
		return FIXEDPHILIP_GIT_REVISION_COUNT + 1;
	}
}
