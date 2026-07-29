#pragma once

#include <dpp/misc-enum.h> // dpp::loglevel

#include <string>

namespace fixedphilip::log
{
	// Logging implementation used for internal logging functions, as well as "public" logging functions listed below
	void implementation(dpp::loglevel ll, const std::string& prefix, const std::string& message);

	// Logging functions for several different severities/"log levels"
	using log_function = void(const std::string& message);
	log_function trace;
	log_function debug;
	log_function info;
	log_function warning;
	log_function error;
	log_function critical;
}