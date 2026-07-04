#pragma once

#include <string>

namespace fixedphilip::log
{
	using log_function = void(const std::string& message);

	log_function info;
	log_function warning;
	log_function error;
}