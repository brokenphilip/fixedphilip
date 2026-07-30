#include <fixedphilip/discord.h>
#include <fixedphilip/math.h>

namespace fixedphilip
{
	class math_module : public fixedphilip::discord::bot::module
	{
		virtual bool init(fixedphilip::discord::bot& bot) override final
		{
			return true;
		}

		static dpp::task<void> run_calculate(const fixedphilip::discord::bot::command::run_event& event)
		{
            auto cluster = event.get_bot();
            if (!cluster)
            {
                fixedphilip::log::error("run_calculate: bot was null");
                co_return;
            }
            if (auto message_create = event.get_message_create())
            {
                event.reply_not_impl_use_other();
                co_return;
            }

            auto thinking = event.co_thinking_start();

            if (auto slash_command = event.get_slash_command())
            {
                auto expression = std::get<std::string>(slash_command->get_parameter("expression"));
                auto decimals = event.try_get_command_parameter<int64_t>("decimals", 2);
                auto separate = event.try_get_command_parameter<bool>("separate", true);

                int error = 0;
                auto result = te_interp(expression.c_str(), &error);
                if (error)
                {
                    co_await thinking;
                    event.thinking_end(std::format(":x: **| Error parsing expression:**\n```\n{}\n{}\n```", expression, std::string(error, ' ') + "↑"));
                }
                else
                {
                    std::string result_str = fixedphilip::math::format_number(result, decimals, separate);

                    co_await thinking;
                    event.thinking_end(std::format("### :abacus: **| Result:**\n> {} = **{}**", dpp::utility::markdown_escape(expression, true), result_str));
                }
            }
		}

		static dpp::task<void> run_convert(const fixedphilip::discord::bot::command::run_event& event)
		{
            auto cluster = event.get_bot();
            if (!cluster)
            {
                fixedphilip::log::error("run_convert: bot was null");
                co_return;
            }
            if (auto message_create = event.get_message_create())
            {
                event.reply_not_impl_use_other();
                co_return;
            }

            auto thinking = event.co_thinking_start();

            static auto next_call = std::chrono::minutes(1);
            if (fixedphilip::utils::time::run_if_passed<struct fetch_currency_json>(next_call))
            {
                auto request = co_await cluster->co_request("https://www.floatrates.com/daily/usd.json", dpp::m_get);
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
                auto decimals = event.try_get_command_parameter<int64_t>("decimals", 2);
                auto separate = event.try_get_command_parameter<bool>("separate", true);
                try
                {
                    std::string result, family;
                    fixedphilip::math::conversion::convert(value, to, decimals, separate, &result, &family);
                    response = "### :repeat: | " + family + ":\n> " + dpp::utility::markdown_escape(result, true);
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

		virtual std::vector<fixedphilip::discord::bot::command> commands(fixedphilip::discord::bot& bot) override final
		{
			fixedphilip::discord::bot::command calculate("calculate", "Calculate a math expression", bot.me.id, run_calculate);

			calculate.add_option(dpp::command_option(dpp::co_string, "expression", "Math expression to calculate", true))
				.add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (2 by default, use -1 for automatic)")
					.set_min_value(-1)
					.set_max_value(std::numeric_limits<double>::max_digits10))
				.add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate the result's digits per thousands? (true by default)"));

			fixedphilip::discord::bot::command convert("convert", "Convert between units and currencies", bot.me.id, run_convert);
			convert.add_option(dpp::command_option(dpp::co_string, "value", "Number and units/currencies to convert from", true))
				.add_option(dpp::command_option(dpp::co_string, "to", "Units/currencies to convert the value to", true))
				.add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (2 by default, use -1 for automatic)")
					.set_min_value(-1)
					.set_max_value(std::numeric_limits<double>::max_digits10))
				.add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate the result's digits per thousands? (true by default)"));

			return { calculate, convert };
		}
	public:
		math_module() : fixedphilip::discord::bot::module("math", "Provides commands for calculation and unit/currency conversion") {}
	};
    static math_module instance;
}