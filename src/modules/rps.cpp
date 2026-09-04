#include <discofloor/bot.h>
#include <discofloor/utility.h>
#include <discofloor/timed_interaction.h>

#include <bulbtils/string.h>

namespace discofloor
{
    namespace rps
    {
        struct choice_data
        {
            std::string name;
            std::string emoji;
        };

        static const inline std::vector<choice_data> choices
        {
            { "", "" },
            { "Rock", "🪨" },
            { "Paper", "📄" },
            { "Scissors", "✂️" },
        };

        struct choice
        {
            enum value_t : uint8_t
            {
                rps_none = 0,
                rps_rock,
                rps_paper,
                rps_scissors,
            };
            value_t value;

            choice(value_t value = rps_none) : value(value) {}
            choice(uint8_t value) : value(static_cast<value_t>(value)) {}

            operator uint8_t() { return static_cast<uint8_t>(value); }

            bool operator>(const choice& other) const
            {
                switch (value)
                {
                    case rps_rock: return other.value == rps_scissors;
                    case rps_paper: return other.value == rps_rock;
                    case rps_scissors: return other.value == rps_paper;
                }
                return false;
            }

            bool operator==(const choice& other) const
            {
                return value == other.value;
            }
        };

        class game
        {
            // incrementing counter for rps game ids
            static inline uint32_t counter_ = 0;

            // this rps game's id, used to match button/form ids
            uint32_t id_;

            // the mention of the host player
            std::string player_;

            // the mention of the rps command
            std::string command_;

            // what the host picked
            choice choice_;

            // timed interaction of the original response
            timed_interaction response_;

            void on_timeout(const timed_interaction& response)
            {
                dpp::component content;
                content.set_type(dpp::cot_text_display);
                content.set_content(std::format(
                    "**{}** wanted to play rock, paper, scissors!\n"
                    "-# This game expired - create a new one using {}",
                    player_, command_));

                dpp::component container;
                container.set_type(dpp::cot_container);
                container.add_component_v2(content);

                dpp::message msg;
                msg.set_flags(dpp::m_using_components_v2);
                msg.add_component_v2(container);

                response.edit(msg);
            }
        public:
            game(uint32_t id, const std::string& player, const std::string& command, choice choice, const dpp::form_submit_t& event)
                : id_(id), player_(player), command_(command), choice_(choice), response_(event, [this](const timed_interaction& response) { on_timeout(response); }) {}

            void edit_response(const dpp::message& new_msg) { response_.edit(new_msg); }

            // component custom id for the given rps game (id)
            std::string custom_id(const std::string& prefix) const { return prefix + "_" + std::to_string(id_); }

            // these two funcs decide the id of an rps_game
            static std::string new_modal() { return "rps_modal_" + std::to_string(counter_++); }
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

            auto player() { return player_; }
            auto choice() { return choice_; }
        };
    }

    class rps_module : public module
    {
        std::vector<rps::game> games;

        std::string rps_mention = "`/rps`";
        void set_rps_mention_if_unset(bot* cluster)
        {
            if (rps_mention == "`/rps`")
            {
                rps_mention = cluster->get_command("rps", dpp::ctxm_chat_input).value().get_mention();
            }
        }

        dpp::task<void> run_rps(const run_event& event)
        {
            if (auto message_command = event.get_message_command())
            {
                // required for this chat command
                set_rps_mention_if_unset(event.get_bot());

                // can't prompt dialogs via old-style chat commands
                message_command->reply(":x: **| You can only create RPS games using " + rps_mention + "**");
                co_return;
            }

            dpp::component select_rps;
            select_rps.set_id("select_rps");
            select_rps.set_type(dpp::cot_selectmenu);
            select_rps.set_label("What will you play?");
            select_rps.set_placeholder("Pick between Rock, Paper or Scissors");
            for (uint8_t i = 1; i < rps::choices.size(); i++)
            {
                auto& choice = rps::choices[i];
                select_rps.add_select_option(dpp::select_option(choice.name, choice.name).set_emoji(choice.emoji));
            }
            event.get_slash_command()->dialog(dpp::interaction_modal_response(rps::game::new_modal(), "Rock, Paper, Scissors", { select_rps }));
        }

