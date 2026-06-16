#include "log.h"

#include <iostream>

#include <dpp/misc-enum.h>
#include <dpp/utility.h>

namespace fixedphilip
{
	namespace log
	{
		void dpp_cout_log(dpp::loglevel severity, std::string message)
		{
			if (severity > dpp::ll_trace)
			{
				std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(severity) << ": " << message << "\n";
			}
		}

		void info(std::string message)
		{
			dpp_cout_log(dpp::loglevel::ll_info, message);
		}

		void warning(std::string message)
		{
			dpp_cout_log(dpp::loglevel::ll_warning, message);
		}

		void error(std::string message)
		{
			dpp_cout_log(dpp::loglevel::ll_error, message);
		}
	}
}