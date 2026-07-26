#pragma once

#include <dpp/misc-enum.h> // dpp::loglevel

#include <string>

namespace fixedphilip::log
{
	void implementation(dpp::loglevel ll, const std::string& prefix, const std::string& message);

	using log_function = void(const std::string& message);
	log_function trace;
	log_function debug;
	log_function info;
	log_function warning;
	log_function error;
	log_function critical;
}