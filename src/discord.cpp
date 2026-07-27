#include <fixedphilip/discord.h>

#include <fixedphilip/build.h>

#include <fixedphilip/utils/string.h>
#include <fixedphilip/utils/time.h>

namespace fixedphilip::discord
{
    nlohmann::json bot::config::struct_to_json()
    {
        return
        {
            { "token", token },
            { "prefix", settings.prefix },
        };
    }

    bool bot::config::json_to_struct(const nlohmann::json& data)
    {
        try_at(data, "token", token);
        try_at(data, "prefix", settings.prefix);

        // partial load is fine
        return true;
    }

    bool bot::config::load_from_file(const std::string& filename)
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

        if (settings.prefix.empty())
        {
            fixedphilip::log::info("Old-style commands disabled (prefix is blank)");
        }
        else
        {
            fixedphilip::log::info(std::format("Global prefix for old-style commands set to '{}'", settings.prefix));
        }
        return true;
    }

    dpp::task<void> bot::message_create_event(const dpp::message_create_t& event)
    {
        auto cluster = static_cast<bot*>(event.owner);
        if (!cluster)
        {
            fixedphilip::log::error("message_create_event: owner was null");
            co_return;
        }

        // we don't want bots to run our commands
        if (event.msg.author.is_bot())
        {
            co_return;
        }

        std::string prefix = "";
        {
            std::shared_lock _(cluster->settings_mutex);
            prefix = cluster->settings_.prefix;
        }
        if (!prefix.empty())
        {
            for (auto& module_command : cluster->module_commands)
            {
                auto command = std::format("{}{}", prefix, module_command.name);

                // old-style prefix commands - discouraged by Discord, but still convenient to have
                if (event.msg.content == command || event.msg.content.starts_with(command + " "))
                {
                    co_await module_command.run(fixedphilip::discord::bot::command::run_event(event));
                    co_return;
                }
            }
        }
    }

    dpp::task<void> bot::ready_event(const dpp::ready_t& event)
    {
        if (dpp::run_once<struct ready_event_init>())
        {
            auto cluster = static_cast<bot*>(event.owner);
            if (!cluster)
            {
                fixedphilip::log::error("ready_event_init: owner was null");
                co_return;
            }
            cluster->log(dpp::ll_info, "Connected and logged in as: " + cluster->me.format_username());
            co_await cluster->init_commands();
            //co_await cluster->init_presence();
        }
    }

    dpp::task<void> bot::log_event(const dpp::log_t& event)
    {
        // line 195 of cluster.cpp doesn't seem correct... 		dpp::log_t logmsg(nullptr, 0, msg); - why pass nullptr/0 ?! ?! ?!
        fixedphilip::log::implementation(
            event.severity, 
            //std::format("Cl: {}, Sh: {}", 
            //    event.owner ? std::to_string(event.owner->cluster_id) : "N/A", 
            //    event.shard), 
            "",
            event.message);

        co_return;
    }

    dpp::task<void> bot::init_commands()
    {
        std::vector<dpp::slashcommand> slash_commands;

        auto iter = fixedphilip::discord::bot::module::first();
        while (iter)
        {
            auto iter_commands = iter->commands();
            for (auto& slash_command : iter_commands)
            {
                slash_commands.push_back(slash_command);
            }
            module_commands.insert(module_commands.end(), iter_commands.begin(), iter_commands.end());
            iter = iter->next();
        }

        auto result = co_await co_global_bulk_command_create(slash_commands);
        if (auto command_map = fixedphilip::discord::get_if<dpp::slashcommand_map>("init_commands, co_global_bulk_command_create", result))
        {
            auto result_log = std::format("Registered {} command{}", command_map->size(), command_map->size() == 1 ? "" : "s");

            bool first_command = true;
            for (const auto& [snowflake, command] : *command_map)
            {
                for (auto& module_command : module_commands)
                {
                    if (module_command.name == command.name)
                    {
                        register_command(command.name, [run_fn = module_command.get_run_fn()](const dpp::slashcommand_t& event) -> dpp::task<void>
                        {
                            co_await run_fn(fixedphilip::discord::bot::command::run_event(event));
                        });
                    }
                }
                slash_command_snowflakes[command.name] = snowflake;
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
            log(dpp::ll_info, result_log);
        }
    }

    const dpp::event_dispatch_t& bot::command::run_event::event_dispatch() const
    {
        return std::visit([](auto& event_dispatch) -> const dpp::event_dispatch_t&
        {
            return event_dispatch;
        },
        *this);
    }

    void bot::command::run_event::reply(const dpp::message& msg, dpp::command_completion_event_t callback) const
    {
        if (auto slash_command = get_slash_command())
        {
            slash_command->reply(msg, callback);
            return;
        }
        else if (auto message_create = get_message_create())
        {
            message_create->reply(msg, false, callback);
            return;
        }
    }

    dpp::async<dpp::confirmation_callback_t> bot::command::run_event::co_reply(const dpp::message& msg) const
    {
        if (auto slash_command = get_slash_command())
        {
            return slash_command->co_reply(msg);
        }
        else if (auto message_create = get_message_create())
        {
            return message_create->co_reply(msg);
        }
        return {}; // C4715, unreachable
    }

    void bot::command::run_event::thinking_start() const
    {
        if (auto slash_command = get_slash_command())
        {
            slash_command->thinking();
            return;
        }
        else if (auto message_create = get_message_create())
        {
            message_create->owner->channel_typing(message_create->msg.channel_id);
            return;
        }
    }

    dpp::async<dpp::confirmation_callback_t> bot::command::run_event::co_thinking_start() const
    {
        if (auto slash_command = get_slash_command())
        {
            return slash_command->co_thinking();
        }
        else if (auto message_create = get_message_create())
        {
            return message_create->owner->co_channel_typing(message_create->msg.channel_id);
        }
        return {}; // C4715, unreachable
    }

    void bot::command::run_event::thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback) const
    {
        if (auto slash_command = get_slash_command())
        {
            slash_command->edit_original_response(msg, callback);
            return;
        }
        else if (auto message_create = get_message_create())
        {
            message_create->reply(msg, false, callback);
            return;
        }
    }

    void bot::command::run_event::reply_not_impl_use_other() const
    {
        auto cluster = std::visit([](auto& event_dispatch)
        {
            return static_cast<bot*>(event_dispatch.owner);
        },
        *this);

        if (!cluster)
        {
            fixedphilip::log::error("reply_not_impl_use_other: cluster was null");
            reply(":warning: **| Not implemented.");
            return;
        }

        std::string command_text;
        if (auto slash_command = get_slash_command())
        {
            std::string prefix = "";
            {
                std::shared_lock _(cluster->settings_mutex);
                prefix = cluster->settings_.prefix;
            }
            if (prefix.empty())
            {
                reply(":warning: **| Not implemented.");
                return;
            }
            command_text = "`" + prefix + slash_command->command.get_command_name() + "`";
        }
        else if (auto message_create = get_message_create())
        {
            std::string prefix = "";
            {
                std::shared_lock _(cluster->settings_mutex);
                prefix = cluster->settings_.prefix;
            }
            auto name = message_create->msg.content.substr(prefix.length());
            auto snowflake = cluster->slash_command_snowflake(name);
            if (snowflake == dpp::snowflake(0))
            {
                fixedphilip::log::error("Failed to find snowflake for command " + name);
                command_text = "`/" + name + "`";
            }
            else
            {
                command_text = dpp::utility::slashcommand_mention(snowflake, name);
            }
        }
        reply(std::format(":warning: **| Not implemented, use {} instead.**", command_text));
    }

    /*
    dpp::task<void> bot::init_presence()
    {
        update_presence();

        auto update_rate_mins = instance_->config_.presence_update_rate_mins;
        if (update_rate_mins > 0)
        {
            cluster_.start_timer([](const dpp::timer& timer) -> dpp::task<void>
            {
                if (!instance_)
                {
                    fixedphilip::log::error("update_presence timer: bot was null");
                    co_return;
                }
                instance_->update_presence();
            },
            60 * update_rate_mins);
        }
        co_return;
    }

    void bot::update_presence()
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
    */
    void bot::fetch_app_info_async()
    {
        // as this function's name implies, the lambda will run asynchronously(!!!) and NOT when this function is called
        // it CAN'T be run synchronously - if we block the thread, the REST API request queue NEVER gets serviced !!!
        current_application_get([](const dpp::confirmation_callback_t& result) -> dpp::task<void>
        {
            if (auto app = fixedphilip::discord::get_if<dpp::application>("fetch_app_info_async, current_application_get", result))
            {
                // HACK: you can't really do any of this safely anyways, might as well cast away the const
                auto cluster = const_cast<dpp::cluster*>(result.bot);
                if (!cluster)
                {
                    fixedphilip::log::error("fetch_app_info_async, current_application_get: bot was null");
                    co_return;
                }

                auto& app_owner = app->owner;
                static_cast<bot*>(cluster)->app_owner_ = app_owner;
                cluster->log(dpp::loglevel::ll_info, "Instance owner is: " + app_owner.username);

                // check for any privileged intents - if we don't have permission to use them, disable them
                uint32_t intents_to_disable = 0;

                if (!(app->flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members)) && ((cluster->intents & dpp::i_guild_members) != 0))
                {
                    fixedphilip::log::warning
                    (
                        "The 'Guild Members' privileged intent is not enabled for this application. "
                        "Features that require 'on_guild_member_add/remove' (when users join or leave a server), "
                        "'on_guild_member_update' (when a user's server info is updated) or complete member lists of servers, "
                        "such as displaying accurate statistics as to how many users the bot is serving, will not work for this session. "
                        "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                    );
                    intents_to_disable |= dpp::i_guild_members;
                }

                if (!(app->flags & (dpp::apf_gateway_presence_limited | dpp::apf_gateway_presence)) && ((cluster->intents & dpp::i_guild_presences) != 0))
                {
                    fixedphilip::log::warning
                    (
                        "The 'Guild Presences' privileged intent is not enabled for this application. "
                        "Features that require user presence (status, activities) updates will not work for this session. "
                        "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                    );
                    intents_to_disable |= dpp::i_guild_presences;
                }

                if (!(app->flags & (dpp::apf_gateway_message_content_limited | dpp::apf_gateway_message_content)) && ((cluster->intents & dpp::i_message_content) != 0))
                {
                    fixedphilip::log::warning
                    (
                        "The 'Message Content' privileged intent is not enabled for this application. "
                        "Features that require 'on_message_create' (when a message is sent) or 'on_message_update' "
                        "(when a message is edited), such as old-style prefix commands, will not work for this session. "
                        "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                    );
                    intents_to_disable |= dpp::i_message_content;
                }
                else
                {
                    cluster->on_message_create.attach(message_create_event);
                }

                // shards that have already started will be stuck in a reconnect loop if we don't fix their intents
                // we don't need to reconnect them manually - they'll automatically reconnect anyways
                // or, if we manage to update the intent before the initial connection, there won't be a need for a reconnect
                if (intents_to_disable)
                {
                    cluster->intents &= ~intents_to_disable;

                    // HACK: ideally we'd use a unique_lock for the cluster's shards_mutex, but it is inaccessible (private)
                    for (auto& shard : cluster->get_shards())
                    {
                        auto client = shard.second;
                        if (client)
                        {
                            client->intents &= ~intents_to_disable;
                        }
                    }
                }
            }
        });
    }

    bot::bot(const std::string& token, const settings& settings, uint32_t intents, uint32_t shards, uint32_t cluster_id, 
        uint32_t maxclusters, bool compressed, dpp::cache_policy_t policy, uint32_t pool_threads) 
        : dpp::cluster(token, intents, shards, cluster_id, maxclusters, compressed, policy, pool_threads)
    {
        settings_ = settings;

        // attach our own events first (modules do it in their inits)
        on_log.attach(log_event);
        on_ready.attach(ready_event);
        // message_create_event is attached to on_message_create in fetch_app_info_async

        fetch_app_info_async();

        // initialize modules alphabetically by their name
        auto iter = fixedphilip::discord::bot::module::first();
        while (iter)
        {
            if (iter->init(*this))
            {
                loaded_modules_.push_back(iter);
            }
            iter = iter->next();
        }
    }

    bot::~bot()
    {
        // destroy loaded modules in reverse order of initialization
        auto iter = fixedphilip::discord::bot::module::last();
        while (iter)
        {
            if (loaded_modules_.empty())
            {
                return;
            }
            auto last_loaded_module = loaded_modules_.back();
            if (iter == last_loaded_module)
            {
                loaded_modules_.pop_back();
                iter->destroy(*this);
            }
            iter = iter->previous();
        }

        // todo race cond?
        for (auto& module_command : module_commands)
        {
            unregister_command(module_command.name);
        }
    }

    dpp::task<bot::counts> bot::co_get_counts()
    {
        counts counts;
        auto guild_cache = dpp::get_guild_cache();
        if (guild_cache)
        {
            co_return counts;
        }

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
            auto result = co_await co_current_application_get();
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
        co_return counts;
    }
}