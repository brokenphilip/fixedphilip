#include <fixedphilip/log.h>

#include <iostream> // std::cout
#include <syncstream> // std::osyncstream

#include <dpp/utility.h> // dpp::utility::*

namespace fixedphilip::log
{
	void implementation(dpp::loglevel ll, const std::string& prefix, const std::string& message)
	{
		if (ll == dpp::ll_trace)
		{
			return;
		}

		std::osyncstream sync_cout(std::cout);
		sync_cout << "[" << dpp::utility::current_date_time() << "] ";
		if (!prefix.empty())
		{
			sync_cout << "(" << prefix << ") ";
		}
		sync_cout << dpp::utility::loglevel(ll) << ": " << message << std::endl;
	}

	void trace(const std::string& message)
	{
		implementation(dpp::loglevel::ll_trace, "", message);
	}

	void debug(const std::string& message)
	{
		implementation(dpp::loglevel::ll_debug, "", message);
	}

	void info(const std::string& message)
	{
		implementation(dpp::loglevel::ll_info, "", message);
	}

	void warning(const std::string& message)
	{
		implementation(dpp::loglevel::ll_warning, "", message);
	}

	void error(const std::string& message)
	{
		implementation(dpp::loglevel::ll_error, "", message);
	}

	void critical(const std::string& message)
	{
		implementation(dpp::loglevel::ll_critical, "", message);
	}
}