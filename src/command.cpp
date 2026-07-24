#include <fixedphilip/command.h>

namespace fixedphilip
{
	const dpp::event_dispatch_t& command::run_event::event_dispatch() const
	{
		return std::visit([](auto& event_dispatch) -> const dpp::event_dispatch_t&
		{
			return event_dispatch;
		},
		*this);
	}

	void command::run_event::reply(const dpp::message& msg, dpp::command_completion_event_t callback) const
	{
		if (auto slash_command = get_slash_command())
		{
			slash_command->reply(msg, callback);
			return;
		}
		else if (auto message_create = get_message_create())
		{
			message_create->reply(msg, false, callback);
			return;
		}
		fixedphilip::log::error("command::run_event::reply: unreachable code reached");
	}

	dpp::async<dpp::confirmation_callback_t> command::run_event::co_reply(const dpp::message& msg) const
	{
		if (auto slash_command = get_slash_command())
		{
			return slash_command->co_reply(msg);
		}
		else if (auto message_create = get_message_create())
		{
			return message_create->co_reply(msg);
		}
		fixedphilip::log::error("command::run_event::co_reply: unreachable code reached");
		return {}; // C4715, unreachable
	}

	void command::run_event::thinking_start() const
	{
		if (auto slash_command = get_slash_command())
		{
			slash_command->thinking();
			return;
		}
		else if (auto message_create = get_message_create())
		{
			message_create->owner->channel_typing(message_create->msg.channel_id);
			return;
		}
		fixedphilip::log::error("command::run_event::thinking_start: unreachable code reached");
	}

	dpp::async<dpp::confirmation_callback_t> command::run_event::co_thinking_start() const
	{
		if (auto slash_command = get_slash_command())
		{
			return slash_command->co_thinking();
		}
		else if (auto message_create = get_message_create())
		{
			return message_create->owner->co_channel_typing(message_create->msg.channel_id);
		}
		fixedphilip::log::error("command::run_event::co_thinking_start: unreachable code reached");
		return {}; // C4715, unreachable
	}

	void command::run_event::thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback) const
	{
		if (auto slash_command = get_slash_command())
		{
			slash_command->edit_original_response(msg, callback);
			return;
		}
		else if (auto message_create = get_message_create())
		{
			message_create->reply(msg, false, callback);
			return;
		}
		fixedphilip::log::error("command::run_event::thinking_end: unreachable code reached");
	}

	void command::run_event::reply_not_impl_use_other(fixedphilip::discord::bot& bot) const
	{
		std::string command_text;
		if (auto slash_command = get_slash_command())
		{
			auto& prefix = bot.prefix();
			if (prefix.empty())
			{
				reply(":warning: **| Not implemented.");
				return;
			}
			command_text = "`" + prefix + slash_command->command.get_command_name() + "`";
		}
		else if (auto message_create = get_message_create())
		{
			auto name = message_create->msg.content.substr(bot.prefix().length());
			auto snowflake = bot.slash_command_snowflake(name);
			if (snowflake == dpp::snowflake(0))
			{
				fixedphilip::log::error("Failed to find snowflake for command " + name);
				command_text = "`/" + name + "`";
			}
			else
			{
				command_text = "</" + name + ":" + std::to_string(snowflake) + ">";
			}
		}
		else
		{
			fixedphilip::log::error("command::run_event::reply_not_impl_use_other: unreachable code reached");
			reply(":warning: **| Not implemented.");
			return;
		}
		reply(std::format(":warning: **| Not implemented, use {} instead.**", command_text));
	}
}