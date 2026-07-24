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
            if (request.error == dpp::h_success)
            {
                nlohmann::json data = {};
                try
                {
                    data = nlohmann::json::parse(request.body);
                }
                catch (const std::exception& e)
                {
                    fixedphilip::log::error(std::format("Exception parsing currency conversion json file: {}", e.what()));
                    goto exit_fetch_currency_json;
                }
                if (data.empty())
                {
                    fixedphilip::log::error("Currency conversion json file is empty");
                    goto exit_fetch_currency_json;
                }

                std::string family_name = "";
                try
                {
                    std::string date = data.begin().value().at("date");
                    family_name = std::format("Currency ({})", date);
                }
                catch (const std::exception& e)
                {
                    fixedphilip::log::error(std::format("Exception reading date from currency conversion json file: {}", e.what()));
                    goto exit_fetch_currency_json;
                }
                if (family_name.empty())
                {
                    fixedphilip::log::error("Currency conversion family name is empty");
                    goto exit_fetch_currency_json;
                }

                struct conversion_currency : public fixedphilip::math::conversion_unit
                {
                    conversion_currency(std::vector<std::string> currency_aliases, fixedphilip::math::number_t currency_to_one_usd, fixedphilip::math::number_t one_usd_to_currency)
                    {
                        aliases = currency_aliases;

                        unit_to_base = [currency_to_one_usd](fixedphilip::math::number_t currency)
                        {
                            return currency * currency_to_one_usd;
                        };
                        base_to_unit = [one_usd_to_currency](fixedphilip::math::number_t usd)
                        {
                            return usd * one_usd_to_currency;
                        };
                    }
                };

                fixedphilip::math::conversion_family currency_conversion_family
                {
                    family_name,
                    {
                        conversion_currency {{ "U.S. Dollar", "usd" }, 1, 1 },
                    }
                };

                for (auto& [currency_key, currency_info] : data.items())
                {
                    std::string name = "";
                    try
                    {
                        name = currency_info.at("code");
                        fixedphilip::utils::string::inplace::to_lowercase(name);
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception reading 'code' from currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (name.empty())
                    {
                        fixedphilip::log::error(std::format("'code' from currency key '{}' is empty", currency_key));
                        continue;
                    }

                    std::string pretty_print = "";
                    try
                    {
                        pretty_print = currency_info.at("name");
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception reading 'name' from currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (pretty_print.empty())
                    {
                        fixedphilip::log::error(std::format("'name' from currency key '{}' is empty", currency_key));
                        continue;
                    }

                    std::string currency_to_usd_str = "";
                    try
                    {
                        currency_to_usd_str = currency_info.at("inverseRate");
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception reading 'inverseRate' from currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (currency_to_usd_str.empty())
                    {
                        fixedphilip::log::error(std::format("'inverseRate' from currency key '{}' is empty", currency_key));
                        continue;
                    }

                    fixedphilip::math::number_t currency_to_usd = -1;
                    try
                    {
                        currency_to_usd = fixedphilip::math::string_to_number(currency_to_usd_str);
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception parsing 'currency_to_usd' for currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (currency_to_usd < -1)
                    {
                        fixedphilip::log::error(std::format("'currency_to_usd' for currency key '{}' is negative", currency_key));
                        continue;
                    }


                    std::string usd_to_currency_str = "";
                    try
                    {
                        usd_to_currency_str = currency_info.at("rate");
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception reading 'rate' from currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (usd_to_currency_str.empty())
                    {
                        fixedphilip::log::error(std::format("'rate' from currency key '{}' is empty", currency_key));
                        continue;
                    }

                    fixedphilip::math::number_t usd_to_currency = -1;
                    try
                    {
                        usd_to_currency = fixedphilip::math::string_to_number(usd_to_currency_str);
                    }
                    catch (const std::exception& e)
                    {
                        fixedphilip::log::error(std::format("Exception parsing 'usd_to_currency' for currency key '{}': {}", currency_key, e.what()));
                        continue;
                    }
                    if (usd_to_currency < -1)
                    {
                        fixedphilip::log::error(std::format("'usd_to_currency' for currency key '{}' is negative", currency_key));
                        continue;
                    }

                    currency_conversion_family.units.push_back(conversion_currency{ { pretty_print, name }, currency_to_usd, usd_to_currency });
                }

                std::erase_if(fixedphilip::math::conversion_families, [](const auto& item) { return item.name.starts_with("Currency"); });
                fixedphilip::math::conversion_families.push_back(currency_conversion_family);

                // the website claims these update daily, then proceeds to specify 12 am/pm (every 12 hours???)
                // ...when in reality, apparently, these "daily" values tend to update every hour lol
                next_call = std::chrono::minutes(30);
            }
            else
            {
                next_call = std::chrono::minutes(1);
            }
        }

        exit_fetch_currency_json:

        std::string response;
        if (auto slash_command = event.get_slash_command())
        {
            auto value = std::get<std::string>(slash_command->get_parameter("value"));
            auto to = std::get<std::string>(slash_command->get_parameter("to"));
            auto decimals = fixedphilip::discord::try_get_command_parameter<int64_t>(*slash_command, "decimals", -1);
            auto separate = fixedphilip::discord::try_get_command_parameter<bool>(*slash_command, "separate", false);
            try
            {
                response = fixedphilip::math::convert(value, to, decimals, separate);
            }
            catch (std::runtime_error& e)
            {
                response = std::format(
                    ":x: **| Error:** {}\n"
                    "> -# **Common reasons:**\n"
                    "> -# \\- Missing space between number and unit, eg. `15min`\n"
                    "> -# \\- Incompatible unit(s), eg. `15min to km` or `15min 30in to (...)`\n"
                    "> -# \\- Duplicate 'to' units, eg. `15 min to sec sec`\n"
                    "> -# \\- Unknown or missing unit(s), eg. `15 to sec` or `15 min to blabla`", 
                    e.what());
            }
        }

        co_await thinking;
        event.thinking_end(response);
    }
}

FIXEDPHILIP_COMMAND(convert, "Unit/currency conversion");