#pragma once

#include <fixedphilip/utils/node.h>
#include <fixedphilip/discord.h>

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

			inline dpp::async<dpp::confirmation_callback_t> co_reply(const dpp::message& msg) const
			{
				if (auto slash_command = get_slash_command())
				{
					return slash_command->co_reply(msg);
				}
				else if (auto message_create = get_message_create())
				{
					return message_create->co_reply(msg);
				}
				return {}; // C4715, unreachable
			}
			inline dpp::async<dpp::confirmation_callback_t> co_reply(const std::string& msg) const { return co_reply(dpp::message(msg)); }

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

			inline dpp::async<dpp::confirmation_callback_t> co_thinking_start() const
			{
				if (auto slash_command = get_slash_command())
				{
					return slash_command->co_thinking();
				}
				else if (auto message_create = get_message_create())
				{
					return message_create->owner->co_channel_typing(message_create->msg.channel_id);
				}
				return {}; // C4715, unreachable
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
		using run_function = dpp::task<void>(const run_event& event, fixedphilip::discord::bot& bot);
	private:
		const char* name_;
		const char* description_;
		const char* version_;

		init_function* init_;
		run_function* run_;
	public:
		command(const char* name, const char* description, const char* version, init_function* init, run_function* run) : name_(name), description_(description), version_(version), init_(init), run_(run) {}
		~command() {}

		inline auto name() { return name_; }
		inline auto description() { return description_; }
		inline auto version() { return version_; }

		inline auto init(dpp::slashcommand& command) { init_(command); }
		inline dpp::task<void> run(const run_event& event)
		{
			auto bot = fixedphilip::discord::bot::get_instance();
			if (!bot)
			{
				fixedphilip::log::error(std::format("{} command: bot was null", name_));
				event.reply("An internal error occurred.");
				co_return;
			}
			co_await run_(event, *bot);
		}

		inline virtual bool compare_nodes(command* current, command* next) override final { return strcmp(current->name_, next->name_) < 0; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description, version) static fixedphilip::command name##_(#name, description, version, &name::init, &name::run)