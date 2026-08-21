#include <discofloor/bot.h>

#include <fixedphilip/math.h>

namespace discofloor
{
	class math_module : public module
	{
		static dpp::task<void> run_calculate(const run_event& event)
		{
            auto thinking = event.co_thinking_start();

            auto expression = event.get_cmd_required_param_value<std::string>("expression");
            auto decimals = event.get_cmd_optional_param_value<int64_t>("decimals", 2);
            auto separate = event.get_cmd_optional_param_value<bool>("separate", true);

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

		static dpp::task<void> run_convert(const run_event& event)
		{
            auto cluster = event.get_bot();
            auto thinking = event.co_thinking_start();

            static auto next_call = std::chrono::minutes(1);
            if (bulbtils::time::run_if_passed<struct fetch_currency_json>(next_call))
            {
                auto result = co_await cluster->co_request("https://www.floatrates.com/daily/usd.json", dpp::m_get);
                if (result.status != 200)
                {
                    cluster->log(dpp::ll_error, "Floatrates GET HTTP status " + std::to_string(result.status));
                    goto exit_update_currencies;
                }

                nlohmann::json data = {};
                try
                {
                    data = nlohmann::json::parse(result.body);
                }
                catch (const std::exception& e)
                {
                    cluster->log(dpp::ll_error, std::format("Exception parsing currency conversion json file: {}", e.what()));
                    goto exit_update_currencies;
                }
                if (data.empty())
                {
                    cluster->log(dpp::ll_error, "Currency conversion json file is empty");
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

            auto value = event.get_cmd_required_param_value<std::string>("value");
            auto to = event.get_cmd_required_param_value<std::string>("to");
            auto decimals = event.get_cmd_optional_param_value<int64_t>("decimals", 2);
            auto separate = event.get_cmd_optional_param_value<bool>("separate", true);
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
            
            co_await thinking;
            event.thinking_end(response);
		}

		virtual std::vector<command> commands(bot& bot) override final
		{
			command calculate("calculate", "Calculate a math expression", bot.me.id, run_calculate);

			calculate.add_option(dpp::command_option(dpp::co_string, "expression", "Math expression to calculate", true))
				.add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (2 by default, use -1 for automatic)")
					.set_min_value(-1)
					.set_max_value(std::numeric_limits<double>::max_digits10))
				.add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate the result's digits per thousands? (true by default)"));

			command convert("convert", "Convert between units or currencies", bot.me.id, run_convert);
			convert.add_option(dpp::command_option(dpp::co_string, "value", "Values (math expressions and units/currencies) to convert from", true))
				.add_option(dpp::command_option(dpp::co_string, "to", "Unit(s) or currency to convert the value to", true))
				.add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to round the result to (2 by default, use -1 for automatic)")
					.set_min_value(-1)
					.set_max_value(std::numeric_limits<double>::max_digits10))
				.add_option(dpp::command_option(dpp::co_boolean, "separate", "Separate the result's digits per thousands? (true by default)"));

			return { calculate, convert };
		}
	public:
		math_module() : module("math") {}
	};
    static math_module instance;
}