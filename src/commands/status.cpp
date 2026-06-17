#include <command.h>
#include <dpp/dpp.h>

#include <format>
#include <version.h>
#include <utils/stopwatch.h>

namespace status
{
    void run(const dpp::slashcommand_t& event)
    {
        auto cluster = event.owner;
        if (cluster)
        {
            event.reply(std::format(
                "### Running fixedphilip {} ({}):\n"
                "- Uptime: {:%T}\n"
                "- Cluster: {}\n"
                "  - Total shards: {}\n"
                "- Shard: {}\n"
                "- Ping: {}",
                fixedphilip::build_version(), fixedphilip::build_date_time(),
                std::chrono::seconds(fixedphilip::utils::app_uptime.elapsed<std::chrono::seconds>()),
                cluster->cluster_id,
                cluster->numshards,
                event.shard,
                static_cast<int>(cluster->rest_ping * 1000)
            ));
        }
        else
        {
            event.reply(std::format(
                "### Running fixedphilip {} ({}):\n"
                "- Uptime: {:%T}\n"
                "- Shard: {}",
                fixedphilip::build_version(), fixedphilip::build_date_time(),
                std::chrono::seconds(fixedphilip::utils::app_uptime.elapsed<std::chrono::seconds>()),
                event.shard
            ));
        }
    }
}

FIXEDPHILIP_COMMAND(status, "Check bot status");