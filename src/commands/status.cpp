#include <fixedphilip/command.h>

#include <fixedphilip/build.h>
#include <fixedphilip/utils/time.h>

namespace fixedphilip::commands::status
{
    dpp::task<void> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        co_return;
    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        auto thinking = event.co_thinking_start();

        auto& event_dispatch = event.event_dispatch();
        auto& cluster = bot.cluster();

        std::string uptime = fixedphilip::utils::time::format_uptime();

        auto counts = co_await bot.co_get_counts();

        std::string server_count = "?";
        if (counts.servers >= 0)
        {
            server_count = std::to_string(counts.servers);
        }

        std::string user_count = "?";
        if (counts.users >= 0)
        {
            if (counts.users_fallback)
            {
                user_count = std::format("~{}", counts.users);
            }
            else
            {
                user_count = std::format("{} (unique, non-bot)", counts.users);
            }
        }

        std::string user_install_count = "?";
        if (counts.user_installs >= 0)
        {
            user_install_count = std::to_string(counts.user_installs);
        }

        std::string total_user_count = "?";
        if (counts.total_users >= 0)
        {
            total_user_count = std::to_string(counts.total_users);
        }

        auto embed = dpp::embed()
            .set_color(0x7F00FF)
            .set_author(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM), "https://github.com/brokenphilip/fixedphilip", "https://cdn.discordapp.com/app-icons/449970784585121792/e1f2f0407a77ddd696202c7ec3720e1b.png")
            .set_description(std::format("**Built on:** {}\n**Targets:** " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", fixedphilip::build::date_time(), FIXEDPHILIP_BUILD_ARCHITECTURE_NUM))
            .add_field("Instance owner", fixedphilip::discord::bot::get_instance()->app_owner().username)
            .add_field("Uptime (Ping)", std::format("{} (`{} ms`)", uptime, static_cast<int>(cluster.rest_ping * 1000)))
            .add_field("User count", 
                std::format(
                    "\\- {} user{} in {} server{}\n"
                    "\\- {} user install{}\n"
                    "\\- {} total",
                    user_count, counts.users == 1 ? "" : "s", server_count, counts.servers == 1 ? "" : "s",
                    user_install_count, counts.user_installs == 1 ? "" : "s",
                    total_user_count))
            .add_field("Shard #", std::format("`{}` (out of {})", event_dispatch.shard, cluster.numshards))
            .add_field("CPU usage", "`???` (of total `???`)")
            .add_field("Memory usage", "`???` (of total `???`)")
            .add_field("Disk usage", "`???` (of total `???`)")
            .set_footer(dpp::embed_footer().set_text("Last restarted"))
            .set_timestamp(std::chrono::duration_cast<std::chrono::seconds>(fixedphilip::utils::time::program_start.time_since_epoch()).count());

        auto msg = dpp::message(embed);

        co_await thinking;
        event.thinking_end(msg);
    }
}

FIXEDPHILIP_COMMAND(status, "Check bot status", "v1");