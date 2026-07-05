#pragma once

#include <cstring> // strcmp
#include <variant>

#include <fixedphilip/utils/node.h>

#include <dpp/appcommand.h> // dpp::slashcommand
#include <dpp/dispatcher.h> // dpp::*_t events
#include <dpp/cluster.h>

namespace fixedphilip
{
	class command : public utils::node<command>
	{
	public:
		struct run_event : public std::variant<dpp::slashcommand_t, dpp::message_create_t>
		{
			inline auto get_slash_command() const { return std::get_if<dpp::slashcommand_t>(this); }
			inline auto get_message_create() const { return std::get_if<dpp::message_create_t>(this); }

			inline auto& get_event_dispatch() const
			{
				return std::visit([](auto& event_dispatch) -> const dpp::event_dispatch_t&
				{
					return event_dispatch;
				}, 
				*this);
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

			inline void thinking_start() const
			{
				if (auto slash_command = get_slash_command())
				{
					slash_command->thinking();
				}
				else if (auto message_create = get_message_create())
				{
					message_create->owner->channel_typing(message_create->msg.channel_id);
				}
			}

			inline void thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const
			{
				if (auto slash_command = get_slash_command())
				{
					slash_command->edit_original_response(msg, callback);
				}
				else if (auto message_create = get_message_create())
				{
					message_create->reply(msg, false, callback);
				}
			}
		};

		using init_function = void(dpp::slashcommand& command);
		using run_function = void(const run_event& event);
	private:
		const char* name_;
		const char* description_;
		const char* last_modified_;

		init_function* init_;
		run_function* run_;
	public:
		command(const char* name, const char* description, const char* last_modified, init_function* init, run_function* run) : name_(name), description_(description), last_modified_(last_modified), init_(init), run_(run) {}
		~command() {}

		inline auto name() { return name_; }
		inline auto description() { return description_; }
		inline auto last_modified() { return last_modified_; }

		inline auto init(dpp::slashcommand& command) { init_(command); }
		inline auto run(const run_event& event) { run_(event); }

		inline virtual bool compare_nodes(command* current, command* next) override final { return strcmp(current->name_, next->name_) < 0; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command name##_(#name, description, __DATE__ " " __TIME__, &name::init, &name::run)