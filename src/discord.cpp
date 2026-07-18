#include <fixedphilip/discord.h>

#include <fixedphilip/command.h>
#include <fixedphilip/build.h>

#include <fixedphilip/utils/string.h>
#include <fixedphilip/utils/time.h>

bool fixedphilip::discord::bot::config::load_from_file(const std::string& filename)
{
    fixedphilip::utils::file::settings config_settings
    {
        .filename = filename,
        .create_if_not_found = true,
        .log = true,
    };

    auto result = load(config_settings);
    if (result == fixedphilip::utils::file::r_file_not_found)
    {
        fixedphilip::log::warning("Default config saved - make sure to update your bot token");
        return false;
    }
    else if (result != fixedphilip::utils::file::r_success)
    {
        // logs are already printed for us
        return false;
    }

    save(config_settings);

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

dpp::task<void> fixedphilip::discord::bot::on_message_create(const dpp::message_create_t& event)
{
    if (!instance_)
    {
        fixedphilip::log::error("on_message_create: bot was null");
        co_return;
    }

    // we don't want bots to run our commands
    if (event.msg.author.is_bot())
    {
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
                co_return;
            }
            iter = iter->next();
        }
    }
}

dpp::task<void> fixedphilip::discord::bot::on_ready(const dpp::ready_t& event)
{
    if (dpp::run_once<struct on_ready_init>())
    {
        if (!instance_)
        {
            fixedphilip::log::error("on_ready_init: bot was null");
            co_return;
        }
        co_await instance_->init_commands();
        co_await instance_->init_presence();
    }
}

dpp::task<void> fixedphilip::discord::bot::on_slashcommand(const dpp::slashcommand_t& event)
{
    if (!instance_)
    {
        fixedphilip::log::error("on_slashcommand: bot was null");
        co_return;
    }

    auto iter = fixedphilip::command::first();
    while (iter)
    {
        if (event.command.get_command_name() == iter->name())
        {
            co_await iter->run(fixedphilip::command::run_event(event));
            co_return;
        }
        iter = iter->next();
    }
}

dpp::task<void> fixedphilip::discord::bot::init_commands()
{
    std::vector<dpp::slashcommand> commands;
    auto iter = fixedphilip::command::first();
    while (iter)
    {
        auto name = iter->name();
        dpp::slashcommand command(name, iter->description(), cluster_.me.id);

        // we want to guarantee that commands are initialized in-order, so we co_await their init
        // not to mention "command" can be a dangling reference if commands goes out-of-scope and gets destroyed
        auto success = co_await iter->init(command);
        if (success)
        {
            commands.push_back(std::move(command));
        }

        // also add or update their version to the map
        //command_cache.command_version_map[name] = iter->version();

        iter = iter->next();
    }

    // update our command cache with the latest command (+version) data
    //command_cache.save(command_cache_settings);

    auto result = co_await cluster_.co_global_bulk_command_create(commands);
    if (auto command_map = fixedphilip::discord::get_if<dpp::slashcommand_map>("init_commands, co_global_bulk_command_create", result))
    {
        auto result_log = std::format("Registered {} command{}", command_map->size(), command_map->size() == 1 ? "" : "s");

        bool first_command = true;
        for (const auto& [snowflake, command] : *command_map)
        {
            if (first_command)
            {
                result_log += ": '" + command.name + "'";
            }
            else
            {
                result_log += ", '" + command.name + "'";
            }
            first_command = false;
        }

        fixedphilip::log::info(result_log);
    }
}

dpp::task<void> fixedphilip::discord::bot::init_presence()
{
    update_presence();
    cluster_.start_timer([](const dpp::timer& timer) -> dpp::task<void>
    {
        if (!instance_)
        {
            fixedphilip::log::error("update_presence timer: bot was null");
            co_return;
        }

        instance_->update_presence();
    },
    60 * instance_->config_.presence_update_rate_mins);

    co_return;
}

