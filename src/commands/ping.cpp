#include <fixedphilip/command.h>

namespace ping
{
    void init(dpp::slashcommand& command)
    {

    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        event.reply(std::format("Pong! :3 ({} ms)", static_cast<int>(bot.cluster().rest_ping * 1000)));
        co_return;
    }
}

FIXEDPHILIP_COMMAND(ping, "Ping-pong test with REST ping", "v1");