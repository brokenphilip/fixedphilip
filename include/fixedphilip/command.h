#pragma once

#include <cstring> // strcmp
#include <variant>

#include <fixedphilip/node.h>

#include <dpp/appcommand.h> // dpp::slashcommand
#include <dpp/dispatcher.h> // dpp::*_t events

namespace fixedphilip
{
	class command : public node<command>
	{
	public:
		using run_event_t = std::variant<dpp::slashcommand_t, dpp::message_create_t>;

		struct run_event : public run_event_t
		{
			inline auto get_slash_command() const { return std::get_if<dpp::slashcommand_t>(this); }
			inline auto get_message_create() const { return std::get_if<dpp::message_create_t>(this); }

			inline auto get_event_dispatch() const
			{
				if (auto slash_command = get_slash_command())
				{
					return static_cast<const dpp::event_dispatch_t*>(slash_command);
				}
				else if (auto message_create = get_message_create())
				{
					return static_cast<const dpp::event_dispatch_t*>(message_create);
				}
				return static_cast<const dpp::event_dispatch_t*>(nullptr);
			}

			inline void reply(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const
			{
				if (auto slash_command = get_slash_command())
				{
					slash_command->reply(msg, callback);
				}
				else if (auto message_create = get_message_create())
				{
					message_create->reply(msg, false, callback);
				}
			}
			inline void reply(const std::string& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const { reply(dpp::message(msg), callback); }
		};

		using init_function = void(dpp::slashcommand& command);
		using run_function = void(const run_event& event);
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
		inline auto run(const run_event& event) { run_(event); }

		inline virtual bool compare_nodes(command* current, command* next) override final { return strcmp(current->name_, next->name_) < 0; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, &name::init, &name::run)