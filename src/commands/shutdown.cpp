#include <fixedphilip/command.h>

#include <fixedphilip/log.h>

namespace fixedphilip::commands::shutdown
{
    void init(dpp::slashcommand& command)
    {

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
            co_await event.co_reply("Shutting down...");
            fixedphilip::log::info("Shutdown initiated via command");
            bot.cluster().shutdown();
        }
        else
        {
            event.reply("Only the instance owner can run this command");
        }
        co_return;
    }
}

FIXEDPHILIP_COMMAND(shutdown, "Shuts the bot down", "v1");