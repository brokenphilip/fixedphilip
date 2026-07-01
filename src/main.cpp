#include <filesystem>
#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <fixedphilip/command.h>
#include <fixedphilip/version.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/stopwatch.h>

#ifdef _WIN32
const char* hi = "idk";
#endif

#ifdef __linux__
const char* hello = "what";
#endif

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

const char* prefix_temp = "fp!";

int main(int argc, char const *argv[])
{
    fixedphilip::utils::program_uptime.start();
    fixedphilip::log::info(std::format("Running fixedphilip {} by brokenphilip", fixedphilip::build_version()/*, fixedphilip::build_date_time()*/));

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
    dpp::cluster bot(token, dpp::i_default_intents | dpp::i_message_content);

    bool restart = false;
    bot.current_application_get([&bot, &restart](const dpp::confirmation_callback_t& result)
    {
        if (auto app = std::get_if<dpp::application>(&result.value))
        {
            if (!(app->flags & dpp::apf_gateway_message_content_limited) && !(app->flags & dpp::apf_gateway_message_content))
            {
                fixedphilip::log::warning
                (
                    "The 'Message Content' privileged intent is not enabled for this application. "
                    "Features that require 'on_message_create' (when a message is sent) or 'on_message_update' "
                    "(when a message is edited), such as old-style prefix commands, will not work for this session. "
                    "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                );
                bot.intents &= ~dpp::i_message_content;

                // shards that have already started will be stuck in a reconnect loop if we don't fix their intents
                for (auto& shard : bot.get_shards())
                {
                    auto client = shard.second;
                    if (client)
                    {
                        client->intents &= ~dpp::i_message_content;
                    }
                }
            }
            else
            {
                // requires the "Message Content" privileged intent to be enabled on the bot
                bot.on_message_create([&bot](const dpp::message_create_t& event)
                {
                    auto iter = fixedphilip::command::first();
                    while (iter)
                    {
                        auto command = std::format("{}{}", prefix_temp, iter->name());

                        // old-style prefix commands - discouraged by Discord, but still convenient to have
                        if (event.msg.content == command || event.msg.content.starts_with(command + " "))
                        {
                            iter->run(fixedphilip::command::run_event(event));
                            break;
                        }
                        iter = iter->next();
                    }
                });
            }
        }
    });

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
                iter->run(fixedphilip::command::run_event(event));
                break;
            }
            iter = iter->next();
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            // first, fetch all previously registered commands, and remove the ones that don't exist anymore
            bot.global_commands_get([&bot](const dpp::confirmation_callback_t& result)
            {
                if (auto commands_ptr = std::get_if<dpp::slashcommand_map>(&result.value))
                {
                    auto commands = *commands_ptr;

                    // get stale commands
                    auto iter = fixedphilip::command::first();
                    while (iter)
                    {
                        auto name = iter->name();
                        std::erase_if(commands, [name](const auto& item)
                        {
                            // hold intellisense's hand
                            const dpp::slashcommand& command = item.second;
                            return strncmp(name, command.name.c_str(), strlen(name)) == 0;
                        });
                        iter = iter->next();
                    }

                    // delete stale commands
                    for (const auto& [key, value] : commands)
                    {
                        fixedphilip::log::info(std::format("Deleting stale command '{}'...", value.name));
                        bot.global_command_delete(key);
                    }
                }
                else
                {
                    if (result.is_error())
                    {
                        fixedphilip::log::error("idfk");
                    }
                    else
                    {
                        fixedphilip::log::error("global_commands_get: unknown error (result.value not of type slashcommand_map)");
                    }
                }
            });

            // finally, register our commands
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
    fixedphilip::utils::program_uptime.stop();
    return 0;
}
