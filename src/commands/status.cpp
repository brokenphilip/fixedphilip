#include <fixedphilip/command.h>
#include <fixedphilip/version.h>
#include <fixedphilip/utils/stopwatch.h>
#include <fixedphilip/log.h>

#include <dpp/dpp.h>

namespace status
{
    void init(dpp::slashcommand& command)
    {

    }

    void run(const fixedphilip::command::run_event& event)
    {
        auto event_dispatch = event.get_event_dispatch();
        if (!event_dispatch)
        {
            fixedphilip::log::error("status command: event_dispatch was null");
        }

        auto shard = event_dispatch->shard;
        auto cluster = event_dispatch->owner;
        if (cluster)
        {
            event.reply(std::format(
                "### Running fixedphilip {} ({}):\n"
                "- Uptime: {:%T}\n"
                "- Cluster: {}\n"
                "  - Total shards: {}\n"
                "- Shard: {}\n"
                "- Ping: {} ms",
                fixedphilip::build_version(), fixedphilip::build_date_time(),
                std::chrono::seconds(fixedphilip::utils::program_uptime.elapsed<std::chrono::seconds>()),
                cluster->cluster_id,
                cluster->numshards,
                shard,
                static_cast<int>(cluster->rest_ping * 1000)
            ));
        }
        else
        {
            fixedphilip::log::error("status command: cluster was null");

            event.reply(std::format(
                "### Running fixedphilip {} ({}):\n"
                "- Uptime: {:%T}\n"
                "- Shard: {}",
                fixedphilip::build_version(), fixedphilip::build_date_time(),
                std::chrono::seconds(fixedphilip::utils::program_uptime.elapsed<std::chrono::seconds>()),
                shard
            ));
        }
    }
}

FIXEDPHILIP_COMMAND(status, "Check bot status");