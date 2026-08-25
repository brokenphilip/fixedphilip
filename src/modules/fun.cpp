#include <discofloor/bot.h>

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

        virtual std::vector<command> commands(bot& bot) override final
        {
            command coin("coin", "Flip a coin", bot.me.id, run_coin);
            command dice("dice", "Roll the dice", bot.me.id, run_dice);

            

            return { coin, dice };
        }
    public:
        fun_module() : module("fun") {}
    };
    static fun_module instance;
}