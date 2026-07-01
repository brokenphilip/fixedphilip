#include <chrono>

namespace fixedphilip
{
	namespace utils
	{
		class stopwatch
		{
			using clock = std::chrono::steady_clock;

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
			inline bool running()
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
			inline auto elapsed()
			{
				if (running())
				{
					return std::chrono::duration_cast<Duration>(clock::now() - start_).count();
				}
				return std::chrono::duration_cast<Duration>(end_ - start_).count();
			}
		};

		// Global stopwatch measuring application uptime
		inline stopwatch app_uptime;
	}
}