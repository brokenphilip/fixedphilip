#include <fixedphilip/command.h>

namespace fixedphilip::commands::ping
{
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        co_return true;
    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        event.reply(std::format(":ping_pong: **| Pong!** ({} ms)", static_cast<int>(bot.cluster().rest_ping * 1000)));
        co_return;
    }
}

FIXEDPHILIP_COMMAND(ping, "Ping-pong test with REST latency");