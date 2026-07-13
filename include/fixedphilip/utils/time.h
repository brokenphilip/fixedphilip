#pragma once

#include <chrono>
#include <string>

namespace fixedphilip::utils::time
{
	class stopwatch
	{
	public:
		using clock = std::chrono::steady_clock;
	private:
		std::chrono::time_point<clock> start_;
		std::chrono::time_point<clock> end_;
	public:
		// Starts, or resumes, measuring elapsed time. Starting a Stopwatch that is already running does nothing.
		inline void start()
		{
			if (!running())
			{
				auto elapsed = end_ - start_;
				start_ = clock::now() - elapsed;
				end_ = {};
			}
		}

		// Stops measuring elapsed time. Elapsed time does not get reset. Stopping a Stopwatch that is already stopped does nothing.
		inline void stop()
		{
			if (running())
			{
				end_ = clock::now();
			}
		}

		// Gets whether the Stopwatch is currently measuring elapsed time or not.
		inline bool running() const
		{
			return start_.time_since_epoch().count() != 0 && end_.time_since_epoch().count() == 0;
		}

		// Stops measuring elapsed time (if running) and resets the elapsed time to zero.
		inline void reset()
		{
			start_ = {};
			end_ = {};
		}

		// Gets the elapsed time (in microseconds by default - 1000us = 1ms).
		template <typename Duration = std::chrono::microseconds>
		inline Duration elapsed() const
		{
			if (running())
			{
				return std::chrono::duration_cast<Duration>(clock::now() - start_);
			}
			return std::chrono::duration_cast<Duration>(end_ - start_);
		}
	};

	class raii_stopwatch : public stopwatch
	{
		using stopwatch::start;
		using stopwatch::stop;
		using stopwatch::reset;
	public:
		inline raii_stopwatch() { start(); }
		inline ~raii_stopwatch() { stop(); }
		inline bool running() { return true; }
		using stopwatch::elapsed;
	};

	// How long this program has been running for
	const inline raii_stopwatch program_uptime;

	// Time point when the program started
	const inline auto program_start = std::chrono::system_clock::now();

	inline std::string format_uptime()
	{
		std::string uptime;
		auto program_uptime_elapsed = program_uptime.elapsed<std::chrono::seconds>();
		if (program_uptime_elapsed > std::chrono::days(1))
		{
			auto days = std::chrono::duration_cast<std::chrono::days>(program_uptime_elapsed);
			program_uptime_elapsed -= days;
			uptime = std::format("{} {:%Hh %Mm}", days, program_uptime_elapsed);
		}
		else if (program_uptime_elapsed > std::chrono::hours(1))
		{
			uptime = std::format("{:%Hh %Mm %Ss}", program_uptime_elapsed);
		}
		else if (program_uptime_elapsed > std::chrono::minutes(1))
		{
			uptime = std::format("{:%Mm %Ss}", program_uptime_elapsed);
		}
		else
		{
			uptime = std::format("{:%Ss}", program_uptime_elapsed);
		}
		return uptime;
	}
}