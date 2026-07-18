#include <fixedphilip/command.h>

namespace fixedphilip::commands::shutdown
{
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        co_return true;
    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        dpp::user author;

        if (auto slash_command = event.get_slash_command())
        {
            author = slash_command->command.usr;
        }
        if (auto message_create = event.get_message_create())
        {
            author = message_create->msg.author;
        }

        if (author == bot.app_owner())
        {
            fixedphilip::log::info("Shutdown initiated via command");
            co_await event.co_reply(":wave: **| Shutting down...**");
            
            // apparently you can't clear presences lol
            //bot.cluster().set_presence(dpp::presence(dpp::ps_offline, dpp::at_custom, ""));
            bot.cluster().shutdown();
        }
        else
        {
            event.reply(":no_entry: **| Only the instance owner can run this command!**");
        }
    }
}

FIXEDPHILIP_COMMAND(shutdown, "Shuts the bot down");