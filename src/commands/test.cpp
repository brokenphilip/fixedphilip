#include <fixedphilip/command.h>

namespace fixedphilip::commands::test
{
	dpp::task<void> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
	{
		command.set_nsfw(false);

		command.add_option
		(
			dpp::command_option(dpp::co_sub_command, "mention", "Mentions a user").add_option
			(
				dpp::command_option(dpp::co_user, "user", "User to mention")
			)
		);

		bot.cluster().start_timer([](const dpp::timer& timer) -> dpp::task<void>
		{
			auto bot = fixedphilip::discord::bot::get_instance();
			if (!bot)
			{
				fixedphilip::log::error("test command timer: bot was null");
				co_return;
			}

			std::chrono::year_month_day ymd(std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()));
			
			uint32_t day = static_cast<uint32_t>(ymd.day());
			static uint32_t old_day = 69;
			if (old_day != day)
			{
				old_day = day;

				auto get_suffix = [](uint32_t day) -> std::string
				{
					if (day == 11 || day == 12 || day == 13)
					{
						return "th";
					}

					switch (day % 10)
					{
						case 1: return "st";
						case 2: return "nd";
						case 3: return "rd";
						default: return "th";
					}
				};
				auto msg = dpp::message(std::format("it's the {}{} now, time for wordle! :3", day, get_suffix(day)));
				co_await bot->cluster().co_direct_message_create(bot->app_owner().id, msg);
			}
		},
		60);

		co_return;
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