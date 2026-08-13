#include <fixedphilip/discord.h>

#include <fixedphilip/build.h>
#include <fixedphilip/math.h>

#include <fixedphilip/utils/string.h>
#include <fixedphilip/utils/time.h>

namespace fixedphilip::discord
{
    nlohmann::json bot_settings::struct_to_json() const
    {
        return
        {
            { "prefix", prefix },
            { "disabled_modules", disabled_modules },
            { "data_folder", data_folder },
            { "max_data_size_id", fixedphilip::file::size_to_string(max_data_size_id) },
            { "max_data_size_total", fixedphilip::file::size_to_string(max_data_size_total) },
        };
    }

    bool bot_settings::json_to_struct(const nlohmann::json& data)
    {
        fixedphilip::file::json_try_at(data, "prefix", prefix, true);
        fixedphilip::file::json_try_at(data, "disabled_modules", disabled_modules, true);
        fixedphilip::file::json_try_at(data, "data_folder", data_folder, true);

        std::string max_data_size_id_str;
        if (fixedphilip::file::json_try_at(data, "max_data_size_id", max_data_size_id_str, true))
        {
            try
            {
                fixedphilip::math::number_t max_data_size_id_num;
                fixedphilip::math::conversion::convert(max_data_size_id_str, "b", -1, false, nullptr, nullptr, &max_data_size_id_num);
                max_data_size_id = static_cast<uintmax_t>(max_data_size_id_num);
            }
            catch (std::exception& e)
            {
                fixedphilip::log::error(std::format("Failed to parse 'max_data_size_id' for bot settings - {}", e.what()));
            }
        }

        std::string max_data_size_total_str;
        if (fixedphilip::file::json_try_at(data, "max_data_size_total", max_data_size_total_str, true))
        {
            try
            {
                fixedphilip::math::number_t max_data_size_total_num;
                fixedphilip::math::conversion::convert(max_data_size_total_str, "b", -1, false, nullptr, nullptr, &max_data_size_total_num);
                max_data_size_total = static_cast<uintmax_t>(max_data_size_total_num);
            }
            catch (std::exception& e)
            {
                fixedphilip::log::error(std::format("Failed to parse 'max_data_size_total' for bot settings - {}", e.what()));
            }
        }

        // partial load is okay
        return true;
    }

