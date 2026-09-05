#include <discofloor/bot.h>
#include <discofloor/utility.h>

#include <bulbtils/string.h>

#include <regex>
#include <random>

namespace discofloor
{
    class fun_module : public module
    {
        struct fun_config : public pretty_print_json_file
        {
            struct emoji
            {
                std::string name = "";
                dpp::snowflake id = 0;
                bool animated = false;

                nlohmann::json struct_to_json(const bulbtils::file::settings& save_settings) const
                {
                    return
                    {
                        { "name", name },
                        { "id", id },
                        { "animated", animated },
                    };
                }

                bool json_to_struct(const nlohmann::json& data, const bulbtils::file::settings& load_settings, const std::string& internal_emoji_name)
                {
                    bulbtils::file::settings new_load_settings;

                    new_load_settings.warning_callback = [&load_settings, &internal_emoji_name](const std::string& log)
                        { load_settings.warning(std::format("Loading emoji '{}' - {}", internal_emoji_name, log)); };
                    new_load_settings.error_callback = [&load_settings, &internal_emoji_name](const std::string& log)
                        { load_settings.error(std::format("Loading emoji '{}' - {}", internal_emoji_name, log)); };

                    if (!json_try_at(data, new_load_settings, "name", name))
                    {
                        return false;
                    }

                    std::string id_str;
                    if (!json_try_at(data, new_load_settings, "id", id_str))
                    {
                        return false;
                    }

                    id = dpp::snowflake(id_str);
                    if (!id)
                    {
                        new_load_settings.error("invalid ID");
                        return false;
                    }

                    if (!json_try_at(data, new_load_settings, "animated", animated))
                    {
                        return false;
                    }

                    return true;
                }

                auto mention()
                {
                    return dpp::emoji::get_mention(name, id, animated);
                }
            };

            emoji heads, tails, flipping;
            emoji rolling[6];
            emoji rolled[6];

            virtual nlohmann::json struct_to_json(const bulbtils::file::settings& save_settings) const override final
            {
                auto rolling_array = nlohmann::json::array();
                for (int i = 0; i < 6; i++)
                {
                    rolling_array += rolling[i].struct_to_json(save_settings);
                }

                auto rolled_array = nlohmann::json::array();
                for (int i = 0; i < 6; i++)
                {
                    rolled_array += rolled[i].struct_to_json(save_settings);
                }

                return
                {
                    { "heads", heads.struct_to_json(save_settings) },
                    { "tails", tails.struct_to_json(save_settings) },
                    { "flipping", flipping.struct_to_json(save_settings) },
                    { "rolling", rolling_array },
                    { "rolled", rolled_array },
                };
            }
            virtual bool json_to_struct(const nlohmann::json& data, const bulbtils::file::settings& load_settings) override final
            {
                nlohmann::json heads_json;
                if (!json_try_at(data, load_settings, "heads", heads_json))
                {
                    return false;
                }
                if (!heads.json_to_struct(heads_json, load_settings, "heads"))
                {
                    return false;
                }

                nlohmann::json tails_json;
                if (!json_try_at(data, load_settings, "tails", tails_json))
                {
                    return false;
                }
                if (!tails.json_to_struct(tails_json, load_settings, "tails"))
                {
                    return false;
                }

                nlohmann::json flipping_json;
                if (!json_try_at(data, load_settings, "flipping", flipping_json))
                {
                    return false;
                }
                if (!flipping.json_to_struct(flipping_json, load_settings, "flipping"))
                {
                    return false;
                }

                nlohmann::json rolling_array;
                if (!json_try_at(data, load_settings, "rolling", rolling_array))
                {
                    return false;
                }
                auto rolling_count = rolling_array.size();
                if (rolling_count != 6)
                {
                    load_settings.error("Expected 6 'rolling' emojis, found " + rolling_count);
                    return false;
                }
                for (int i = 0; i < 6; i++)
                {
                    if (!rolling[i].json_to_struct(rolling_array[i], load_settings, std::format("rolling[{}]", i)))
                    {
                        return false;
                    }
                }

                nlohmann::json rolled_array;
                if (!json_try_at(data, load_settings, "rolled", rolled_array))
                {
                    return false;
                }
                auto rolled_count = rolled_array.size();
                if (rolled_count != 6)
                {
                    load_settings.error("Expected 6 'rolled' emojis, found " + rolled_count);
                    return false;
                }
                for (int i = 0; i < 6; i++)
                {
                    if (!rolled[i].json_to_struct(rolled_array[i], load_settings, std::format("rolled[{}]", i)))
                    {
                        return false;
                    }
                }
                return true;
            }
        };

        fun_config config;

        static auto random(int min_inclusive, int max_inclusive)
        {
            static std::random_device dev;
            static std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(min_inclusive, max_inclusive);
            return dist(rng);
        }

        static dpp::task<void> send_wait_edit(const run_event& event, const std::string& send, int wait_seconds, const std::string& edit)
        {
            // ignore C26811 here - this function is always co_await-ed and thus the references remain valid
            auto cluster = event.get_bot();
            if (auto message_command = event.get_message_command())
            {
                auto result = co_await message_command->co_reply(send);
                if (result.is_error())
                {
                    log_event(event, dpp::ll_error, "send_wait_edit: co_reply failed - " + result.get_error().human_readable);
                    co_return;
                }

                co_await cluster->co_sleep(wait_seconds);

                auto msg = result.get<dpp::message>();
                msg.set_content(edit);
                event.get_bot()->message_edit(msg);

            }
            else if (auto slash_command = event.get_slash_command())
            {
                auto result = co_await slash_command->co_reply(send);
                if (result.is_error())
                {
                    log_event(event, dpp::ll_error, "send_wait_edit: co_reply failed - " + result.get_error().human_readable);
                    co_return;
                }

                co_await cluster->co_sleep(wait_seconds);

                slash_command->edit_original_response(dpp::message(edit));
            }
        }

        dpp::task<void> run_coin(const run_event& event)
        {
            auto flipping_msg = config.flipping.mention();

            bool heads = random(0, 1);
            auto flipped = heads ? config.heads.mention() : config.tails.mention();

            auto flipped_msg = std::format("{} **| You flipped {}.**", flipped, heads ? "heads" : "tails");

            co_await send_wait_edit(event, flipping_msg, 2, flipped_msg);
        }

        dpp::task<void> run_dice(const run_event& event)
        {
            auto number = random(0, 5);

            auto& rolling = config.rolling[number];
            auto& rolled = config.rolled[number];

            auto rolling_msg = rolling.mention();
            auto rolled_msg = std::format("{} **| You rolled a {}.**", rolled.mention(), number + 1);

            co_await send_wait_edit(event, rolling_msg, 5, rolled_msg);
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command coin("coin", "Flip a coin", bot.me.id, 
                [this](const auto& event) -> dpp::task<void> { co_await run_coin(event); });

            command dice("dice", "Roll the dice", bot.me.id, 
                [this](const auto& event) -> dpp::task<void> { co_await run_dice(event); });

            return { coin, dice };
        }

        virtual bool init(bot& bot) override final
        {
            bulbtils::file::settings config_settings
            {
                .filename = "fun.json",
                .create_if_not_found = true,
            };
            bot.append_loggers(config_settings);

            auto result = config.load(config_settings);
            if (result != bulbtils::file::r_success)
            {
                bot.log(dpp::ll_error, "Failed to load 'fun.json' - fun module will not be loaded");
                return false;
            }
            return true;
        }
    public:
        fun_module() : module("fun") {}
    };
    static fun_module instance;
}