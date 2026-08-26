#include <discofloor/bot.h>

#include <bulbtils/string.h>

#include <regex>
#include <random>

namespace discofloor
{
    class fun_module : public module
    {
        static auto random(int min_inclusive, int max_inclusive)
        {
            std::random_device dev;
            std::mt19937 rng(dev());
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
                    cluster->log(dpp::ll_error, "run_dice failed: " + result.get_error().human_readable);
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
                    cluster->log(dpp::ll_error, "run_dice failed: " + result.get_error().human_readable);
                    co_return;
                }

                co_await cluster->co_sleep(wait_seconds);

                slash_command->edit_original_response(dpp::message(edit));
            }
        }

        static dpp::task<void> run_coin(const run_event& event)
        {
            auto flipping = dpp::emoji::get_mention("fp_coin", 1541915249970647112, true);
            
            bool heads = random(0, 1);
            std::string coin = heads ? "heads" : "tails";
            dpp::snowflake id = heads ? 1541915227258490970 : 1541915228533559336;
            auto flipped = std::format("{} **| You flipped {}.**", dpp::emoji::get_mention("fp_" + coin, id), coin);

            co_await send_wait_edit(event, flipping, 2, flipped);
        }

        static dpp::task<void> run_dice(const run_event& event)
        {
            static const std::vector<dpp::emoji> rolling
            {
                { "fp_rolling_1", 1541889993557942475, dpp::e_animated },
                { "fp_rolling_2", 1541889989133205514, dpp::e_animated },
                { "fp_rolling_3", 1541889981335736510, dpp::e_animated },
                { "fp_rolling_4", 1541889983831478312, dpp::e_animated },
                { "fp_rolling_5", 1541889986050138303, dpp::e_animated },
                { "fp_rolling_6", 1541889991242940658, dpp::e_animated },
            };

            static const std::vector<dpp::emoji> rolled
            {
                { "fp_rolled_1", 1541890129742798948 },
                { "fp_rolled_2", 1541890128690020414 },
                { "fp_rolled_3", 1541890126987132978 },
                { "fp_rolled_4", 1541890125871579217 },
                { "fp_rolled_5", 1541890124697182270 },
                { "fp_rolled_6", 1541890123589746870 },
            };

            auto number = random(1, 6);
            auto rolling_msg = rolling[number - 1].get_mention();
            auto rolled_msg = std::format("{} **| You rolled a {}.**", rolled[number - 1].get_mention(), number);

            co_await send_wait_edit(event, rolling_msg, 5, rolled_msg);
        }

        class rps_game
        {
            static inline uint32_t counter = 0;
            uint32_t id;
        public:
            struct rps_choice
            {
                std::string name;
                std::string value;
                std::string emoji;

                rps_choice(const std::string& name, const std::string& value, const std::string& emoji)
                    : name(name), value(value), emoji(emoji) {}

                rps_choice() : name(""), value(""), emoji("") {}

                bool operator>(const rps_choice& other) const
                {
                    if (value == "rock")
                    {
                        return other.value == "scissors";
                    }
                    else if (value == "paper")
                    {
                        return other.value == "rock";
                    }
                    else //if (value == "scissors")
                    {
                        return other.value == "paper";
                    }
                }

                bool operator==(const rps_choice& other) const
                {
                    return value == other.value;
                }
            };
            static const inline std::vector<rps_choice> choices
            {
                { "Rock", "rock", "🪨" },
                { "Paper", "paper", "📄" },
                { "Scissors", "scissors", "✂️" },
            };
            rps_choice choice;
            dpp::user player;
            dpp::form_submit_t form_event;

            rps_game(uint32_t id, const rps_choice& choice, const dpp::user& player, const dpp::form_submit_t& form_event) 
                : id(id), choice(choice), player(player), form_event(form_event) {}

            std::string custom_id(const std::string& prefix) const { return prefix + "_" + std::to_string(id); }

            static std::string new_modal() { return "rps_modal_" + std::to_string(counter++); }
            static uint32_t id_from_modal(const std::string& modal)
            {
                auto id = modal;
                bulbtils::string::inplace::replace_all(id, "rps_modal_", "");
                try
                {
                    return std::stoul(id);
                }
                catch (std::exception& e)
                {
                    return -1;
                }
            }
        };
        std::vector<rps_game> rps_games;

        dpp::task<void> run_rps(const run_event& event)
        {
            if (auto message_command = event.get_message_command())
            {
                message_command->reply(":x: **| You can only play RPS via slash commands.**");
                co_return;
            }

            dpp::component select_rps;
            select_rps.set_id("select_rps");
            select_rps.set_type(dpp::cot_selectmenu);
            select_rps.set_label("What will you play?");
            select_rps.set_placeholder("Pick between Rock, Paper or Scissors");
            for (auto& choice : rps_game::choices)
            {
                select_rps.add_select_option(dpp::select_option(choice.name, choice.value).set_emoji(choice.emoji));
            }

            dpp::interaction_modal_response modal(rps_game::new_modal(), "Rock, Paper, Scissors", {select_rps});

            auto slash_command = event.get_slash_command();

            event.get_slash_command()->dialog(modal);
        }

        dpp::event_handle form_submit_handle = SIZE_MAX;
        dpp::task<void> form_submit_event(const dpp::form_submit_t& event)
        {
            if (!event.custom_id.starts_with("rps_modal_"))
            {
                co_return;
            }

            auto choice = std::find_if(rps_game::choices.begin(), rps_game::choices.end(),
                [chosen = std::get<std::string>(event.components[0].value)](const rps_game::rps_choice& it)
            {
                return it.value == chosen;
            });
            auto& rps = rps_games.emplace_back(rps_game::id_from_modal(event.custom_id), *choice, event.command.usr, event);

            dpp::component container;
            container.set_type(dpp::cot_container);

            dpp::component content;
            content.set_type(dpp::cot_text_display);
            content.set_content(std::format(
                "**{}** wants to play rock, paper, scissors!\n"
                "-# They've already selected an option, choose your response:",
                event.command.usr.username));

            container.add_component_v2(content);

            dpp::component actions;
            actions.set_type(dpp::cot_action_row);

            for (auto& choice : rps_game::choices)
            {
                dpp::component button;
                button.set_type(dpp::cot_button);
                button.set_label(choice.name);
                button.set_emoji(choice.emoji);
                button.set_style(dpp::cos_secondary);
                button.set_id(rps.custom_id(choice.value));

                actions.add_component_v2(button);
            }
            container.add_component_v2(actions);

            dpp::message msg;
            msg.set_flags(dpp::m_using_components_v2);
            msg.add_component_v2(container);

            event.reply(msg);
        }

        dpp::event_handle button_click_handle = SIZE_MAX;
        dpp::task<void> button_click_event(const dpp::button_click_t& event)
        {
            if (!event.custom_id.starts_with("rock_") && !event.custom_id.starts_with("paper_") && !event.custom_id.starts_with("scissors_"))
            {
                co_return;
            }

            bool is_rock = false;
            bool is_paper = false;
            bool is_scissors = false;

            auto rps = std::find_if(rps_games.begin(), rps_games.end(), [&is_rock, &is_paper, &is_scissors, custom_id = event.custom_id](const rps_game& it)
            {
                is_rock = it.custom_id("rock") == custom_id;
                is_paper = it.custom_id("paper") == custom_id;
                is_scissors = it.custom_id("scissors") == custom_id;
                return is_rock || is_paper || is_scissors;
            });

            if (rps == rps_games.end())
            {
                event.owner->log(dpp::ll_error, "fun::button_click: Failed to find RPS - " + event.custom_id);
                co_return;
            }

            rps_game::rps_choice opp_choice;
            if (is_rock)
            {
                opp_choice = rps_game::choices[0];
            }
            if (is_paper)
            {
                opp_choice = rps_game::choices[1];
            }
            if (is_scissors)
            {
                opp_choice = rps_game::choices[2];
            }

            dpp::component title;
            title.set_type(dpp::cot_text_display);
            title.set_content(std::format(
                "### {} {} :vs: {} {}",
                rps->player.username, rps->choice.emoji,
                opp_choice.emoji, event.command.usr.username));

            dpp::component container;
            container.set_type(dpp::cot_container);
            container.add_component_v2(title);

            dpp::component separator;
            separator.set_type(dpp::cot_separator);
            separator.set_spacing(dpp::sep_small);
            separator.set_divider(true);

            dpp::component content;
            content.set_type(dpp::cot_text_display);

            std::string motivator = "\n-# Create a new game using `/rps`";

            if (rps->choice == opp_choice)
            {
                content.set_content("It's a draw!" + motivator);
            }
            else if (rps->choice > opp_choice)
            {
                content.set_content(std::format("**{}** wins!{}", rps->player.username, motivator));
            }
            else
            {
                content.set_content(std::format("**{}** wins!{}", event.command.usr.username, motivator));
            }
            container.add_component_v2(separator);
            container.add_component_v2(content);

            dpp::message msg;
            msg.set_flags(dpp::m_using_components_v2);
            msg.add_component_v2(container);

            auto result = co_await rps->form_event.co_edit_original_response(msg);
            if (result.is_error())
            {
                event.owner->log(dpp::ll_error, "fun::button_click: failed to get original response for " + event.custom_id + " - " + result.get_error().human_readable);
                co_return;
            }
            event.reply(opp_choice.emoji + " **| Result:** " + result.get<dpp::message>().get_url());
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command coin("coin", "Flip a coin", bot.me.id, run_coin);
            command dice("dice", "Roll the dice", bot.me.id, run_dice);

            command rps("rps", "Initiate a 'Rock, Paper, Scissors' game", bot.me.id, 
                [this](const auto& event) -> dpp::task<void> { co_await run_rps(event); });

            return { coin, dice, rps };
        }

        virtual bool init(bot& bot) override final
        {
            form_submit_handle = bot.on_form_submit.attach([this](const auto& event) -> dpp::task<void> { co_await form_submit_event(event); });
            button_click_handle = bot.on_button_click.attach([this](const auto& event) -> dpp::task<void> { co_await button_click_event(event); });
            return true;
        }

        virtual void destroy(bot& bot) override final
        {
            bot.on_button_click.detach(button_click_handle);
            bot.on_form_submit.detach(form_submit_handle);

            for (auto& rps_game : rps_games)
            {
                dpp::component content;
                content.set_type(dpp::cot_text_display);
                content.set_content(std::format(
                    "**{}** wanted to play rock, paper, scissors!\n"
                    "-# This game expired - create a new one using `/rps`",
                    rps_game.player.username));

                dpp::component container;
                container.set_type(dpp::cot_container);
                container.add_component_v2(content);

                dpp::message msg;
                msg.set_flags(dpp::m_using_components_v2);
                msg.add_component_v2(container);
                rps_game.form_event.edit_original_response(msg);
            }
        }
    public:
        fun_module() : module("fun") {}
    };
    static fun_module instance;
}