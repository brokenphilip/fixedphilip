#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <fixedphilip/command.h>
#include <fixedphilip/build.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/stopwatch.h>
#include <fixedphilip/utils/file.h>
#include <fixedphilip/discord.h>

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

template <fixedphilip::utils::string_literal Name, typename T, void(*Callback)(T&)>
void wawa/*_impl*/(const dpp::confirmation_callback_t& result)
{

}

int main(/*int argc, char const* argv[]*/)
{
    fixedphilip::utils::program_uptime.start();
    fixedphilip::utils::program_start = std::chrono::system_clock::now();

    // 1. splash screen :3
    fixedphilip::log::info("====================");
    fixedphilip::log::info(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM));
    fixedphilip::log::info(std::format("Built on {}", fixedphilip::build::date_time()));
    fixedphilip::log::info(std::format("Targets " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", FIXEDPHILIP_BUILD_ARCHITECTURE_NUM));
    fixedphilip::log::info("====================");

    // 2. load config
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

    auto result = config.load(config_settings);
    if (result == fixedphilip::file::r_file_not_found)
    {
        fixedphilip::log::warning("Default config saved - make sure to update your bot token. Shutting down...");
        return 1;
    }
    else if (result != fixedphilip::file::r_success)
    {
        // logs are already printed for us
        return 1;
    }

    std::string token = config.token;
    if (token == FIXEDPHILIP_DEFAULT_TOKEN || token.empty())
    {
        fixedphilip::log::error("Bot token not set in config file - shutting down...");
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
    
    // 3. after loading the config, initialize our bot/cluster, fetch the required data and set the appropriate event callbacks
    dpp::cluster bot(config.token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members);

    // TODO: fix coroutines
    /*
    auto async_app_result = bot.co_current_application_get();
    auto app_result = async_app_result.sync_wait_for(std::chrono::seconds(5));

    if (app_result)
    {
        fixedphilip::discord::command_completion_event<dpp::application>("main.cpp, co_current_application_get", [&bot, &prefix](const auto& app)
        {
            fixedphilip::discord::instance_owner = app.owner;
            fixedphilip::log::info("Instance owner is: " + fixedphilip::discord::instance_owner.format_username());

            // check for any privileged intents - if we don't have permission to use them, disable them
            uint32_t disabled_intents = 0;

            if (!(app.flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members)))
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

            if (!(app.flags & (dpp::apf_gateway_message_content_limited | dpp::apf_gateway_message_content)))
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
        })
        (app_result.value());
    }
    */
    
    bot.current_application_get(fixedphilip::discord::command_completion_event<dpp::application>("main.cpp, current_application_get", [&bot, &prefix](const auto& app)
    {
        fixedphilip::discord::instance_owner = app.owner;
        fixedphilip::log::info("Instance owner is: " + fixedphilip::discord::instance_owner.format_username());

        // check for any privileged intents - if we don't have permission to use them, disable them
        uint32_t disabled_intents = 0;

        if (!(app.flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members)))
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

        if (!(app.flags & (dpp::apf_gateway_message_content_limited | dpp::apf_gateway_message_content)))
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
    }));

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
            // TODO: fix coroutines
            /*
            auto async_commands_result = bot.co_global_commands_get();
            auto commands_result = async_commands_result.sync_wait_for(std::chrono::seconds(5));

            fixedphilip::log::info("=== THEN YOU are doing good i think");

            if (commands_result)
            {
                fixedphilip::discord::command_completion_event<dpp::slashcommand_map>("main.cpp, co_global_commands_get", [&bot](const auto& slashcommand_map)
                {
                    // create modifiable copy of the command map
                    dpp::slashcommand_map command_map = slashcommand_map;

                    // the command cache contains when each command was last modified
                    struct command_cache_ : public fixedphilip::file::json_pretty_print
                    {
                        std::unordered_map<std::string, std::string> command_last_modified_map;

                        virtual nlohmann::json struct_to_json() override final
                        {
                            nlohmann::json data;
                            for (const auto& [command, last_modified] : command_last_modified_map)
                            {
                                data[command] = last_modified;
                            }
                            return data;
                        }
                        virtual bool json_to_struct(const nlohmann::json& data) override final
                        {
                            auto iter = fixedphilip::command::first();
                            while (iter)
                            {
                                try_at(data, iter->name(), command_last_modified_map[iter->name()]);
                                iter = iter->next();
                            }
                            return true;
                        }
                    } command_cache;

                    fixedphilip::file::settings command_cache_settings
                    {
                        .filename = "command_cache.json",
                        .create_if_not_found = true,
                        .log = true,
                    };

                    command_cache.load(command_cache_settings);

                    // check for stale commands
                    auto iter = fixedphilip::command::first();
                    while (iter)
                    {
                        auto name = iter->name();
                        auto last_modified = iter->last_modified();
                        std::erase_if(command_map, [name, last_modified, &command_cache](const auto& item)
                            {
                                auto command_name_match = !strncmp(name, item.second.name.c_str(), strlen(name));
                                if (command_name_match)
                                {
                                    // fetch the last modified date/time before we modify it
                                    auto last_modified_match = !strncmp(command_cache.command_last_modified_map[name].c_str(), last_modified, strlen(last_modified));

                                    if (!last_modified_match)
                                    {
                                        fixedphilip::log::info(std::format("Command '{}' last modified: {} -> {}", name, command_cache.command_last_modified_map[name], last_modified));
                                        command_cache.command_last_modified_map[name] = last_modified;
                                    }

                                    // if both the command name and the last modified date/time match, the command is NOT stale - delete it from the map
                                    return last_modified_match;
                                }
                                return false;
                            });
                        iter = iter->next();
                    }

                    // delete stale commands
                    for (const auto& [key, value] : command_map)
                    {
                        fixedphilip::log::info(std::format("Deleting stale command '{}'...", value.name));
                        bot.global_command_delete(key);
                    }

                    // update our command cache with the latest data
                    command_cache.save(command_cache_settings);

                    fixedphilip::log::info("=== IF YOU SEE THESE TWO LOG LINES IN ORDER");
                })
                (commands_result.value());
            }

            // finally, register our commands (outside of the event, in case it fails)
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
            */

            // first, check for stale commands, and remove them - we must do this after the bot is ready, because the dpp::slashcommand_map is empty otherwise
            bot.global_commands_get(fixedphilip::discord::command_completion_event<dpp::slashcommand_map>("main.cpp, global_commands_get", [&bot](const auto& slashcommand_map)
            {
                // create modifiable copy of the command map
                dpp::slashcommand_map command_map = slashcommand_map;

                // the command cache contains when each command was last modified
                struct command_cache_ : public fixedphilip::file::json_pretty_print
                {
                    std::unordered_map<std::string, std::string> command_last_modified_map;

                    virtual nlohmann::json struct_to_json() override final
                    {
                        nlohmann::json data;
                        for (const auto& [command, last_modified] : command_last_modified_map)
                        {
                            data[command] = last_modified;
                        }
                        return data;
                    }
                    virtual bool json_to_struct(const nlohmann::json& data) override final
                    {
                        auto iter = fixedphilip::command::first();
                        while (iter)
                        {
                            try_at(data, iter->name(), command_last_modified_map[iter->name()]);
                            iter = iter->next();
                        }
                        return true;
                    }
                } command_cache;

                fixedphilip::file::settings command_cache_settings
                {
                    .filename = "command_cache.json",
                    .create_if_not_found = true,
                    .log = true,
                };

                command_cache.load(command_cache_settings);

                // check for stale commands
                auto iter = fixedphilip::command::first();
                while (iter)
                {
                    auto name = iter->name();
                    auto last_modified = iter->last_modified();
                    std::erase_if(command_map, [name, last_modified, &command_cache](const auto& item)
                    {
                        auto command_name_match = !strncmp(name, item.second.name.c_str(), strlen(name));
                        if (command_name_match)
                        {
                            // fetch the last modified date/time before we modify it
                            auto last_modified_match = !strncmp(command_cache.command_last_modified_map[name].c_str(), last_modified, strlen(last_modified));

                            if (!last_modified_match)
                            {
                                fixedphilip::log::info(std::format("Command '{}' last modified: {} -> {}", name, command_cache.command_last_modified_map[name], last_modified));
                                command_cache.command_last_modified_map[name] = last_modified;
                            }

                            // if both the command name and the last modified date/time match, the command is NOT stale - delete it from the map
                            return last_modified_match;
                        }
                        return false;
                    });
                    iter = iter->next();
                }

                // delete stale commands
                for (const auto& [key, value] : command_map)
                {
                    fixedphilip::log::info(std::format("Deleting stale command '{}'...", value.name));
                    bot.global_command_delete(key);
                }

                // update our command cache with the latest data
                command_cache.save(command_cache_settings);

                // finally, register our commands (within this event, because it will be too early otherwise)
                std::vector<dpp::slashcommand> commands;
                iter = fixedphilip::command::first();
                while (iter)
                {
                    auto& command = commands.emplace_back(iter->name(), iter->description(), bot.me.id);
                    iter->init(command);
                    iter = iter->next();
                }
                bot.global_bulk_command_create(commands);
                fixedphilip::log::info(std::format("Registered {} commands", commands.size()));
            }));
        }
    });

    bot.start(dpp::st_wait);

    fixedphilip::log::info("Cluster shards terminated - shutting down...");
    fixedphilip::utils::program_uptime.stop();
    return 0;
}
