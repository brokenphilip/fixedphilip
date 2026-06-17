#pragma once

#include <dpp/dispatcher.h>

namespace fixedphilip
{
	class command
	{
	public:
		using run_function = void(const dpp::slashcommand_t& event);
	private:
		static inline command* _first = nullptr;
		static inline command* _last = nullptr;

		command* _prev = nullptr;
		command* _next = nullptr;

		const char* _name;
		const char* _description;

		run_function* _run;
	public:
		command(const char* name, const char* description, run_function* run);
		~command();

		inline auto previous() { return _prev; }
		inline auto next() { return _next; }

		inline auto name() { return _name; }
		inline auto description() { return _description; }

		inline auto run(const dpp::slashcommand_t& event) { _run(event); }

		static inline auto first() { return _first; }
		static inline auto last() { return _last; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, &name::run)