#pragma once

#include <dpp/dispatcher.h>

namespace fixedphilip
{
	class command
	{
	public:
		using run_function = void(const dpp::slashcommand_t& event);
	private:
		static inline command* first_ = nullptr;
		static inline command* last_ = nullptr;

		command* prev_ = nullptr;
		command* next_ = nullptr;

		const char* name_;
		const char* description_;

		run_function* run_;
	public:
		command(const char* name, const char* description, run_function* run);
		~command();

		inline auto previous() { return prev_; }
		inline auto next() { return next_; }

		inline auto name() { return name_; }
		inline auto description() { return description_; }

		inline auto run(const dpp::slashcommand_t& event) { run_(event); }

		static inline auto first() { return first_; }
		static inline auto last() { return last_; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, &name::run)