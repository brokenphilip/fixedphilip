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

			const dpp::event_dispatch_t& event_dispatch() const;

			void reply(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const;
			inline void reply(const std::string& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const { reply(dpp::message(msg), callback); }

			dpp::async<dpp::confirmation_callback_t> co_reply(const dpp::message& msg) const;
			inline dpp::async<dpp::confirmation_callback_t> co_reply(const std::string& msg) const { return co_reply(dpp::message(msg)); }

			void thinking_start() const;
			dpp::async<dpp::confirmation_callback_t> co_thinking_start() const;

			void thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const;
			inline void thinking_end(const std::string& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const { thinking_end(dpp::message(msg), callback); }

			void reply_not_impl_use_other(fixedphilip::discord::bot& bot) const;
		};

		using init_function = dpp::task<bool>(dpp::slashcommand& command, fixedphilip::discord::bot& bot);
		using run_function = dpp::task<void>(const run_event& event, fixedphilip::discord::bot& bot);
	private:
		const char* description_;

		init_function* init_;
		run_function* run_;
	public:
		command(const char* name, const char* description, init_function* init, run_function* run) : 
			named_node<command>(name), description_(description), init_(init), run_(run) {}
		~command() {}

		inline auto description() { return description_; }

		dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot) { co_return co_await init_(command, bot); }
		dpp::task<void> run(const run_event& event, fixedphilip::discord::bot& bot) { co_await run_(event, bot); }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::command fixedphilip_command_##name##_(#name, description, &fixedphilip::commands::name::init, &fixedphilip::commands::name::run)