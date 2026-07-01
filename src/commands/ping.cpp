#include <fixedphilip/command.h>
#include <fixedphilip/log.h>

#include <dpp/dpp.h>

namespace ping
{
    void init(dpp::slashcommand& command)
    {

    }

    void run(const fixedphilip::command::run_event& event)
    {
        auto event_dispatch = event.get_event_dispatch();
        if (!event_dispatch)
        {
            fixedphilip::log::error("ping command: event_dispatch was null");
        }

        auto cluster = event.get_event_dispatch()->owner;
        if (cluster)
        {
            event.reply(std::format("Pong! :3 ({} ms)", static_cast<int>(cluster->rest_ping * 1000)));
        }
        else
        {
            fixedphilip::log::warning("ping command: cluster was null");
            event.reply("Pong! :3");
        }
    }
}

FIXEDPHILIP_COMMAND(ping, "Ping-pong test with REST ping");