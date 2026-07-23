#include <fixedphilip/command.h>

#include <tinyexpr/tinyexpr.h>

namespace fixedphilip::commands::calcluate
{
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        command
            .add_option(dpp::command_option(dpp::co_string, "expression", "Math expression to calculate", true))
            .add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (no rounding by default)"))
            .add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate result digit thousands (all digits are grouped by default)"));
        co_return true;
    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        if (auto message_create = event.get_message_create())
        {
            event.reply_not_impl_use_other();
            co_return;
        }

        auto thinking = event.co_thinking_start();

        std::string response;
        if (auto slash_command = event.get_slash_command())
        {
            auto interaction = slash_command->command.get_command_interaction();
            auto expression = interaction.get_value<std::string>(0);
            int decimals = -1;
            bool separate = false;

            for (int i = 1; i < interaction.options.size(); i++)
            {
                auto subcommand = interaction.options[i];
                if (subcommand.name == "decimals")
                {
                    decimals = static_cast<int>(interaction.get_value<int64_t>(i));
                }
                else if (subcommand.name == "separate")
                {
                    separate = interaction.get_value<bool>(i);
                }
            }

            int error = 0;
            auto result = te_interp(expression.c_str(), &error);
            if (error)
            {
                std::string arrow(error, ' ');
                arrow += "↑";

                co_await thinking;
                event.thinking_end(std::format(":x: **| Error parsing expression:**\n```\n{}\n{}\n```", expression, arrow));
            }
            else
            {
                std::string result_str;
                if (decimals < 0)
                {
                    result_str = std::format("{}", result);
                }
                else
                {
                    result_str = std::format("{:.{}f}", result, decimals);
                }
                co_await thinking;
                event.thinking_end(std::format("### :abacus: **| Result:**\n> {} = **{}**", expression, result_str));
            }
        }
    }
}

FIXEDPHILIP_COMMAND(calcluate, "Calculate a math expression");