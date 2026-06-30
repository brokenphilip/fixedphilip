#include <fixedphilip/command.h>

#include <dpp/dpp.h>

namespace test
{
	void init(dpp::slashcommand& command)
	{
		command.set_nsfw(true);

		command.add_option
		(
			dpp::command_option(dpp::co_sub_command, "mention", "Mentions a user").add_option
			(
				dpp::command_option(dpp::co_user, "user", "User to mention")
			)
		);
	}

	void run(const dpp::slashcommand_t& event)
	{
		auto subcmd = event.command.get_command_interaction().options[0];
		if (subcmd.name == "user")
		{
			if (subcmd.options.empty())
			{
				event.reply("No user specified");
			}
			else
			{
				auto user = event.command.get_resolved_user(subcmd.get_value<dpp::snowflake>(0));
				event.reply("Mentioning " + user.get_mention());
			}
		}
	}
}

FIXEDPHILIP_COMMAND(test, "Test command");