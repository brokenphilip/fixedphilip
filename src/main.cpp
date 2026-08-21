#include <fixedphilip/log.h>
#include <fixedphilip/build.h>

#include <discofloor/bot.h>

bool handle_arg_num(int& num, const std::string& name, char* argv[], int index)
{
    constexpr const char* prefix = "--";

    std::string arg = prefix + name;

    if (arg != argv[index])
    {
        return false;
    }

    if (!argv[index + 1] || std::string(argv[index + 1]).starts_with(prefix))
    {
        throw std::runtime_error(std::format("Couldn't parse CLI argument '{}': missing parameter", name));
    }

    try
    {
        num = std::stoi(argv[index + 1]);
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::format("Couldn't parse CLI argument '{}': parameter '{}' is not a valid number", name, argv[index + 1]));
    }
    return true;
}

bool handle_arg_str(std::string& str, const std::string& name, char* argv[], int index)
{
    constexpr const char* prefix = "--";

    std::string arg = prefix + name;

    if (arg != argv[index])
    {
        return false;
    }

    if (!argv[index + 1])
    {
        throw std::runtime_error(std::format("Couldn't parse CLI argument '{}': missing parameter", name));
        return false;
    }

    str = argv[index + 1];
    return true;
}

int main(int argc, char* argv[])
{
    fixedphilip::log::info("==============================");
    fixedphilip::log::info(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM));
    fixedphilip::log::info(std::format("Built on {}", fixedphilip::build::date_time()));
    fixedphilip::log::info(std::format("Targets " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", FIXEDPHILIP_BUILD_ARCHITECTURE_NUM));
    fixedphilip::log::info("==============================");

    auto total_shards = 0;
    auto cluster_id = 0;
    auto max_clusters = 1;
    auto intents = dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members;
    std::string config_file = "config.json";

    try
    {
        for (int i = 1; i < argc; i++)
        {
            if (handle_arg_num(total_shards, "total-shards", argv, i))
            {
                i++;
                continue;
            }
            if (handle_arg_num(cluster_id, "cluster-id", argv, i))
            {
                i++;
                continue;
            }
            if (handle_arg_num(max_clusters, "max-clusters", argv, i))
            {
                i++;
                continue;
            }
            if (handle_arg_num(intents, "intents", argv, i))
            {
                i++;
                continue;
            }
            if (handle_arg_str(config_file, "config-file", argv, i))
            {
                i++;
                continue;
            }
        }
    }
    catch (std::exception& e)
    {
        fixedphilip::log::error(std::format("{} - shutting down...", e.what()));
        return 1;
    }

    if (total_shards < 0 || (total_shards > 0 && total_shards <= max_clusters))
    {
        fixedphilip::log::error(std::format("Total shards '{}' out of bounds (must be 0 or above {}) - shutting down...", total_shards, max_clusters - 1));
        return 1;
    }

    if (max_clusters < 1)
    {
        fixedphilip::log::error(std::format("Max clusters '{}' out of bounds (must be above zero) - shutting down...", max_clusters));
        return 1;
    }

    if (cluster_id < 0 || cluster_id >= max_clusters)
    {
        fixedphilip::log::error(std::format("Cluster ID '{}' out of bounds (must be between 0 and {}, inclusive) - shutting down...", cluster_id, max_clusters - 1));
        return 1;
    }

    fixedphilip::log::info(std::format("Starting bot with cluster ID {} (out of {})...", cluster_id, max_clusters));
    {
        bulbtils::file::settings config_settings
        {
            .filename = config_file,
            .create_if_not_found = true,
            .warning_callback = fixedphilip::log::warning,
            .error_callback = fixedphilip::log::error,
        };
        discofloor::bot_config bot_config;
        if (!bot_config.load_check_save(config_settings))
        {
            fixedphilip::log::error("Bot configuration failed - shutting down...");
            return 1;
        }

        auto logger = [](const dpp::log_t& event)
        {
            // line 195 of cluster.cpp doesn't seem correct... 		dpp::log_t logmsg(nullptr, 0, msg); - why pass nullptr/0 ?! ?! ?!
            fixedphilip::log::implementation(
                event.severity,
                //std::format("Cl: {}, Sh: {}", 
                //    event.owner ? std::to_string(event.owner->cluster_id) : "N/A", 
                //    event.shard), 
                "",
                event.message);
        };

        discofloor::bot bot(bot_config, logger, intents, total_shards, cluster_id, max_clusters);
        bot.start();
    }

    fixedphilip::log::info("Bot terminated - shutting down...");
    return 0;
}
