#pragma once

#include <chrono>
#include <string>

namespace fixedphilip::utils::time
{
	// Stopwatch used for measuring elapsed time
	class stopwatch
	{
	public:
		using clock = std::chrono::steady_clock;
	private:
		std::chrono::time_point<clock> start_;
		std::chrono::time_point<clock> end_;
	public:
		// Starts, or resumes, measuring elapsed time
		// Starting a stopwatch that is already running does nothing
		inline void start()
		{
			if (!running())
			{
				auto elapsed = end_ - start_;
				start_ = clock::now() - elapsed;
				end_ = {};
			}
		}

		// Stops measuring elapsed time. Elapsed time does not get reset
		// Stopping a stopwatch that is already stopped does nothing
		inline void stop()
		{
			if (running())
			{
				end_ = clock::now();
			}
		}

		// Gets whether the stopwatch is currently measuring elapsed time or not
		inline bool running() const
		{
			return start_.time_since_epoch().count() != 0 && end_.time_since_epoch().count() == 0;
		}

		// Stops measuring elapsed time (if running) and resets the elapsed time to zero
		// If you're looking to restart the stopwatch, call reset() and then start()
		inline void reset()
		{
			start_ = {};
			end_ = {};
		}

		// Gets the elapsed time (in microseconds by default: 1000us = 1ms)
		// You can get the raw tick count by calling Duration.count()
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

	// RAII wrapper class for the stopwatch
	// Starts when created, stops when destroyed, is always running during its lifetime
	class raii_stopwatch : public stopwatch
	{
		using stopwatch::start;
		using stopwatch::stop;
		using stopwatch::reset;
	public:
		inline raii_stopwatch() { start(); }
		inline ~raii_stopwatch() { stop(); }
		inline bool running() const { return true; }
	};

	// Use this function to run code within an if() statement only when enough time has passed
	// T can be any unique 'tag' identifier name, eg. "struct any_unique_name_you_like_here"
	// Returns true once "duration" has passed since the previous successful function call
	// ...ie. since the last time this particular "run_if_passed" returned true
	// NOTE: This function also returns true on the very first function call
	template <typename T, typename Duration = std::chrono::microseconds>
	inline bool run_if_passed(Duration duration)
	{
		static stopwatch last_call;
		if (last_call.elapsed<Duration>() > duration || !last_call.running())
		{
			last_call.reset();
			last_call.start();
			return true;
		}
		return false;
	}
}