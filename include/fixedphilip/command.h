#pragma once

#include <cstring> // strcmp

#include <fixedphilip/node.h>

#include <dpp/dispatcher.h>

namespace fixedphilip
{
	class command : public node<command>
	{
	public:
		using run_function = void(const dpp::slashcommand_t& event);
	private:
		const char* name_;
		const char* description_;

		run_function* run_;
	public:
		command(const char* name, const char* description, run_function* run) : name_(name), description_(description), run_(run) {}
		~command() {}

		inline auto name() { return name_; }
		inline auto description() { return description_; }

		inline auto run(const dpp::slashcommand_t& event) { run_(event); }

		inline virtual bool compare_nodes(command* current, command* next) override final
		{
			return strcmp(current->name_, next->name_) < 0;
		}
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, &name::run)