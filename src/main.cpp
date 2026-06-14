#include <filesystem>
#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include "command.h"

namespace ping
{
    void Run(const dpp::slashcommand_t& event)
    {
        event.reply("Pong! :3");
    }
}

static fixedphilip::Command ping_("ping", "piiing pooong", &ping::Run);

namespace meow
{
    void Run(const dpp::slashcommand_t& event)
    {
        event.reply("mrrrrp >w<");
    }
}

static fixedphilip::Command meow_("meow", "meow :3 mrrrp >w<", &meow::Run);

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

namespace fixedphilip
{
    void dpp_cout_log(dpp::loglevel severity, std::string message)
    {
        if (severity > dpp::ll_trace)
        {
            std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(severity) << ": " << message << "\n";
        }
    }
}

#define FIXEDPHILIP_VERSION "0.1"

int main(int argc, char const *argv[])
{
    fixedphilip::dpp_cout_log(dpp::loglevel::ll_info, "fixedphilip " FIXEDPHILIP_VERSION " by brokenphilip");

    const char* config_file_name = "config.json";
    if (!std::filesystem::exists(config_file_name))
    {
        fixedphilip::dpp_cout_log(dpp::loglevel::ll_warning, std::format("Configuration file not found - creating '{}'...", config_file_name));
        std::ofstream config_file(config_file_name);
        if (!config_file)
        {
            fixedphilip::dpp_cout_log(dpp::loglevel::ll_error, "Failed to create the config file! Exiting...");
            return 1;
        }

        config_file << "{ \"token\": \"your_bot_token_here\" }";
        fixedphilip::dpp_cout_log(dpp::loglevel::ll_info, "Config file created - modify it before running the program again. Exiting...");

        return 1;
    }

    nlohmann::json config;
    {
        std::ifstream config_file(config_file_name);
        if (!config_file)
        {
            fixedphilip::dpp_cout_log(dpp::loglevel::ll_error, "Failed to read the config file! Exiting...");
            return 1;
        }

        config_file >> config;
    }

    std::string token = config["token"];
    dpp::cluster bot(token);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event)
    {
        auto iter = fixedphilip::Command::GetFirst();
        while (iter)
        {
            if (event.command.get_command_name() == iter->GetName())
            {
                iter->Run(event);
                break;
            }
            iter = iter->GetNext();
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            auto iter = fixedphilip::Command::GetFirst();
            while (iter)
            {
                bot.global_command_create(dpp::slashcommand(iter->GetName(), iter->GetDescription(), bot.me.id));
                iter = iter->GetNext();
            }
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
