#include <fixedphilip/command.h>
#include <fixedphilip/build.h>
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
            return;
        }

        std::string uptime = "???";
        auto program_uptime = std::chrono::seconds(fixedphilip::utils::program_uptime.elapsed<std::chrono::seconds>());
        if (program_uptime > std::chrono::days(1))
        {
            auto days = std::chrono::duration_cast<std::chrono::days>(program_uptime);
            program_uptime -= days;
            uptime = std::format("{} {:%Hh %Mm}", days, program_uptime);
        }
        else if (program_uptime > std::chrono::hours(1))
        {
            uptime = std::format("{:%Hh %Mm %Ss}", program_uptime);
        }
        else if (program_uptime > std::chrono::minutes(1))
        {
            uptime = std::format("{:%Mm %Ss}", program_uptime);
        }
        else
        {
            uptime = std::format("{:%Ss}", program_uptime);
        }

        auto shard = event_dispatch->shard;
        auto cluster = event_dispatch->owner;

        if (!cluster)
        {
            fixedphilip::log::error("status command: cluster was null");
            return;
        }

        std::string owner = "?";
        cluster->current_application_get([&owner](const dpp::confirmation_callback_t& result)
        {
            if (auto app = std::get_if<dpp::application>(&result.value))
            {
                owner = app->owner.format_username();
            }
        });

        uint32_t server_count = 0xFFFFFFFF;
        uint32_t user_count = 0xFFFFFFFF;
        for (auto& shard : cluster->get_shards())
        {
            auto client = shard.second;
            if (client)
            {
                server_count = client->get_guild_count();
                //user_count = client->get_member_count();

                {
                    uint64_t total = 0;
                    dpp::cache<dpp::guild>* c = dpp::get_guild_cache();
                    std::shared_lock l(c->get_mutex());
                    std::unordered_map<dpp::snowflake, dpp::guild*>& gc = c->get_container();
                    for (auto g = gc.begin(); g != gc.end(); ++g)
                    {
                        dpp::guild* gp = (dpp::guild*)g->second;
                        fixedphilip::log::info(std::format("Guild: {}", gp->name));
                        for (auto& member : gp->members)
                        {
                            std::string username = "???";
                            if (auto user = member.second.get_user())
                            {
                                username = user->format_username();
                            }

                            user_count++;
                            fixedphilip::log::info(std::format("User #{}: {}", user_count, username));
                        }
                    }

                }
            }
        }

        // what a mouthful
        auto current_time = fixedphilip::utils::stopwatch::clock::now();
        auto current_uptime = std::chrono::seconds(fixedphilip::utils::program_uptime.elapsed<std::chrono::seconds>());
        auto launch_time = current_time - current_uptime;
        auto unix_launch_time = std::chrono::duration_cast<std::chrono::seconds>(launch_time.time_since_epoch()).count();

        auto embed = dpp::embed()
            .set_color(0x7F00FF)
            .set_author(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM), "https://github.com/brokenphilip/fixedphilip", "https://cdn.discordapp.com/app-icons/449970784585121792/e1f2f0407a77ddd696202c7ec3720e1b.png")
            .set_description(std::format("**Built on:** {}\n**Targets:** " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit\n**Instance owner:** `{}`\n**Instance launched:** <t:{}:s>",
                fixedphilip::build::date_time(),
                FIXEDPHILIP_BUILD_ARCHITECTURE_NUM,
                owner,
                unix_launch_time))
            .add_field("Ping", std::format("{} ms", static_cast<int>(cluster->rest_ping * 1000)), true)
            .add_field("Servers", std::to_string(server_count), true)
            .add_field("Users", std::to_string(user_count), true)
            .add_field("Cluster #", cluster ? std::format("`{}`", cluster->cluster_id) : "N/A", true)
            .add_field("Shards", cluster ? std::to_string(cluster->numshards) : "N/A", true)
            .add_field("Shard #", std::format("`{}`", shard), true)
            .add_field("Uptime", uptime, true)
            .add_field("CPU usage", "???", true)
            .add_field("Total CPU", "???", true)
            .add_field("Mem. usage", "???", true)
            .add_field("Free mem.", "???", true)
            .add_field("Total memory", "???", true)
            .add_field("Disk usage", "???", true)
            .add_field("Free storage", "???", true)
            .add_field("Total disk uage", "???", true);

        auto msg = dpp::message(embed);
        event.reply(msg);
    }
}

FIXEDPHILIP_COMMAND(status, "Check bot status");