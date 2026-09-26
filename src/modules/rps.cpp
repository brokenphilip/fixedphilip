#include <discofloor/bot.h>
#include <discofloor/utility.h>
#include <discofloor/timed_interaction.h>

#include <bulbtils/string.h>

namespace discofloor
{
    namespace rps
    {
        // these are the possible choices an rps game can have
        enum choice : uint8_t
        {
            c_none = 0, // default value, undecided
            c_first = 1, // first valid value

            c_rock = 1,
            c_paper,
            c_scissors,

            c_count, // not a valid choice - just total value count, including none
        };

        // required data for an rps choice - name ie. value and its respective emoji
        struct choice_datum
        {
            std::string name;
            std::string emoji;
        };

        // fixed choice data according to the enum (choices.size() <=> c_count)
        static const inline std::vector<choice_datum> choice_data
        {
            { "", "" },
            { "Rock", "🪨" },
            { "Paper", "📄" },
            { "Scissors", "✂️" },
        };

        // defines a player's choice, or rather a player AND their choice
        class player_choice
        {
            std::string player_;
            choice choice_;
        public:
            player_choice(const std::string& player, choice choice) : player_(player), choice_(choice) {}

            auto player() const { return player_; }
            auto choice() const { return choice_; }

            bool operator>(const player_choice& other) const
            {
                if (choice_ == c_none || other.choice_ == c_none)
                {
                    throw std::runtime_error("Undeterminable outcome");
                }
                switch (choice_)
                {
                    case c_rock: return other.choice_ == c_scissors;
                    case c_paper: return other.choice_ == c_rock;
                    case c_scissors: return other.choice_ == c_paper;
                }
                return false;
            }

            bool operator==(const player_choice& other) const
            {
                if (choice_ == c_none || other.choice_ == c_none)
                {
                    throw std::runtime_error("Undeterminable outcome");
                }
                return choice_ == other.choice_;
            }
        };

        // managed RPS game - (host) player choice and (timed) interaction ie. message/buttons
        class game
        {
            player_choice host_choice_;
            timed_interaction game_interaction_;

            // next available rps game id
            // cannot use 0 because bot restarts will make old games invalid
            // cannot use std::time(nullptr) as id directly in case 2 games start at once
            static inline uint64_t next_id_ = std::time(nullptr);

            // this rps game's id, used to match button/form ids
            uint64_t id_;

            // the slashcommand mention for creating rps games
            std::string command_;

            static constexpr int lifespan = timed_interaction::recommended_lifespan_seconds;
            static inline const std::string modal_prefix = "rps_modal_";

            // can throw exception but never will
            static uint64_t id_from_modal(const std::string& modal)
            {
                auto id = modal;
                bulbtils::string::inplace::replace_all(id, modal_prefix, "");
                return std::stoull(id);
            }

            void on_timeout()
            {
                dpp::component content;
                content.set_type(dpp::cot_text_display);
                content.set_content(std::format(
                    "**{}** wanted to play rock, paper, scissors!\n"
                    "-# This game expired - create a new one using {}",
                    host_choice_.player(), command_));

                dpp::component container;
                container.set_type(dpp::cot_container);
                container.add_component_v2(content);

                dpp::message msg;
                msg.set_flags(dpp::m_using_components_v2);
                msg.add_component_v2(container);

                game_interaction_.edit(msg);
            }
        public:
            // generate a new modal custom ID for a potential RPS game (if the user doesn't cancel the form)
            static std::string generate_new_modal_id() { return modal_prefix + std::to_string(next_id_++); }

            // generate a button custom ID for this RPS game (id)
            std::string generate_button_id(const std::string& prefix) const { return prefix + "_" + std::to_string(id_); }

            // checks the custom ID of a form event to see if it belongs to an RPS game
            static auto is_rps_form(const std::string& event_id) { return event_id.starts_with(modal_prefix); }

            // get the host player
            auto player() const { return host_choice_.player(); }

            // is this rps game completed (resolved or timed out)
            auto completed() const { return game_interaction_.timed_out(); }

