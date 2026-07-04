#include <fixedphilip/log.h>

#include <iostream> // std::cout

#include <dpp/misc-enum.h> // dpp::loglevel
#include <dpp/utility.h>

namespace fixedphilip::log
{
	void dpp_cout_log(dpp::loglevel severity, const std::string& message)
	{
		if (severity > dpp::ll_trace)
		{
			std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(severity) << ": " << message << "\n";
		}
	}

	void info(const std::string& message)
	{
		dpp_cout_log(dpp::loglevel::ll_info, message);
	}

	void warning(const std::string& message)
	{
		dpp_cout_log(dpp::loglevel::ll_warning, message);
	}

	void error(const std::string& message)
	{
		dpp_cout_log(dpp::loglevel::ll_error, message);
	}
}