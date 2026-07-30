#include <fixedphilip/discord.h>

#include <fixedphilip/build.h>

#include <fixedphilip/utils/string.h>
#include <fixedphilip/utils/time.h>

namespace fixedphilip::discord
{
    nlohmann::json bot::config::struct_to_json()
    {
        nlohmann::json data
        {
            { "token", token },
        };
        data.update(settings.struct_to_json());
        return data;
    }

    bool bot::config::json_to_struct(const nlohmann::json& data)
    {
        auto data_copy = data;
        fixedphilip::file::json_try_at(data_copy, "token", token);
        data_copy.erase("token");
        return settings.json_to_struct(data_copy);
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

        if (settings.disabled_modules.empty())
        {
            fixedphilip::log::info("No modules will be disabled");
        }
        else
        {
            std::string result_log = std::format("If existing and enabled, {} module{} will be disabled", settings.disabled_modules.size(), settings.disabled_modules.size() == 1 ? "" : "s");
            bool first_module = true;
            for (auto& module : settings.disabled_modules)
            {
                if (first_module)
                {
                    result_log += ": '" + module + "'";
                }
                else
                {
                    result_log += ", '" + module + "'";
                }
                first_module = false;
            }
            fixedphilip::log::info(result_log);
        }

        // using this opportunity to add any new keys that might not exist
        save(config_settings);
        return true;
    }

    bot* bot::command::run_event::get_bot() const
    {
        return std::visit([](auto& event_dispatch)
        {
            return static_cast<bot*>(event_dispatch.owner);
        },
        *this);
    }

    const dpp::interaction_create_t* bot::command::run_event::get_interaction_create() const
    {
        if (auto slash_command = get_slash_command())
        {
            return slash_command;
        }
        else if (auto message_menu = get_message_context_menu())
        {
            return message_menu;
        }
        else if (auto user_menu = get_user_context_menu())
        {
            return user_menu;
        }
        return nullptr;
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
        if (auto message_create = get_message_create())
        {
            message_create->reply(msg, false, callback);
            return;
        }
        get_interaction_create()->reply(msg, callback);
    }

    dpp::async<dpp::confirmation_callback_t> bot::command::run_event::co_reply(const dpp::message& msg) const
    {
        if (auto message_create = get_message_create())
        {
            return message_create->co_reply(msg);
        }
        return get_interaction_create()->co_reply(msg);
    }

    void bot::command::run_event::thinking_start() const
    {
        if (auto message_create = get_message_create())
        {
            message_create->owner->channel_typing(message_create->msg.channel_id);
            return;
        }
        get_interaction_create()->thinking();
    }

    dpp::async<dpp::confirmation_callback_t> bot::command::run_event::co_thinking_start() const
    {
        if (auto message_create = get_message_create())
        {
            return message_create->owner->co_channel_typing(message_create->msg.channel_id);
        }
        return get_interaction_create()->co_thinking();
    }