void fixedphilip::discord::bot::update_presence()
{
    const std::vector<std::pair<std::string, std::string>> token_conversion
    {
        { "%prefix%", config_.prefix },
        { "%version%", std::to_string(FIXEDPHILIP_BUILD_VERSION_NUM) },
    };

    std::string presence_string = instance_->config_.presence_activity;
    for (int i = 0; i < token_conversion.size(); i++)
    {
        fixedphilip::utils::string::replace_all(presence_string, token_conversion[i].first, token_conversion[i].second);
    }
    cluster_.set_presence(dpp::presence(instance_->config_.presence_status, instance_->config_.activity_type, presence_string));
}

void fixedphilip::discord::bot::fetch_app_info_async()
{    
    // as this function's name implies, the lambda will run asynchronously(!!!) and NOT when this function is called
    // it CAN'T be run synchronously - if we block the thread, the REST API request queue NEVER gets serviced !!!
    cluster_.current_application_get([](const dpp::confirmation_callback_t& result) -> dpp::task<void>
    {
        if (!instance_)
        {
            fixedphilip::log::error("fetch_app_info_async: bot was null");
            co_return;
        }

        if (auto app = fixedphilip::discord::get_if<dpp::application>("fetch_app_info_async, current_application_get", result))
        {
            auto& cluster = instance_->cluster_;

            auto& app_owner = app->owner;
            instance_->app_owner_ = app_owner;
            fixedphilip::log::info("Instance owner is: " + app_owner.username);

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
    // on_message_create is registered by fetch_app_info_async
    cluster_.on_slashcommand(on_slashcommand);
    cluster_.on_ready(on_ready);
}

dpp::task<fixedphilip::discord::bot::counts> fixedphilip::discord::bot::co_get_counts()
{
    counts counts;
    auto guild_cache = dpp::get_guild_cache();
    if (guild_cache)
    {
        std::vector<dpp::snowflake> users;

        int server_count = 0;

        // this fallback is used in case we lack the necessary intent for accurate results
        int fallback_user_count = 0;

        // we must lock the mutex while we're using the cache
        {
            std::shared_lock _(guild_cache->get_mutex());
            auto& guilds = guild_cache->get_container();

            for (const auto& [guild_snowflake, guild] : guilds)
            {
                if (!guild)
                {
                    continue;
                }
                for (const auto& [member_snowflake, member] : guild->members)
                {
                    auto user = member.get_user();
                    if (!user)
                    {
                        continue;
                    }
                    if (user->is_bot())
                    {
                        continue;
                    }
                    users.push_back(member_snowflake);
                }
                fallback_user_count += guild->member_count;
            }
            server_count = guilds.size();
        }

        std::sort(users.begin(), users.end());
        auto last = std::unique(users.begin(), users.end());
        users.erase(last, users.end());

        // cache these for later use, as we will only call the api once per interval
        static int user_install_count = -1;
        static bool has_guild_members_intent = false;
        static auto next_call = std::chrono::minutes(1);
        if (fixedphilip::utils::time::run_if_passed<struct fetch_app_data>(next_call))
        {
            auto result = co_await cluster_.co_current_application_get();
            if (auto app = fixedphilip::discord::get_if<dpp::application>("co_get_counts, co_current_application_get", result))
            {
                // these update daily, so one hour is generous enough
                next_call = std::chrono::minutes(60);

                user_install_count = app->approximate_user_install_count;
                has_guild_members_intent = (app->flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members));
            }
            else
            {
                // cached values are good enough, but try to update them again a bit later
                next_call = std::chrono::minutes(1);
            }
        }

        counts.servers = server_count;
        counts.users = has_guild_members_intent ? users.size() : fallback_user_count;
        counts.users_fallback = !has_guild_members_intent;
        counts.user_installs = user_install_count;
        counts.total_users = user_install_count >= 0 ? counts.users + user_install_count : -1;
    }
    co_return counts;
}
