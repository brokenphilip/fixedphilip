#include <filesystem>
#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <fixedphilip/command.h>
#include <fixedphilip/version.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/stopwatch.h>

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

int main(int argc, char const *argv[])
{
    fixedphilip::utils::app_uptime.start();
    fixedphilip::log::info(std::format("fixedphilip {} ({}) by brokenphilip", fixedphilip::build_version(), fixedphilip::build_date_time()));

    const char* config_file_name = "config.json";
    if (!std::filesystem::exists(config_file_name))
    {
        fixedphilip::log::warning(std::format("Configuration file not found - creating '{}'...", config_file_name));

        std::ofstream config_file(config_file_name);
        if (!config_file)
        {
            fixedphilip::log::error("Failed to create the config file! Shutting down...");
            return 1;
        }

        config_file << "{ \"token\": \"your_bot_token_here\" }";
        fixedphilip::log::info("Config file created - modify it before running the program again. Shutting down...");
        return 1;
    }

    nlohmann::json config;
    {
        std::ifstream config_file(config_file_name);
        if (!config_file)
        {
            fixedphilip::log::error("Failed to read the config file! Shutting down...");
            return 1;
        }

        config_file >> config;
    }

    std::string token = config["token"];
    dpp::cluster bot(token);

    //dpp::utility::bot_invite_url();
    //fixedphilip::dpp_cout_log(dpp::loglevel::ll_info, bot.invite

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event)
    {
        auto iter = fixedphilip::command::first();
        while (iter)
        {
            if (event.command.get_command_name() == iter->name())
            {
                iter->run(event);
                break;
            }
            iter = iter->next();
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            auto iter = fixedphilip::command::first();
            while (iter)
            {
                dpp::slashcommand command(iter->name(), iter->description(), bot.me.id);
                iter->init(command);
                bot.global_command_create(command);
                iter = iter->next();
            }
        }
    });

    bot.start(dpp::st_wait);

    fixedphilip::log::info("Cluster shards terminated. Shutting down...");
    return 0;
}