    void bot::command::run_event::thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback) const
    {
        if (auto message_create = get_message_create())
        {
            message_create->reply(msg, false, callback);
            return;
        }
        get_interaction_create()->edit_original_response(msg, callback);
    }

    void bot::command::run_event::reply_not_impl_use_other() const
    {
        auto cluster = get_bot();
        if (!cluster)
        {
            fixedphilip::log::error("reply_not_impl_use_other: cluster was null");
            reply(":warning: **| Not implemented.");
            return;
        }

        auto prefix = cluster->settings().prefix;
        std::string command_text;
        if (auto slash_command = get_slash_command())
        {
            if (prefix.empty())
            {
                reply(":warning: **| Not implemented.");
                return;
            }
            command_text = "`" + prefix + slash_command->command.get_command_name() + "`";
        }
        else if (auto message_create = get_message_create())
        {
            auto name = message_create->msg.content.substr(prefix.length());
            auto snowflake = cluster->slash_command_snowflake(name);
            if (snowflake == dpp::snowflake(0))
            {
                cluster->log(dpp::ll_error, "Failed to find snowflake for command " + name);
                command_text = "`/" + name + "`";
            }
            else
            {
                command_text = dpp::utility::slashcommand_mention(snowflake, name);
            }
        }
        else
        {
            cluster->log(dpp::ll_error, "reply_not_impl_use_other: incorrectly called by wrong run_event type");
            reply(":warning: **| Not implemented.");
            return;
        }
        reply(std::format(":warning: **| Not implemented, use {} instead.**", command_text));
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
            cluster->ready_init_done_ = true;
            cluster->log(dpp::ll_info, "Connected and logged in as: " + cluster->me.format_username());
            cluster->create_commands_async();
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

    void bot::create_commands_async()
    {
        // we must create a local copy specifically to pass to global_bulk_command_create()
        std::vector<dpp::slashcommand> commands;

        // iteratively erase all module commands in case we're updating them via module late-load
        {
            std::unique_lock _(module_commands_mutex_);

            auto module_command = module_commands_.begin();
            while (module_command != module_commands_.end())
            {
                if (module_command->type == dpp::ctxm_chat_input)
                {
                    on_slashcommand.detach(module_command->event_handles[0]);
                    if ((intents & dpp::i_message_content) != 0)
                    {
                        on_message_create.detach(module_command->event_handles[1]);
                    }
                }
                else if (module_command->type == dpp::ctxm_message)
                {
                    on_message_context_menu.detach(module_command->event_handles[0]);
                }
                else if (module_command->type == dpp::ctxm_user)
                {
                    on_user_context_menu.detach(module_command->event_handles[0]);
                }
                module_command = module_commands_.erase(module_command);
            }

            {
                std::shared_lock _(loaded_modules_mutex_);

                for (auto& loaded_module : loaded_modules_)
                {
                    for (auto& command : loaded_module->commands(*this))
                    {
                        module_commands_.emplace_back(command);
                        commands.push_back(command);
                    }
                }
            }
        }

        // as this function's name implies, the lambda will run asynchronously(!!!) and NOT when this function is called
        global_bulk_command_create(commands, [](const dpp::confirmation_callback_t& result) -> dpp::task<void>
        {
            if (auto command_map = fixedphilip::discord::get_if<dpp::slashcommand_map>("init_commands, global_bulk_command_create", result))
            {
                // HACK: you can't really do any of this safely anyways, might as well cast away the const
                auto cluster = static_cast<bot*>(const_cast<dpp::cluster*>(result.bot));
                if (!cluster)
                {
                    fixedphilip::log::error("init_commands, global_bulk_command_create: bot was null");
                    co_return;
                }

                // we take the command_map results instead of our own (later down the line)
                // because we want to informatively print which commands in specific discord has created
                // ie. if any mistakes or errors happen below, we'll just print logs separately
                auto result_log = std::format("Created {} command{}", command_map->size(), command_map->size() == 1 ? "" : "s");

                bool first_command = true;
                {
                    std::unique_lock _(cluster->module_commands_mutex_);

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

                        // find module command from the slashcommand map we're given (they're identical if their names AND TYPES match)
                        auto module_command = std::find_if(cluster->module_commands_.begin(), cluster->module_commands_.end(), [&command](const bot::module_command& other)
                        {
                            return command.name == other.name && command.type == other.type;
                        });
                        if (module_command == cluster->module_commands_.end())
                        {
                            cluster->log(dpp::ll_error, std::format("Module command '{}' not found", command.name));
                            continue;
                        }

                        module_command->snowflake = snowflake;
                        if (module_command->type == dpp::ctxm_chat_input)
                        {
                            module_command->event_handles[0] = cluster->on_slashcommand.attach(
                                [name = module_command->name, run_fn = module_command->get_run_fn()]
                                (const dpp::slashcommand_t& event) -> dpp::task<void>
                            {
                                if (event.command.get_command_name() == name) co_await run_fn(fixedphilip::discord::bot::command::run_event(event));
                            });

                            if ((cluster->intents & dpp::i_message_content) != 0)
                            {
                                module_command->event_handles[1] = cluster->on_message_create.attach(
                                    [prefix = cluster->settings().prefix, name = module_command->name, run_fn = module_command->get_run_fn()]
                                    (const dpp::message_create_t& event) -> dpp::task<void>
                                {
                                    // we don't want bots to run our commands
                                    if (event.msg.author.is_bot())
                                    {
                                        co_return;
                                    }

                                    if (!prefix.empty())
                                    {
                                        auto command = std::format("{}{}", prefix, name);
                                        if (event.msg.content == command || event.msg.content.starts_with(command + " "))
                                        {
                                            co_await run_fn(fixedphilip::discord::bot::command::run_event(event));
                                        }
                                    }
                                });
                            }
                        }
                        else if (module_command->type == dpp::ctxm_message)
                        {
                            module_command->event_handles[0] = cluster->on_message_context_menu.attach(
                                [name = module_command->name, run_fn = module_command->get_run_fn()]
                                (const dpp::message_context_menu_t& event) -> dpp::task<void>
                            {
                                if (event.command.get_command_name() == name) co_await run_fn(fixedphilip::discord::bot::command::run_event(event));
                            });
                        }
                        else if (module_command->type == dpp::ctxm_user)
                        {
                            module_command->event_handles[0] = cluster->on_user_context_menu.attach(
                                [name = module_command->name, run_fn = module_command->get_run_fn()]
                                (const dpp::user_context_menu_t& event) -> dpp::task<void>
                            {
                                if (event.command.get_command_name() == name) co_await run_fn(fixedphilip::discord::bot::command::run_event(event));
                            });
                        }
                        else
                        {
                            cluster->log(dpp::ll_error, std::format("Command '{}' is of invalid type", module_command->name));
                            cluster->module_commands_.erase(module_command);
                        }
                    }
                }

                // all commands iterated, print resulting log
                cluster->log(dpp::ll_info, result_log);
            }
        });
    }

    void bot::fetch_app_info_async()
    {
        // as this function's name implies, the lambda will run asynchronously(!!!) and NOT when this function is called
        // it CAN'T be run synchronously - if we block the thread, the REST API request queue NEVER gets serviced !!!
        current_application_get([](const dpp::confirmation_callback_t& result) -> dpp::task<void>
        {
            if (auto app = fixedphilip::discord::get_if<dpp::application>("fetch_app_info_async, current_application_get", result))
            {
                // HACK: you can't really do any of this safely anyways, might as well cast away the const
                auto cluster = static_cast<bot*>(const_cast<dpp::cluster*>(result.bot));
                if (!cluster)
                {
                    fixedphilip::log::error("fetch_app_info_async, current_application_get: bot was null");
                    co_return;
                }

                auto& app_owner = app->owner;
                cluster->app_owner_ = app_owner;
                cluster->log(dpp::loglevel::ll_info, "Application (instance) owner is: " + app_owner.username);

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

                    // disable on_message_create to prevent log spam
                    std::shared_lock _(cluster->module_commands_mutex_);
                    for (auto& command : cluster->module_commands_)
                    {
                        if (command.type == dpp::ctxm_chat_input)
                        {
                            cluster->on_message_create.detach(command.event_handles[1]);
                        }
                    }
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

    bot::bot(const std::string& token, const bot_settings& settings, uint32_t intents, uint32_t shards, uint32_t cluster_id, 
        uint32_t maxclusters, bool compressed, dpp::cache_policy_t policy, uint32_t pool_threads) 
        : dpp::cluster(token, intents, shards, cluster_id, maxclusters, compressed, policy, pool_threads), settings_(settings)
    {
        // attach our own events first (modules do it in their own inits, but we do their commands ourselves later)
        on_log.attach(log_event);
        on_ready.attach(ready_event);

        // we're doing this here instead of on_ready_init because we want this to run as soon as possible
        // to ideally avoid restarting clusters/shards after potentially fixing up their intents
        fetch_app_info_async();

        bool first_module = true;
        std::string result_log = "";

        // initialize modules alphabetically by their name
        // their commands are created in on_ready_init
        auto iter = fixedphilip::discord::bot::module::first();
        do
        {
            std::string name = iter->name();
            if (std::find(settings_.disabled_modules.begin(), settings_.disabled_modules.end(), name) != settings_.disabled_modules.end())
            {
                continue;
            }

            if (!iter->init(*this))
            {
                continue;
            }

            if (first_module)
            {
                result_log += ": '" + name + "'";
            }
            else
            {
                result_log += ", '" + name + "'";
            }
            first_module = false;
            loaded_modules_.push_back(iter);
        } 
        while (iter = iter->next());

        log(dpp::ll_info, std::format("Loaded {} module{}{}", loaded_modules_.size(), loaded_modules_.size() == 1 ? "" : "s", result_log));
    }

    bot::~bot()
    {
        // destroy loaded modules in reverse order of initialization
        for (auto& loaded_module : std::views::reverse(loaded_modules_))
        {
            loaded_module->destroy(*this);
        }
    }

    dpp::snowflake bot::slash_command_snowflake(const std::string& slash_command)
    {
        auto module_command = std::find_if(module_commands_.begin(), module_commands_.end(), [&slash_command](const bot::module_command& other)
        {
            return other.name == slash_command && other.type == dpp::ctxm_chat_input;
        });
        if (module_command == module_commands_.end())
        {
            return dpp::snowflake();
        }
        // module_command->id is 0 for some reason
        return module_command->snowflake;
    }

    bool bot::add_module(module* module_to_add)
    {
        if (!ready_init_done_)
        {
            // too early to add modules
            return false;
        }

        if (std::find(settings_.disabled_modules.begin(), settings_.disabled_modules.end(), module_to_add->name()) != settings_.disabled_modules.end())
        {
            // module disabled by config
            return false;
        }

        if (!module_to_add->init(*this))
        {
            // module itself did not want to be added
            return false;
        }

        loaded_modules_.insert(std::lower_bound(loaded_modules_.begin(), loaded_modules_.end(), module_to_add, [](const module* a, const module* b) { return strcmp(a->name(), b->name()) < 0; }), module_to_add);
        create_commands_async();
        return true;
    }

    bool bot::remove_module(module* module_to_add)
    {
        auto it = std::find(loaded_modules_.begin(), loaded_modules_.end(), module_to_add);
        if (it == loaded_modules_.end())
        {
            // module not loaded
            return false;
        }
        module_to_add->destroy(*this);
        loaded_modules_.erase(it);
        return true;
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
