#include <fixedphilip/discord.h>
#include <fixedphilip/math.h>
#include <fixedphilip/build.h>

namespace fixedphilip::discord
{
    class utility_module : public bot::module
    {
        static dpp::task<void> run_shutdown(const bot::command::run_event& event)
        {
            auto cluster = event.get_bot();
            if (!cluster)
            {
                event.reply(":warning: **| An internal error occurred.**");
                fixedphilip::log::error("run_shutdown: bot was null");
                co_return;
            }

            dpp::user author;
            if (auto slash_command = event.get_slash_command())
            {
                author = slash_command->command.usr;
            }
            if (auto message_create = event.get_message_create())
            {
                author = message_create->msg.author;
            }

            if (author == cluster->app_owner())
            {
                cluster->log(dpp::ll_info, "Shutdown initiated via command");
                co_await event.co_reply(":wave: **| Shutting down...**");

                // apparently you can't clear presences lol
                //bot.cluster().set_presence(dpp::presence(dpp::ps_offline, dpp::at_custom, ""));
                cluster->shutdown();
            }
            else
            {
                event.reply(":no_entry: **| Only the instance owner can run this command!**");
            }
        }

        static dpp::task<void> run_status(const bot::command::run_event& event)
        {
            auto cluster = event.get_bot();
            if (!cluster)
            {
                event.reply(":warning: **| An internal error occurred.**");
                fixedphilip::log::error("run_status: bot was null");
                co_return;
            }

            auto thinking = event.co_thinking_start();
            auto uptime = cluster->format_running_time();
            auto counts = co_await cluster->co_get_counts();

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
                .add_field("Instance owner", cluster->app_owner().username)
                .add_field("Uptime (Ping)", std::format("{} (`{} ms`)", uptime, static_cast<int>(cluster->rest_ping * 1000)))
                .add_field("User count",
                    std::format(
                        "\\- {} user{} in {} server{}\n"
                        "\\- {} user install{}\n"
                        "\\- {} total",
                        user_count, counts.users == 1 ? "" : "s", server_count, counts.servers == 1 ? "" : "s",
                        user_install_count, counts.user_installs == 1 ? "" : "s",
                        total_user_count))
                .add_field("Cluster ID", std::format("`{}` (out of {})", cluster->cluster_id, cluster->maxclusters))
                .add_field("Shard ID", std::format("`{}` (out of {})", event.event_dispatch().shard, cluster->numshards))
                .add_field("CPU usage", "`???` (of total `???`)")
                .add_field("Memory usage", "`???` (of total `???`)")
                .add_field("Disk usage", "`???` (of total `???`)")
                .set_footer(dpp::embed_footer().set_text("Last restarted"))
                .set_timestamp(cluster->start_time_unix());

            auto msg = dpp::message(embed);

            co_await thinking;
            event.thinking_end(msg);
        }

        virtual std::vector<bot::command> commands(bot& bot) override final
        {
            bot::command shutdown("shutdown", "Shuts the bot down", bot.me.id, run_shutdown);
            bot::command status("status", "Displays bot status", bot.me.id, run_status);
            return { shutdown, status };
        }
    public:
        utility_module() : bot::module("utility", "Provides utility commands for bot management") {}
    };
    static utility_module instance;
}