            // create a new rps game given the (host) player and choice, creating the game interaction in the process
            game(const std::string& player, rps::choice choice, const std::string& modal, const std::string& command, const dpp::form_submit_t& event)
                : host_choice_(player, choice),
                id_(id_from_modal(modal)),
                command_(command),
                game_interaction_(event, lifespan, [this](timed_interaction& response) { on_timeout(); })
            {
                dpp::component container;
                container.set_type(dpp::cot_container);

                dpp::component content;
                content.set_type(dpp::cot_text_display);
                content.set_content(std::format(
                    "**{}** wants to play rock, paper, scissors!\n"
                    "-# They've already selected an option, choose your response:",
                    player));

                // buttons need to be added to an action row first
                // they can't be added directly to the container
                // afterwards we add the whole action row to the container
                dpp::component actions;
                actions.set_type(dpp::cot_action_row);

                for (uint8_t i = 1; i < c_count; i++)
                {
                    auto& choice_datum = choice_data[i];

                    dpp::component button;
                    button.set_type(dpp::cot_button);
                    button.set_label(choice_datum.name);
                    button.set_emoji(choice_datum.emoji);
                    button.set_style(dpp::cos_secondary);
                    button.set_id(generate_button_id(choice_data[i].name));

                    actions.add_component_v2(button);
                }

                dpp::component content_footer;
                content_footer.set_type(dpp::cot_text_display);
                content_footer.set_content("-# Game expires at " + dpp::utility::timestamp(std::time(nullptr) + lifespan, dpp::utility::tf_short_time));

                container.add_component_v2(content);
                container.add_component_v2(actions);
                container.add_component_v2(content_footer);

                dpp::message msg;
                msg.set_flags(dpp::m_using_components_v2);
                msg.add_component_v2(container);

                event.reply(msg);
            }

            // we want to force the interaction to time out before our destructor finishes
            // if we let the game interaction dtor do it, we already lost access to rps game data
            ~game()
            {
                game_interaction_.force_timeout(false);
            }

            // resolve an active rps game by comparing the choices of opponent vs host
            // returns false if timed out, true otherwise
            bool resolve(const player_choice& opponent_choice)
            {
                auto host_player = host_choice_.player();
                auto opp_player = opponent_choice.player();

                dpp::component container;
                container.set_type(dpp::cot_container);

                dpp::component title;
                title.set_type(dpp::cot_text_display);
                title.set_content(std::format(
                    "### {} {} :vs: {} {}",
                    host_player,
                    choice_data[host_choice_.choice()].emoji,
                    choice_data[opponent_choice.choice()].emoji,
                    opp_player));

                dpp::component separator;
                separator.set_type(dpp::cot_separator);
                separator.set_spacing(dpp::sep_small);
                separator.set_divider(true);

                dpp::component content;
                content.set_type(dpp::cot_text_display);

                std::string motivator = "\n-# Create a new game using " + command_;

                if (host_choice_ == opponent_choice)
                {
                    content.set_content("It's a draw!" + motivator);
                }
                else if (host_choice_ > opponent_choice)
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

                try
                {
                    game_interaction_.edit(msg);
                    game_interaction_.force_timeout(true);
                }
                catch (timed_interaction::timed_out_error& e)
                {
                    return false;
                }
                return true;
            }
        };

        class game_manager
        {
            mutable std::shared_mutex mutex_;
            std::vector<std::unique_ptr<game>> games_;
        public:
            void start_new_game_from_form_event(const dpp::form_submit_t& event, const std::string& command)
            {
                if (!rps::game::is_rps_form(event.custom_id))
                {
                    // not an error, it's just not an rps game, so do nothing
                    return;
                }

                auto choice_str = std::get<std::string>(event.components[0].value);
                rps::choice choice;
                for (uint8_t i = 1; i < rps::c_count; i++)
                {
                    auto& choice_datum = rps::choice_data[i];
                    if (choice_datum.name == choice_str)
                    {
                        choice = static_cast<rps::choice>(i);
                        break;
                    }
                }
                auto player = event.command.usr.get_mention();

                std::unique_lock _(mutex_);
                games_.push_back(std::make_unique<game>(player, choice, event.custom_id, command, event));
            }

