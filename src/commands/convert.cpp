#include <fixedphilip/command.h>

#include <fixedphilip/utils/time.h>

#include <fixedphilip/math.h>

namespace fixedphilip::commands::convert
{
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        command
            .add_option(dpp::command_option(dpp::co_string, "value", "Number and units/currencies to convert from", true))
            .add_option(dpp::command_option(dpp::co_string, "to", "Units/currencies to convert the value to", true))
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
            event.reply_not_impl_use_other(bot);
            co_return;
        }

        auto thinking = event.co_thinking_start();

        static auto next_call = std::chrono::minutes(1);
        if (false && fixedphilip::utils::time::run_if_passed<struct fetch_currency_json>(next_call))
        {
            auto request = co_await bot.cluster().co_request("https://www.floatrates.com/daily/usd.json", dpp::m_get);
            if (request.error != dpp::h_success)
            {
                fixedphilip::log::error("Floatrates GET HTTP status " + std::to_string(request.status));
                goto exit_update_currencies;
            }

            nlohmann::json data = {};
            try
            {
                data = nlohmann::json::parse(request.body);
            }
            catch (const std::exception& e)
            {
                fixedphilip::log::error(std::format("Exception parsing currency conversion json file: {}", e.what()));
                goto exit_update_currencies;
            }
            if (data.empty())
            {
                fixedphilip::log::error("Currency conversion json file is empty");
                goto exit_update_currencies;
            }

            if (fixedphilip::math::conversion::update_currencies(data))
            {
                // the website claims these update daily, then proceeds to specify 12 am/pm (every 12 hours???)
                // ...when in reality, apparently, these "daily" values tend to update every hour lol
                next_call = std::chrono::minutes(30);
            }
            else
            {
                next_call = std::chrono::minutes(1);
            }
        }

        exit_update_currencies:

        std::string response;
        if (auto slash_command = event.get_slash_command())
        {
            auto value = std::get<std::string>(slash_command->get_parameter("value"));
            auto to = std::get<std::string>(slash_command->get_parameter("to"));
            auto decimals = fixedphilip::discord::try_get_command_parameter<int64_t>(*slash_command, "decimals", -1);
            auto separate = fixedphilip::discord::try_get_command_parameter<bool>(*slash_command, "separate", false);
            try
            {
                std::string result, family;
                fixedphilip::math::conversion::convert(value, to, decimals, separate, &result, &family);
                response = "### :repeat: | " + family + ":\n> " + result;
            }
            catch (fixedphilip::math::conversion::error& e)
            {
                response = std::format(
                    ":x: **| Conversion error:** {}\n"
                    "> -# **Common reasons:**\n"
                    "> -# \\- Missing space between number and unit\n"
                    "> -# eg. `15min`\n"
                    "> -# \\- Incompatible units\n"
                    "> -# eg. `15 min to km` or `15 min 30 km/h`\n"
                    "> -# \\- Duplicate destination (\"to\") units\n"
                    "> -# eg. `15 min to sec sec`\n"
                    "> -# \\- Unknown or missing unit(s)\n"
                    "> -# eg. `15 to sec` or `15 min to blabla`",
                    e.what());
            }
            catch (std::exception& e)
            {
                response = std::format(":x: **| General error:** {}", e.what());
            }
        }

        co_await thinking;
        event.thinking_end(response);
    }
}

FIXEDPHILIP_COMMAND(convert, "Unit/currency conversion");