#include <chrono>

namespace fixedphilip
{
	namespace utils
	{
		class stopwatch
		{
			using clock = std::chrono::steady_clock;

			std::chrono::time_point<clock> _start;
			std::chrono::time_point<clock> _end;
		public:
			// Starts, or resumes, measuring elapsed time. Starting a Stopwatch that is already running does nothing.
			inline void start()
			{
				if (!running())
				{
					auto elapsed = _end - _start;
					_start = clock::now() - elapsed;
					_end = {};
				}
			}

			// Stops measuring elapsed time. Elapsed time does not get reset. Stopping a Stopwatch that is already stopped does nothing.
			inline void stop()
			{
				if (running())
				{
					_end = clock::now();
				}
			}

			// Gets whether the Stopwatch is currently measuring elapsed time or not.
			inline bool running()
			{
				return _start.time_since_epoch().count() != 0 && _end.time_since_epoch().count() == 0;
			}

			// Stops measuring elapsed time (if running) and resets the elapsed time to zero.
			inline void reset()
			{
				_start = {};
				_end = {};
			}

			// Gets the elapsed time (in microseconds by default - 1000us = 1ms).
			template <typename Duration = std::chrono::microseconds>
			inline auto elapsed()
			{
				if (running())
				{
					return std::chrono::duration_cast<Duration>(clock::now() - _start).count();
				}
				return std::chrono::duration_cast<Duration>(_end - _start).count();
			}
		};

		// Global stopwatch measuring application uptime
		inline stopwatch app_uptime;
	}
}