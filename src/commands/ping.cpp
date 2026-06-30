#include <command.h>
#include <dpp/dpp.h>

namespace ping
{
    void run(const dpp::slashcommand_t& event)
    {
        auto cluster = event.owner;
        if (cluster)
        {
            event.reply(std::format("Pong! :3 ({} ms)", static_cast<int>(cluster->rest_ping * 1000)));
        }
        else
        {
            event.reply("Pong! :3");
        }
    }
}

FIXEDPHILIP_COMMAND(ping, "Ping-pong test with REST ping");