#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <fixedphilip/command.h>
#include <fixedphilip/build.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/stopwatch.h>
#include <fixedphilip/utils/file.h>

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

int main(/*int argc, char const* argv[]*/)
{
    fixedphilip::utils::program_uptime.start();
    fixedphilip::log::info("====================");
    fixedphilip::log::info(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM));
    fixedphilip::log::info(std::format("Built on {}", fixedphilip::build::date_time()));
    fixedphilip::log::info(std::format("Targets " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", FIXEDPHILIP_BUILD_ARCHITECTURE_NUM));
    fixedphilip::log::info("====================");

    struct config_ : public fixedphilip::file::json_pretty_print
    {
        std::string token = FIXEDPHILIP_DEFAULT_TOKEN;
        std::string prefix = "fp!";

        virtual nlohmann::json struct_to_json() override final
        {
            return
            {
                { "token", token },
                { "prefix", prefix },
            };
        }
        virtual bool json_to_struct(const nlohmann::json& data) override final
        {
            try_at(data, "token", token);
            try_at(data, "prefix", prefix);

            // partial load is fine
            return true;
        }
    } config;

    fixedphilip::file::settings config_settings
    {
        .filename = "config.json",
        .create_if_not_found = true,
        .log = true,
    };

    config.load(config_settings);

    std::string token = config.token;
    if (token == FIXEDPHILIP_DEFAULT_TOKEN || token.empty())
    {
        fixedphilip::log::error("Token not set in config file - exiting...");
        return 1;
    }

    std::string prefix = config.prefix;
    if (prefix.empty())
    {
        fixedphilip::log::info("Old-style commands disabled (prefix is blank)");
    }
    else
    {
        fixedphilip::log::info(std::format("Global prefix for old-style commands set to '{}'", prefix));
    }
    
    config.save(config_settings);

    dpp::cluster bot(config.token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members);

    bot.current_application_get([&bot, &prefix](const dpp::confirmation_callback_t& result)
    {
        if (auto app = std::get_if<dpp::application>(&result.value))
        {
            uint32_t disabled_intents = 0;

            if (!(app->flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members)))
            {
                fixedphilip::log::warning
                (
                    "The 'Guild Members' privileged intent is not enabled for this application. "
                    "Features that require 'on_guild_member_add/remove' (when users join or leave a server), "
                    "'on_guild_member_update' (when a user's server info is updated) or complete member lists of servers, "
                    "such as displaying accurate statistics as to how many users the bot is serving, will not work for this session. "
                    "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                );
                disabled_intents |= dpp::i_guild_members;
            }

            if (!(app->flags & (dpp::apf_gateway_message_content_limited | dpp::apf_gateway_message_content)))
            {
                fixedphilip::log::warning
                (
                    "The 'Message Content' privileged intent is not enabled for this application. "
                    "Features that require 'on_message_create' (when a message is sent) or 'on_message_update' "
                    "(when a message is edited), such as old-style prefix commands, will not work for this session. "
                    "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                );
                disabled_intents |= dpp::i_message_content;
            }
            else
            {
                // requires the "Message Content" privileged intent to be enabled on the bot
                bot.on_message_create([&bot, &prefix](const dpp::message_create_t& event)
                {
                    if (!prefix.empty())
                    {
                        auto iter = fixedphilip::command::first();
                        while (iter)
                        {
                            auto command = std::format("{}{}", prefix, iter->name());

                            // old-style prefix commands - discouraged by Discord, but still convenient to have
                            if (event.msg.content == command || event.msg.content.starts_with(command + " "))
                            {
                                iter->run(fixedphilip::command::run_event(event));
                                break;
                            }
                            iter = iter->next();
                        }
                    }
                });
            }

            if (disabled_intents)
            {
                bot.intents &= ~disabled_intents;

                // shards that have already started will be stuck in a reconnect loop if we don't fix their intents
                // we don't need to reconnect them manually - they'll automatically reconnect anyways
                // or, if we manage to update the intent before the initial connection, there won't be a need for a reconnect
                for (auto& shard : bot.get_shards())
                {
                    auto client = shard.second;
                    if (client)
                    {
                        client->intents &= ~disabled_intents;
                    }
                }
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
            std::vector<dpp::slashcommand> commands;
            auto iter = fixedphilip::command::first();
            while (iter)
            {
                auto& command = commands.emplace_back(iter->name(), iter->description(), bot.me.id);
                iter->init(command);
                iter = iter->next();
            }
            bot.global_bulk_command_create(commands);
            fixedphilip::log::info(std::format("Registered {} commands", commands.size()));
        }
    });

    bot.start(dpp::st_wait);

    fixedphilip::log::info("Cluster shards terminated. Shutting down...");
    fixedphilip::utils::program_uptime.stop();
    return 0;
}