        dpp::event_handle form_submit_handle = SIZE_MAX;
        dpp::task<void> form_submit_event(const dpp::form_submit_t& event)
        {
            if (!event.custom_id.starts_with("rps_modal_"))
            {
                // not an error, it's just not an rps game, so do nothing
                co_return;
            }

            auto chosen = std::get<std::string>(event.components[0].value);
            uint8_t choice_value;

            for (uint8_t i = 1; i < rps::choices.size(); i++)
            {
                auto& choice = rps::choices[i];
                if (choice.name == chosen)
                {
                    choice_value = i;
                    break;
                }
            }

            auto host_player = event.command.usr.get_mention();

            // required for the expiry message for this rps game
            set_rps_mention_if_unset(static_cast<bot*>(event.owner));

            auto& rps = games.emplace_back(rps::game::id_from_modal(event.custom_id), host_player, rps_mention, choice_value, event);

            dpp::component container;
            container.set_type(dpp::cot_container);

            dpp::component content;
            content.set_type(dpp::cot_text_display);
            content.set_content(std::format(
                "**{}** wants to play rock, paper, scissors!\n"
                "-# They've already selected an option, choose your response:",
                host_player));

            // buttons need to be added to an action row first
            // they can't be added directly to the container
            // afterwards we add the whole action row to the container
            dpp::component actions;
            actions.set_type(dpp::cot_action_row);

            for (uint8_t i = 1; i < rps::choices.size(); i++)
            {
                auto& choice = rps::choices[i];

                dpp::component button;
                button.set_type(dpp::cot_button);
                button.set_label(choice.name);
                button.set_emoji(choice.emoji);
                button.set_style(dpp::cos_secondary);
                button.set_id(rps.custom_id(choice.name));

                actions.add_component_v2(button);
            }

            dpp::component content_footer;
            content_footer.set_type(dpp::cot_text_display);
            content_footer.set_content("-# Game expires at " + dpp::utility::timestamp(std::time(nullptr) + timed_interaction::lifespan_seconds, dpp::utility::tf_short_time));

            container.add_component_v2(content);
            container.add_component_v2(actions);
            container.add_component_v2(content_footer);

            dpp::message msg;
            msg.set_flags(dpp::m_using_components_v2);
            msg.add_component_v2(container);

            event.reply(msg);
        }

        dpp::event_handle button_click_handle = SIZE_MAX;
        dpp::task<void> button_click_event(const dpp::button_click_t& event)
        {
            for (uint8_t i = 1; i < rps::choices.size(); i++)
            {
                if (event.custom_id.starts_with(rps::choices[i].name + "_"))
                {
                    goto find_game;
                }
            }

            // not an error, it's just not an rps game, so do nothing
            co_return;

            find_game:

            rps::choice opp_choice;
            auto rps = std::find_if(games.begin(), games.end(), [&choices = rps::choices, &opp_choice, custom_id = event.custom_id](const rps::game& it)
            {
                for (uint8_t i = 1; i < choices.size(); i++)
                {
                    if (it.custom_id(choices[i].name) == custom_id)
                    {
                        opp_choice = i;
                        return true;
                    }
                }
                return false;
            });

            // required for post-game
            set_rps_mention_if_unset(static_cast<bot*>(event.owner));

            if (rps == games.end())
            {
                event.reply(discofloor::container_msg("This game is invalid - create a new one using " + rps_mention, 0xFF0000));
                co_return;
            }

            auto host_player = rps->player();
            auto opp_player = event.command.usr.get_mention();

            if (host_player == opp_player)
            {
                event.reply(discofloor::container_msg(std::format("**{}**, you can't play against yourself!", host_player), 0xFF0000));
                co_return;
            }

            dpp::component container;
            container.set_type(dpp::cot_container);

            dpp::component title;
            title.set_type(dpp::cot_text_display);
            title.set_content(std::format(
                "### {} {} :vs: {} {}",
                host_player, rps::choices[rps->choice()].emoji,
                rps::choices[opp_choice].emoji, opp_player));
            
            dpp::component separator;
            separator.set_type(dpp::cot_separator);
            separator.set_spacing(dpp::sep_small);
            separator.set_divider(true);

            dpp::component content;
            content.set_type(dpp::cot_text_display);

            std::string motivator = "\n-# Create a new game using " + rps_mention;

            if (rps->choice() == opp_choice)
            {
                content.set_content("It's a draw!" + motivator);
            }
            else if (rps->choice() > opp_choice)
            {
                content.set_content(std::format("**{}** wins!{}", host_player, motivator));
            }
            else
            {
                content.set_content(std::format("**{}** wins!{}", opp_player, motivator));
            }

            container.add_component_v2(title);
            container.add_component_v2(separator);
            container.add_component_v2(content);

            dpp::message msg;
            msg.set_flags(dpp::m_using_components_v2);
            msg.add_component_v2(container);

            rps->edit_response(msg);
            event.reply(discofloor::container_msg(std::format(
                "**{}** and **{}** played rock, paper, scissors!\n"
                "-# Click the reply to see the results, or create a new game using {}",
                host_player, opp_player, rps_mention)));
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command rps("rps", "Initiate a 'Rock, Paper, Scissors' game", bot.me.id,
                [this](const auto& event) -> dpp::task<void> { co_await run_rps(event); });

            return { rps };
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
            games.clear();
        }
    public:
        rps_module() : module("rps") {}
    };
    static rps_module instance;
}