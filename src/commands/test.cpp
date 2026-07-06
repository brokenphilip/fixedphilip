#include <fixedphilip/command.h>

namespace test
{
	void init(dpp::slashcommand& command)
	{
		command.set_nsfw(false);

		command.add_option
		(
			dpp::command_option(dpp::co_sub_command, "mention", "Mentions a user").add_option
			(
				dpp::command_option(dpp::co_user, "user", "User to mention")
			)
		);
	}

	dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
	{
		if (auto slash_command = event.get_slash_command())
		{
			auto sub_command = slash_command->command.get_command_interaction().options[0];
			if (sub_command.name == "mention")
			{
				if (sub_command.options.empty())
				{
					slash_command->reply("No user specified");
				}
				else
				{
					auto user = slash_command->command.get_resolved_user(sub_command.get_value<dpp::snowflake>(0));
					slash_command->reply("Mentioning " + user.get_mention());
				}
			}
		}
		else if (auto message_create = event.get_message_create())
		{
			message_create->reply("Not implemented, use the slash command instead.");
		}
		co_return;
	}
}

FIXEDPHILIP_COMMAND(test, "Test command", "v1");