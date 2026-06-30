#pragma once

#include <cstring> // strcmp

#include <fixedphilip/node.h>

#include <dpp/appcommand.h> // dpp::slashcommand
#include <dpp/dispatcher.h> // dpp::slashcommand_t

namespace fixedphilip
{
	class command : public node<command>
	{
	public:
		using init_function = void(dpp::slashcommand& command);
		using run_function = void(const dpp::slashcommand_t& event);
	private:
		const char* name_;
		const char* description_;

		init_function* init_;
		run_function* run_;
	public:
		command(const char* name, const char* description, init_function* init, run_function* run) : name_(name), description_(description), init_(init), run_(run) {}
		~command() {}

		inline auto name() { return name_; }
		inline auto description() { return description_; }

		inline auto init(dpp::slashcommand& command) { init_(command); }
		inline auto run(const dpp::slashcommand_t& event) { run_(event); }

		inline virtual bool compare_nodes(command* current, command* next) override final
		{
			return strcmp(current->name_, next->name_) < 0;
		}
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, &name::init, &name::run)