#pragma once

#include <string>

namespace fixedphilip
{
	namespace log
	{
		using log_function = void(std::string message);

		log_function info;
		log_function warning;
		log_function error;
	}
}