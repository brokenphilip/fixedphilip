#include <fixedphilip/version.h>
#include <fixedphilip/git.h>

const char* fixedphilip::build_architecture()
{
	switch (sizeof(void*))
	{
		case 4: return "32-bit";
		case 8: return "64-bit";
		default: return "?-bit";
	}
}

const char* fixedphilip::build_date_time()
{
	return __DATE__ " " __TIME__;
}

int fixedphilip::build_version()
{
	return FIXEDPHILIP_GIT_REVISION_COUNT + 1;
}