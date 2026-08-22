#include <discofloor/bot.h>
#include <discofloor/version.h>

#include <fixedphilip/build.h>

namespace discofloor
{
    class meta_module : public module
    {
        static dpp::task<void> run_shutdown(const run_event& event)
        {
            auto cluster = event.get_bot();
            if (event.command_invoker() == cluster->app_owner())
            {
                cluster->log(dpp::ll_info, "Shutdown initiated via command");
                co_await event.co_reply(":wave: **| Shutting down...**");

                // apparently you can't clear presences lol
                //bot.cluster().set_presence(dpp::presence(dpp::ps_offline, dpp::at_custom, ""));
                cluster->shutdown();
            }
            else
            {
                event.reply(":no_entry: **| Only the instance owner can run this command.**");
            }
        }

        static dpp::task<void> run_status(const run_event& event)
        {
            auto cluster = event.get_bot();
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

            std::string storage_usage = "N/A";
            try
            {
                auto size = cluster->data_size_total();
                auto max = cluster->settings().max_data_size_total;

                storage_usage = std::format("{} / {} ({:.2f} %)",
                    bulbtils::file::size_to_string(size),
                    bulbtils::file::size_to_string(max),
                    ((double)size / (double)max) * 100.0);
            }
            catch (std::exception& e)
            {
                cluster->log(dpp::ll_error, std::format("status command: Failed to fetch storage usage - {}", e.what()));
            }

            auto embed = dpp::embed()
                .set_color(0x7F00FF)
                .set_author(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM), "https://github.com/brokenphilip/fixedphilip", "https://cdn.discordapp.com/app-icons/449970784585121792/e1f2f0407a77ddd696202c7ec3720e1b.png")
                .set_description(
                    std::format(
                        "*Powered by discofloor " DISCOFLOOR_VERSION_STRING "*\n"
                        "*Using " DPP_VERSION_TEXT "*\n\n"
                        "**Built on:** {}\n"
                        "**Targets:** " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", 
                    fixedphilip::build::date_time(), 
                    FIXEDPHILIP_BUILD_ARCHITECTURE_NUM))
                .add_field("Instance owner", cluster->app_owner().username)
                .add_field("Uptime (Ping)", std::format("{} (`{} ms`)", uptime, static_cast<int>(cluster->rest_ping * 1000)))
                .add_field("User/server count",
                    std::format(
                        "\\- {} user{} in {} server{}\n"
                        "\\- {} user install{}\n"
                        "\\- {} total",
                    user_count, counts.users == 1 ? "" : "s", server_count, counts.servers == 1 ? "" : "s",
                    user_install_count, counts.user_installs == 1 ? "" : "s",
                    total_user_count))
                .add_field("Cluster ID", std::format("`{}` (out of {})", cluster->cluster_id, cluster->maxclusters))
                .add_field("Shard ID", std::format("`{}` (out of {})", event.event_dispatch().shard, cluster->numshards))
                .add_field("Storage usage", storage_usage)
                .set_footer(dpp::embed_footer().set_text("Last restarted"))
                .set_timestamp(cluster->start_time_unix());

            auto msg = dpp::message(embed);

            co_await thinking;
            event.thinking_end(msg);
        }

