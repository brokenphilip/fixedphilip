#include <fixedphilip/discord.h>

#include <fixedphilip/command.h>

dpp::task<void> fixedphilip::discord::bot::on_message_create(const dpp::message_create_t& event)
{
    if (!instance_)
    {
        fixedphilip::log::error("on_message_create: bot was null");
        co_return;
    }

    auto& prefix = instance_->config_.prefix;
    if (!prefix.empty())
    {
        auto iter = fixedphilip::command::first();
        while (iter)
        {
            auto command = std::format("{}{}", prefix, iter->name());

            // old-style prefix commands - discouraged by Discord, but still convenient to have
            if (event.msg.content == command || event.msg.content.starts_with(command + " "))
            {
                co_await iter->run(fixedphilip::command::run_event(event));
                break;
            }
            iter = iter->next();
        }
    }
}

dpp::task<void> fixedphilip::discord::bot::on_ready(const dpp::ready_t& event)
{
    if (dpp::run_once<struct register_bot_commands>())
    {
        if (!instance_)
        {
            fixedphilip::log::warning("on_ready: bot was null");
            co_return;
        }
        auto& cluster = instance_->cluster();

        auto result = co_await cluster.co_global_commands_get();
        if (auto slashcommand_map = fixedphilip::discord::get_if<dpp::slashcommand_map>("on_ready, co_global_commands_get", result))
        {
            // create modifiable copy of the command map
            dpp::slashcommand_map command_map = *slashcommand_map;

            // the command cache contains when each command was last modified
            struct command_cache_ : public fixedphilip::file::json_pretty_print
            {
                std::unordered_map<std::string, std::string> command_version_map;

                virtual nlohmann::json struct_to_json() override final
                {
                    nlohmann::json data;
                    for (const auto& [command, version] : command_version_map)
                    {
                        data[command] = version;
                    }
                    return data;
                }
                virtual bool json_to_struct(const nlohmann::json& data) override final
                {
                    auto iter = fixedphilip::command::first();
                    while (iter)
                    {
                        try_at(data, iter->name(), command_version_map[iter->name()]);
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
                auto version = iter->version();
                std::erase_if(command_map, [name, version, &command_cache](const auto& item)
                {
                    auto command_name_match = !strncmp(name, item.second.name.c_str(), strlen(name));
                    if (command_name_match)
                    {
                        // fetch the command version before we modify it
                        auto version_match = !strncmp(command_cache.command_version_map[name].c_str(), version, strlen(version));

                        if (!version_match)
                        {
                            fixedphilip::log::info(std::format("Command '{}' version changed: {} -> {}", name, command_cache.command_version_map[name], version));
                            command_cache.command_version_map[name] = version;
                        }

                        // if both the command name and version match, the command is NOT stale - delete it from the map
                        return version_match;
                    }
                    return false;
                });
                iter = iter->next();
            }

            // delete stale commands
            for (const auto& [key, value] : command_map)
            {
                fixedphilip::log::info(std::format("Deleting stale command '{}'...", value.name));
                co_await cluster.co_global_command_delete(key);
            }

            // update our command cache with the latest data
            command_cache.save(command_cache_settings);
        }

        // finally, register our commands
        std::vector<dpp::slashcommand> commands;
        auto iter = fixedphilip::command::first();
        while (iter)
        {
            fixedphilip::log::info(std::format("Registering command '{}'...", iter->name()));
            auto& command = commands.emplace_back(iter->name(), iter->description(), cluster.me.id);
            iter->init(command);
            iter = iter->next();
        }
        cluster.global_bulk_command_create(commands);
    }
}

dpp::task<void> fixedphilip::discord::bot::on_slashcommand(const dpp::slashcommand_t& event)
{
    auto iter = fixedphilip::command::first();
    while (iter)
    {
        if (event.command.get_command_name() == iter->name())
        {
            co_await iter->run(fixedphilip::command::run_event(event));
            break;
        }
        iter = iter->next();
    }
}

void fixedphilip::discord::bot::get_app_info_async()
{    
    cluster_.current_application_get([](const dpp::confirmation_callback_t& result) -> dpp::task<void>
    {
        if (!instance_)
        {
            fixedphilip::log::warning("get_app_info_async: bot was null");
            co_return;
        }

        if (auto app = fixedphilip::discord::get_if<dpp::application>("get_app_info_async, current_application_get", result))
        {
            auto& cluster = instance_->cluster_;

            auto& app_owner = app->owner;
            instance_->app_owner_ = app_owner;
            fixedphilip::log::info("Instance owner is: " + app_owner.format_username());

            // check for any privileged intents - if we don't have permission to use them, disable them
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
                cluster.on_message_create(on_message_create);
            }

            if (disabled_intents)
            {
                cluster.intents &= ~disabled_intents;

                // shards that have already started will be stuck in a reconnect loop if we don't fix their intents
                // we don't need to reconnect them manually - they'll automatically reconnect anyways
                // or, if we manage to update the intent before the initial connection, there won't be a need for a reconnect
                for (auto& shard : cluster.get_shards())
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
}

void fixedphilip::discord::bot::register_events()
{
    cluster_.on_log(dpp::utility::cout_logger());
    cluster_.on_slashcommand(on_slashcommand);
    cluster_.on_ready(on_ready);
}

bool fixedphilip::discord::bot::config::load_from_file(const std::string& filename)
{
    fixedphilip::file::settings config_settings
    {
        .filename = filename,
        .create_if_not_found = true,
        .log = true,
    };

    auto result = load(config_settings);
    if (result == fixedphilip::file::r_file_not_found)
    {
        fixedphilip::log::warning("Default config saved - make sure to update your bot token");
        return false;
    }
    else if (result != fixedphilip::file::r_success)
    {
        // logs are already printed for us
        return false;
    }

    if (token == FIXEDPHILIP_DEFAULT_TOKEN || token.empty())
    {
        fixedphilip::log::error("Bot token not set in config file");
        return false;
    }

    if (prefix.empty())
    {
        fixedphilip::log::info("Old-style commands disabled (prefix is blank)");
    }
    else
    {
        fixedphilip::log::info(std::format("Global prefix for old-style commands set to '{}'", prefix));
    }
    return true;
}