    nlohmann::json bot::config::struct_to_json() const
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
        // create a copy of the data we will pass down to settings, but without the token
        auto data_copy = data;
        bool token_valid = fixedphilip::file::json_try_at(data_copy, "token", token, true);
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
        return std::visit([](auto&& arg) -> const dpp::interaction_create_t*
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_base_of_v<dpp::interaction_create_t, T>)
            {
                return &arg;
            }
            else
            {
                return nullptr;
            }
        },
        *this);
    }

    const dpp::event_dispatch_t& bot::command::run_event::event_dispatch() const
    {
        return std::visit([](auto& event_dispatch) -> const dpp::event_dispatch_t&
        {
            return event_dispatch;
        },
        *this);
    }

    dpp::user bot::command::run_event::get_command_invoker() const
    {
        if (auto message_create = get_message_create())
        {
            return message_create->msg.author;
        }
        return get_interaction_create()->command.usr;
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
        std::string command_text;

        auto cluster = get_bot();
        auto prefix = cluster->settings().prefix;
        
        if (auto slash_command = get_slash_command())
        {
            if (prefix.empty())
            {
                reply(":warning: **| Not implemented.**");
                return;
            }
            command_text = "`" + prefix + slash_command->command.get_command_name() + "`";
        }
        else if (auto message_create = get_message_create())
        {
            auto prefix_len = prefix.length();
            auto name = message_create->msg.content.substr(prefix_len, message_create->msg.content.find(' ') - prefix_len);

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
            throw std::logic_error("reply_not_impl_use_other called from unsupported run_event variant");
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
            cluster->log(dpp::ll_info, std::format("Connected and logged in as: {} ({})", cluster->me.format_username(), std::to_string(cluster->me.id)));
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

                        // note that we're creating a copy of the module command, not keeping a reference to it,
                        // ...because this lambda will run asynchronously at a later point, and the references would dangle in that case
                        auto event_router_async = [prefix = cluster->settings().prefix, command = *module_command](const auto& event) -> dpp::task<void>
                        {
                            using T = std::decay_t<decltype(event)>;

                            if constexpr (std::is_same_v<T, dpp::message_create_t>)
                            {
                                // we don't want bots to run our commands
                                if (event.msg.author.is_bot())
                                {
                                    co_return;
                                }

                                if (!prefix.empty())
                                {
                                    auto chat_command = std::format("{}{}", prefix, command.name);
                                    if (event.msg.content == chat_command)
                                    {
                                        co_await command.run(fixedphilip::discord::bot::command::run_event(event, {}));
                                    }
                                    else if (event.msg.content.starts_with(chat_command + " "))
                                    {
                                        // the first token will always be the command itself, since slashcommands can't have spaces
                                        // message/user context menu commands can, however, have spaces, but we don't care about those here
                                        auto chat_tokens = fixedphilip::utils::string::split_by_whitespace(event.msg.content);
                                        std::vector<dpp::command_data_option> options;

                                        /*
                                        
                                            TODO

                                            if core_cmd.options.size == 0, don't bother checking for chat tokens

                                            if core_cmd.options.size > 0, there are two possibilities:
                                            - if the first option is a subcmd (group), all the others are too
                                            - if it's not, all the options are params

                                            if core_cmd.options[i] is a subcmd group, all of its options must be subcmds, and all subcmd options must be params

                                            if core_cmd.options[i] is a subcmd, all of its options must be params

                                        */

                                        co_await command.run(fixedphilip::discord::bot::command::run_event(event, options));
                                    }
                                }
                            }
                            else if constexpr (std::is_base_of_v<dpp::interaction_create_t, T>)
                            {
                                if (event.command.get_command_name() == command.name)
                                {
                                    co_await command.run(fixedphilip::discord::bot::command::run_event(event));
                                }
                            }
                            else
                            {
                                // can't use false here, or it will never compile (as always, thanks raymond chen :3)
                                // https://devblogs.microsoft.com/oldnewthing/20200311-00/?p=103553
                                static_assert(!sizeof(T*), "Unsupported type T");
                            }
                        };

                        module_command->snowflake = snowflake;
                        if (module_command->type == dpp::ctxm_chat_input)
                        {
                            module_command->event_handles[0] = cluster->on_slashcommand.attach(event_router_async);

                            if ((cluster->intents & dpp::i_message_content) != 0)
                            {
                                module_command->event_handles[1] = cluster->on_message_create.attach(event_router_async);
                            }
                        }
                        else if (module_command->type == dpp::ctxm_message)
                        {
                            module_command->event_handles[0] = cluster->on_message_context_menu.attach(event_router_async);
                        }
                        else if (module_command->type == dpp::ctxm_user)
                        {
                            module_command->event_handles[0] = cluster->on_user_context_menu.attach(event_router_async);
                        }
                        else
                        {
                            // TODO: most likely unreachable
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
                cluster->log(dpp::loglevel::ll_info, std::format("Application (instance) owner is: {} ({})", app_owner.username, std::to_string(app_owner.id)));

                // check for any privileged intents - if we don't have permission to use them, disable them
                uint32_t intents_to_disable = 0;

                if (!(app->flags & (dpp::apf_gateway_guild_members_limited | dpp::apf_gateway_guild_members)) && ((cluster->intents & dpp::i_guild_members) != 0))
                {
                    fixedphilip::log::warning
                    (
                        "The 'Guild Members' privileged intent was requested but is not enabled for this application. "
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
                        "The 'Guild Presences' privileged intent was requested but is not enabled for this application. "
                        "Features that require user presence (status, activities) updates will not work for this session. "
                        "Visit the Discord Developer Portal page for your application/bot to enable the intent and fix this issue."
                    );
                    intents_to_disable |= dpp::i_guild_presences;
                }

                if (!(app->flags & (dpp::apf_gateway_message_content_limited | dpp::apf_gateway_message_content)) && ((cluster->intents & dpp::i_message_content) != 0))
                {
                    fixedphilip::log::warning
                    (
                        "The 'Message Content' privileged intent was requested but is not enabled for this application. "
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

    fixedphilip::file::settings bot::data_file_settings(dpp::snowflake id, const std::string& name)
    {
        auto filename = name + ".json";
        auto data_path = data_folder_id(id) / filename;

        fixedphilip::file::settings settings
        {
            .filename = data_path.string(),
            .create_if_not_found = true,
            .log = true,
        };
        return settings;
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

    std::string bot::format_running_time()
    {
        std::string str;
        auto elapsed = running_time_.elapsed<std::chrono::seconds>();
        if (elapsed > std::chrono::days(1))
        {
            auto days = std::chrono::duration_cast<std::chrono::days>(elapsed);
            elapsed -= days;
            str = std::format("{} {:%Hh %Mm}", days, elapsed);
        }
        else if (elapsed > std::chrono::hours(1))
        {
            str = std::format("{:%Hh %Mm %Ss}", elapsed);
        }
        else if (elapsed > std::chrono::minutes(1))
        {
            str = std::format("{:%Mm %Ss}", elapsed);
        }
        else
        {
            str = std::format("{:%Ss}", elapsed);
        }
        return str;
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

    fixedphilip::file::result bot::load_data(dpp::snowflake id, const std::string& name, bot::data& data_out)
    {
        return data_out.load(data_file_settings(id, name));
    }

    fixedphilip::file::result bot::save_data(dpp::snowflake id, const std::string& name, const bot::data& data)
    {
        auto data_size = data.save_from_struct().size();
        if (data_size_id(id) + data_size > settings_.max_data_size_id
            || data_size_total() + data_size > settings_.max_data_size_total)
        {
            return fixedphilip::file::r_write_error;
        }
        return data.save(data_file_settings(id, name));
    }

    uintmax_t bot::data_size_total()
    {
        return fixedphilip::file::get_folder_size(settings_.data_folder);
    }

    uintmax_t bot::data_size_id(dpp::snowflake id)
    {
        return fixedphilip::file::get_folder_size(data_folder_id(id));
    }

    std::filesystem::path bot::data_folder_id(dpp::snowflake id)
    {
        return std::filesystem::path(settings_.data_folder) / std::to_string(id);
    }

    dpp::task<bot::counts> bot::co_get_counts()
    {
        counts counts;
        auto guild_cache = dpp::get_guild_cache();
        if (!guild_cache)
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