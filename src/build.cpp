#include <fixedphilip/build.h>

namespace fixedphilip::build
{
	const char* date_time()
	{
		return __DATE__ " " __TIME__;
	}
}