        static dpp::task<void> run_storage(const run_event& event)
        {
            auto cluster = event.get_bot();

            // bot data for id, or all
            dpp::snowflake id = 0;
            bool all = false;

            // optional user/guild for prettier reply
            dpp::user* user = nullptr;
            dpp::guild* guild = nullptr;

            auto invoker = event.command_invoker();

            // are we allowed to do dangerous things with this data+
            bool permission = false;
            if (invoker == cluster->app_owner())
            {
                // the instance owner can do everything
                permission = true;
            }

            auto subcmd_group = event.command_interaction().options[0];
            auto subcmd = subcmd_group.options[0];
            if (subcmd.name == "id")
            {
                auto id_str = event.get_cmd_required_param_value<std::string>("snowflake");
                id = dpp::snowflake(id_str);
                if (!id)
                {
                    event.reply(std::format(":x: **| \"{}\" is not a valid ID.**", dpp::utility::markdown_escape(id_str, true)));
                    co_return;
                }

                if (user = dpp::find_user(id))
                {
                    goto is_user;
                }
                if (guild = dpp::find_guild(id))
                {
                    goto is_guild;
                }
            }
            else if (subcmd.name == "user")
            {
                id = event.get_cmd_required_param_value<dpp::snowflake>("who");
                user = dpp::find_user(id);

            is_user:
                if (invoker.id == id)
                {
                    // users can run this command on themselves
                    permission = true;
                }
            }
            else if (subcmd.name == "guild")
            {
                guild = event.get_guild();
                if (!guild)
                {
                    event.reply(":x: **| This command is not being run in the context of a guild.** "
                        "To perform this command on a guild, execute it from there or provide its ID.");
                    co_return;
                }

            is_guild:
                if (guild->base_permissions(&invoker).can(dpp::p_manage_guild))
                {
                    // those that can invite bots can run this command
                    permission = true;
                }
            }
            else if (subcmd.name == "all")
            {
                all = true;
            }

            if (all)
            {
                if (subcmd_group.name == "usage")
                {
                    try
                    {
                        auto size = cluster->data_size_total();
                        auto max = cluster->settings().max_data_size_total;

                        event.reply(std::format(":floppy_disk: **| Total storage usage:** {} / {} ({:.2f} %)",
                            bulbtils::file::size_to_string(size),
                            bulbtils::file::size_to_string(max),
                            ((double)size / (double)max) * 100.0));
                    }
                    catch (std::exception& e)
                    {
                        event.reply(":x: **| Failed to get total storage usage.**");
                        cluster->log(dpp::ll_error, std::format("storage command: Failed to get total storage usage - {}", e.what()));
                    }
                }
                else if (subcmd_group.name == "erase")
                {
                    if (!permission)
                    {
                        event.reply(":no_entry: **| Only the instance owner can erase all bot data.**");
                        co_return;
                    }

                    try
                    {
                        auto files = std::filesystem::remove_all(std::filesystem::path(cluster->settings().data_folder));
                        event.reply(std::format(":white_check_mark: **| Successfully erased {} files of all bot data.**", files));
                    }
                    catch (std::exception& e)
                    {
                        event.reply(std::format(":x: **| Failed to erase all bot data:** {}", e.what()));
                    }
                }
            }
            else // individual (!all)
            {
                std::string target = "";
                if (user)
                {
                    target = std::format("\"{}\" (user)", user->username);
                }
                else if (guild)
                {
                    target = std::format("\"{}\" (guild)", dpp::utility::markdown_escape(guild->name, true));
                }
                else
                {
                    target = std::format("\"{}\"", std::to_string(id));
                }

                if (subcmd_group.name == "usage")
                {
                    try
                    {
                        auto size = cluster->data_size_id(id);
                        auto max = cluster->settings().max_data_size_id;

                        event.reply(std::format(":floppy_disk: **| Storage usage for {}:** {} / {} ({:.2f} %)",
                            target,
                            bulbtils::file::size_to_string(size),
                            bulbtils::file::size_to_string(max),
                            ((double)size / (double)max) * 100.0));
                    }
                    catch (std::exception& e)
                    {
                        event.reply(std::format(":x: **| Failed to get storage usage for {}.**", target));
                        cluster->log(dpp::ll_error, std::format("storage command: Failed to get storage usage for {} - {}", target, e.what()));
                    }
                }
                else if (subcmd_group.name == "erase")
                {
                    if (!permission)
                    {
                        if (user)
                        {
                            event.reply(std::format(":no_entry: **| Only {}, or the instance owner, can erase their bot data.**", target));
                        }
                        else if (guild)
                        {
                            event.reply(std::format(":no_entry: **| Only members with the \"Manage Guild\" permission, or the instance owner, can erase the bot data for {}.**", target));
                        }
                        else
                        {
                            event.reply(std::format(":no_entry: **| Only the instance owner can erase the bot data for {}.**", target));
                        }
                        co_return;
                    }

                    try
                    {
                        auto files = std::filesystem::remove_all(cluster->data_folder_id(id));
                        event.reply(std::format(":white_check_mark: **| Successfully erased {} files of bot data for {}.**", files, target));
                    }
                    catch (std::exception& e)
                    {
                        event.reply(std::format(":x: **| Failed to erase bot data for {}:** {}", target, e.what()));
                    }
                }
            }
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command shutdown("shutdown", "Shuts the bot down", bot.me.id, run_shutdown);
            command status("status", "Displays bot status", bot.me.id, run_status);

            command storage("storage", "Commands for bot data (storage) management", bot.me.id, run_storage);
            std::vector<dpp::command_option> storage_subcmd_groups = 
            {
                {dpp::co_sub_command_group, "usage", "Get usage/quota info for an ID"},
                {dpp::co_sub_command_group, "erase", "Erase all bot data for an ID"},
            };
            std::vector<dpp::command_option> storage_subcmds = 
            {
                {dpp::co_sub_command, "user", "Pass a user for ID"},
                {dpp::co_sub_command, "guild", "Pass this guild for ID"},
                {dpp::co_sub_command, "id", "Pass arbitrary ID"},
                {dpp::co_sub_command, "all", "Use total bot data instead of ID"},
            };
            storage_subcmds[0].add_option(dpp::command_option(dpp::co_user, "who", "Get ID from user", true));
            storage_subcmds[2].add_option(dpp::command_option(dpp::co_string, "snowflake", "Snowflake ID", true));
            for (auto& subcmd_group : storage_subcmd_groups)
            {
                for (auto& subcmd : storage_subcmds)
                {
                    // create copy of subcmd
                    auto subcmd_copy = subcmd;

                    // modify its description so it shows up properly
                    subcmd_copy.description = std::format("{} ({})", subcmd_group.description, subcmd.description);

                    subcmd_group.add_option(subcmd_copy);
                }
                storage.add_option(subcmd_group);
            }

            return { shutdown, status, storage };
        }
    public:
        meta_module() : module("meta") {}
    };
    static meta_module instance;
}