            void resolve_game_using_button_event(const dpp::button_click_t& event, const std::string& command)
            {
                rps::choice opp_choice = c_none;
                for (uint8_t i = 1; i < rps::c_count; i++)
                {
                    if (event.custom_id.starts_with(rps::choice_data[i].name + "_"))
                    {
                        opp_choice = static_cast<choice>(i);
                        break;
                    }
                }
                if (opp_choice == c_none)
                {
                    // not an error, it's just not an rps game, so do nothing
                    return;
                }

                std::shared_lock _(mutex_);
                auto game_ptr_it = std::find_if(games_.begin(), games_.end(), [&opp_choice, custom_id = event.custom_id](const std::unique_ptr<rps::game>& it)
                {
                    if (it.get()->generate_button_id(choice_data[opp_choice].name) == custom_id)
                    {
                        return true;
                    }
                    return false;
                });
                auto opp_player = event.command.usr.get_mention();

                // it's possible that the bot crashed and didn't get to hide the buttons (which it can't do post-mortem)
                if (game_ptr_it == games_.end())
                {
                    game_invalid:
                    event.reply(":x: **| " + opp_player + ", this game is invalid - create a new one using " + command + "**");
                    return;
                }
                auto game = game_ptr_it->get();

                auto host_player = game->player();
                if (host_player == opp_player)
                {
                    event.reply(":x: **| " + host_player + ", you can't play against yourself!**");
                    return;
                }

                player_choice opponent(opp_player, opp_choice);
                if (game->resolve(opponent))
                {
                    event.reply(discofloor::container_msg(std::format(
                        "**{}** and **{}** played rock, paper, scissors!\n"
                        "-# Click the reply to see the results, or create a new game using {}",
                        host_player, opp_player, command)));
                    return;
                }
                goto game_invalid;
            }

            void remove_completed_games()
            {
                std::unique_lock _(mutex_);
                std::erase_if(games_, [](const std::unique_ptr<game>& game)
                {
                    return game.get()->completed();
                });
            }

            void invalidate_all_active_games()
            {
                std::unique_lock _(mutex_);
                games_.clear();
            }
        };
    }

    class rps_module : public bot_module
    {
        rps::game_manager game_manager;
        std::string rps_command_text = "`/rps`";

        dpp::event_handle form_submit_handle = SIZE_MAX;
        dpp::event_handle button_click_handle = SIZE_MAX;
        dpp::timer games_timer_handle = 0;

        dpp::task<void> run_rps(const run_event& event)
        {
            if (auto message_command = event.get_message_command())
            {
                // can't prompt dialogs via old-style chat commands
                message_command->reply(":x: **| You can only create RPS games using " + rps_command_text + "**");
                co_return;
            }

            dpp::component select_rps;
            select_rps.set_id("select_rps");
            select_rps.set_type(dpp::cot_selectmenu);
            select_rps.set_label("What will you play?");
            select_rps.set_placeholder("Pick between Rock, Paper or Scissors");
            for (uint8_t i = 1; i < rps::c_count; i++)
            {
                auto& choice_datum = rps::choice_data[i];
                select_rps.add_select_option(dpp::select_option(choice_datum.name, choice_datum.name).set_emoji(choice_datum.emoji));
            }
            event.get_slash_command()->dialog(dpp::interaction_modal_response(rps::game::generate_new_modal_id(), "Rock, Paper, Scissors", { select_rps }));
        }

        virtual std::vector<bot_command> commands(bot& bot) override final
        {
            bot_command rps("rps", "Initiate a 'Rock, Paper, Scissors' game", bot.me.id,
                [this](const auto& event) -> dpp::task<void> { co_await run_rps(event); });

            return { rps };
        }

        virtual void commands_created(bot& bot) override final
        {
            rps_command_text = bot.get_command("rps", dpp::ctxm_chat_input).value().get_mention();
        }

        virtual bool init(bot& bot) override final
        {
            form_submit_handle = bot.on_form_submit.attach([this](const auto& event) { game_manager.start_new_game_from_form_event(event, rps_command_text); });
            button_click_handle = bot.on_button_click.attach([this](const auto& event) { game_manager.resolve_game_using_button_event(event, rps_command_text); });
            games_timer_handle = bot.start_timer([this](const dpp::timer& timer) { game_manager.remove_completed_games(); }, 60);
            return true;
        }

        virtual void destroy(bot& bot) override final
        {
            bot.stop_timer(games_timer_handle);
            bot.on_button_click.detach(button_click_handle);
            bot.on_form_submit.detach(form_submit_handle);
            game_manager.invalidate_all_active_games();
        }
    public:
        rps_module() : bot_module("rps") {}
    };
    static rps_module instance;
}