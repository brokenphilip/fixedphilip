#include <fixedphilip/command.h>
#include <fixedphilip/build.h>
#include <fixedphilip/utils/stopwatch.h>
#include <fixedphilip/log.h>
#include <fixedphilip/discord.h>

#include <dpp/dpp.h>

#include <coroutine>

namespace status
{
    void init(dpp::slashcommand& command)
    {

    }

    void run(const fixedphilip::command::run_event& event)
    {
        auto& event_dispatch = event.get_event_dispatch();
        auto slash_command = event.get_slash_command();

        event.thinking_start();

        std::string uptime = "";
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

        auto shard = event_dispatch.shard;
        auto cluster = event_dispatch.owner;

        if (!cluster)
        {
            fixedphilip::log::error("status command: cluster was null");
            return;
        }

        std::string server_count = "?";
        std::string user_count = "?";

        auto guild_cache = dpp::get_guild_cache();
        if (guild_cache)
        {
            std::vector<dpp::snowflake> users;

            std::shared_lock _(guild_cache->get_mutex());
            auto& guilds = guild_cache->get_container();
            server_count = std::to_string(guilds.size());

            // this fallback is used in case we lack the necessary intent for accurate results
            int fallback_user_count_num = 0;

            for (const auto& [guild_snowflake, guild] : guilds)
            {
                if (!guild)
                {
                    continue;
                }

                fallback_user_count_num += guild->member_count;

                for (const auto& [member_snowflake, member] : guild->members)
                {
                    users.push_back(member_snowflake);
                }
            }

            std::sort(users.begin(), users.end());
            auto last = std::unique(users.begin(), users.end());
            users.erase(last, users.end());
            int user_count_num = users.size();

            // cache these numbers for later use
            static int user_install_count = 0;
            static bool has_guild_members_intent = true;

            // do not call more than once per day
            static fixedphilip::utils::stopwatch last_api_call;
            if (last_api_call.elapsed<std::chrono::minutes>() > 1440 || !last_api_call.running())
            {
                // TODO: fix coroutines
                /*
                auto async_result = cluster->co_current_application_get();
                auto result = async_result.sync_wait_for(std::chrono::seconds(5));
                
                if (result)
                {
                    fixedphilip::discord::command_completion_event<dpp::application>("status.cpp, co_current_application_get", [](const auto& app)
                    {
                        user_install_count = app.approximate_user_install_count;
                        has_guild_members_intent = (app.flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members));

                        last_api_call.reset();
                        last_api_call.start();
                    })
                    (result.value());
                }*/

                cluster->current_application_get(fixedphilip::discord::command_completion_event<dpp::application>("status.cpp, current_application_get", [](const auto& app)
                {
                    user_install_count = app.approximate_user_install_count;
                    has_guild_members_intent = (app.flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members));

                    last_api_call.reset();
                    last_api_call.start();
                }));

                // TODO fucking nuke me
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }

            // add the approximate amount of 'User Install' users, even if they might not be unique
            user_count_num += user_install_count;

            if (!has_guild_members_intent)
            {
                user_count_num += fallback_user_count_num;
            }
            user_count = std::format("{}{}", has_guild_members_intent ? "" : "~", user_count_num);
        }

        auto embed = dpp::embed()
            .set_color(0x7F00FF)
            .set_author(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM), "https://github.com/brokenphilip/fixedphilip", "https://cdn.discordapp.com/app-icons/449970784585121792/e1f2f0407a77ddd696202c7ec3720e1b.png")
            .set_description(std::format("**Built on:** {}\n**Targets:** " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit\n**Instance owner:** `{}`\n**Instance launched:** <t:{}:s>",
                fixedphilip::build::date_time(),
                FIXEDPHILIP_BUILD_ARCHITECTURE_NUM,
                fixedphilip::discord::instance_owner.format_username(),
                std::chrono::duration_cast<std::chrono::seconds>(fixedphilip::utils::program_start.time_since_epoch()).count()))
            .add_field("Ping", std::format("{} ms", static_cast<int>(cluster->rest_ping * 1000)), true)
            .add_field("Servers", server_count, true)
            .add_field("Users", user_count, true)
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
        event.thinking_end(msg);
    }
}

FIXEDPHILIP_COMMAND(status, "Check bot status");