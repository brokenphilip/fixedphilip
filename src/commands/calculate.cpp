#include <fixedphilip/command.h>

#include <tinyexpr.h>

namespace fixedphilip::commands::calculate
{
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        command
            .add_option(dpp::command_option(dpp::co_string, "expression", "Math expression to calculate", true))
            .add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (use -1 (default) for automatic)")
                .set_min_value(-1)
                .set_max_value(std::numeric_limits<double>::max_digits10))
            .add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate the result's digits per thousands? (false by default)"));
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
            auto expression = std::get<std::string>(slash_command->get_parameter("expression"));
            auto decimals = fixedphilip::discord::try_get_command_parameter<int64_t>(*slash_command, "decimals", -1);
            auto separate = fixedphilip::discord::try_get_command_parameter<bool>(*slash_command, "separate", false);

            int error = 0;
            auto result = te_interp(expression.c_str(), &error);
            if (error)
            {
                // "error" indicated where the parsing error occurred
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
                event.thinking_end(std::format("### :abacus: **| Result:**\n> {} = **{}** (DEBUG: separate = {})", expression, result_str, separate));
            }
        }
    }
}

FIXEDPHILIP_COMMAND(calculate, "Calculate a math expression");