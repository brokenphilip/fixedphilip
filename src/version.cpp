#include "git.h"
#include "version.h"

const char* fixedphilip::build_date_time()
{
	return __DATE__ " " __TIME__;
}

int fixedphilip::build_version()
{
	return FIXEDPHILIP_GIT_REVISION_COUNT + 1;
}