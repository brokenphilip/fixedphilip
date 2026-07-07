#pragma once

#include <fixedphilip/utils/named_node.h>
#include <fixedphilip/discord.h>

namespace fixedphilip
{
	class command : public utils::named_node<command>
	{
	public:
		struct run_event : public std::variant<dpp::slashcommand_t, dpp::message_create_t>
		{
			inline auto get_slash_command() const { return std::get_if<dpp::slashcommand_t>(this); }
			inline auto get_message_create() const { return std::get_if<dpp::message_create_t>(this); }

			inline auto& event_dispatch() const
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

		using init_function = dpp::task<void>(dpp::slashcommand& command, fixedphilip::discord::bot& bot);
		using run_function = dpp::task<void>(const run_event& event, fixedphilip::discord::bot& bot);
	private:
		const char* description_;
		const char* version_;

		init_function* init_;
		run_function* run_;

		inline auto get_bot(const std::string& log_prefix)
		{
			auto bot = fixedphilip::discord::bot::get_instance();
			if (!bot)
			{
				fixedphilip::log::error(std::format("{}: bot was null", log_prefix));
			}
			return bot;
		}
	public:
		command(const char* name, const char* description, const char* version, init_function* init, run_function* run) : named_node<command>(name), description_(description), version_(version), init_(init), run_(run) {}
		~command() {}

		inline auto description() { return description_; }
		inline auto version() { return version_; }

		inline dpp::task<void> init(dpp::slashcommand& command)
		{ 
			if (auto bot = get_bot(std::format("{} command, init", name())))
			{
				co_await init_(command, *bot);
			}
		}
		inline dpp::task<void> run(const run_event& event)
		{
			if (auto bot = get_bot(std::format("{} command, run", name())))
			{
				co_await run_(event, *bot);
			}
			else
			{
				event.reply(":warning: **| An internal error occurred.**");
			}
		}
	};
}

#define FIXEDPHILIP_COMMAND(name, description, version) static fixedphilip::command fixedphilip_command_##name##_(#name, description, version, &fixedphilip::commands::name::init, &fixedphilip::commands::name::run)