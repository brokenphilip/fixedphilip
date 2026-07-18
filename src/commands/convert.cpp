#include <fixedphilip/command.h>
#include <fixedphilip/utils/time.h>
#include <fixedphilip/utils/string.h>

namespace fixedphilip::commands::convert
{
    using number_t = double;
    number_t number_from_string(const std::string& string)
    {
        return std::stod(string);
    }
    using conversion_function = std::function<number_t(number_t)>;

    struct conversion_unit
    {
        std::string unit_name;
        std::string unit_pretty_print;
        conversion_function from_unit_to_base_unit = [](number_t self) { return self; };
        conversion_function to_unit_from_base_unit = [](number_t self) { return self; };
    };

    struct conversion_currency : public conversion_unit
    {
        conversion_currency(std::string name, std::string pretty_print, number_t currency_to_one_usd, number_t one_usd_to_currency)
        {
            unit_name = name;
            unit_pretty_print = pretty_print;

            from_unit_to_base_unit = [currency_to_one_usd](number_t currency)
            {
                return currency * currency_to_one_usd;
            };
            to_unit_from_base_unit = [one_usd_to_currency](number_t usd)
            {
                return usd * one_usd_to_currency;
            };
        }
    };

    struct conversion_family
    {
        std::string unit_family_name;
        std::vector<conversion_unit> family_units;
    };

    template <number_t other>
    number_t add(number_t number)
    {
        return number + other;
    }

    template <number_t other>
    number_t subtract(number_t number)
    {
        return number - other;
    }

    template <number_t other>
    number_t multiply(number_t number)
    {
        return number * other;
    }

    template <number_t other>
    number_t divide(number_t number)
    {
        return number / other;
    }

    number_t celsius_to_fahrenheit(number_t celsius)
    {
        return celsius * number_t(9) / number_t(5) + number_t(32);
    }

    number_t fahrenheit_to_celsius(number_t fahrenheit)
    {
        return (fahrenheit - number_t(32)) * number_t(5) / number_t(9);
    }

    std::vector<conversion_family> conversion_families =
    {
        { "Temperature",
            {
                { "celsius", "°C" },
                { "c", "°C" },
                { "fahrenheit", "°F", fahrenheit_to_celsius, celsius_to_fahrenheit },
                { "f", "°F", fahrenheit_to_celsius, celsius_to_fahrenheit },
                { "kelvin", "K", subtract<273.15>, add<273.15> },
                { "k", "K", subtract<273.15>, add<273.15> },
            }
        },
        { "Speed",
            {
                { "km/h", "km/h" },
                { "kmh", "km/h" },
                { "kph", "km/h" },
                { "kmph", "km/h" },
                { "mph", "mph", multiply<1.609>, divide<1.609> },
                { "mi/h", "mph", multiply<1.609>, divide<1.609> },
                { "m/s", "m/s", multiply<3.6>, divide<3.6> },
                { "mps", "m/s", multiply<3.6>, divide<3.6> },
            }
        },
    };

    std::string convert(number_t number, std::string from, std::string to, int precision)
    {
        auto to_lowercase = [](unsigned char c) { return std::tolower(c); };
        fixedphilip::utils::string::to_lowercase_inplace(from);
        fixedphilip::utils::string::to_lowercase_inplace(to);

        // check all conversion unit families
        for (auto& conversion_family : conversion_families)
        {
            // check all conversion units within this family
            for (auto& from_conversion_unit : conversion_family.family_units)
            {
                // did we find our "from" unit?
                if (from_conversion_unit.unit_name == from)
                {
                    // "from" unit found, proceed to find "to" unit from the same family
                    for (auto& to_conversion_unit : conversion_family.family_units)
                    {
                        // did we find our "to" unit?
                        if (to_conversion_unit.unit_name == to)
                        {
                            // perform the conversion and return the result
                            auto result = to_conversion_unit.to_unit_from_base_unit(from_conversion_unit.from_unit_to_base_unit(number));
                            return std::format("### :repeat: | {}:\n> {} {} = {:.{}f} {}",
                                conversion_family.unit_family_name,
                                number, from_conversion_unit.unit_pretty_print,
                                result, precision, to_conversion_unit.unit_pretty_print);
                        }
                        // not our "to" unit, proceed to next iteration
                    }
                    // we didn't find our "to" unit, maybe it's in a different family
                    // break out of this family and try another
                    break;
                }
                // not our "from" unit, proceed to next iteration
            }
            // we didn't find our "from" unit, maybe it's in a different family
        }
        // we didn't find our "from" unit anywhere!
        return std::format(":x: **| Error:** no suitable conversion found for `{}` -> `{}`", from, to);
    }

    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        command
            .add_option(dpp::command_option(dpp::co_number, "value", "Number to convert the units of", true))
            .add_option(dpp::command_option(dpp::co_string, "from", "Unit of the number to convert from", true))
            .add_option(dpp::command_option(dpp::co_string, "to", "Unit of the number to convert to", true))
            .add_option(dpp::command_option(dpp::co_integer, "decimals", "Number of decimals to display the result (2 by default)"));

        co_return true;
    }

    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        auto thinking = event.co_thinking_start();

        static auto next_call = std::chrono::minutes(1);
        if (fixedphilip::utils::time::run_if_passed<struct fetch_currency_json>(next_call))
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

                conversion_family currency_conversion_family 
                { 
                    family_name, 
                    {
                        conversion_currency { "usd", "U.S. Dollar", 1, 1 },
                    }
                };
                
                for (auto& [currency_key, currency_info] : data.items())
                {
                    std::string name = "";
                    try
                    {
                        name = currency_info.at("code");
                        fixedphilip::utils::string::to_lowercase_inplace(name);
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

                    number_t currency_to_usd = -1;
                    try
                    {
                        currency_to_usd = number_from_string(currency_to_usd_str);
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

                    number_t usd_to_currency = -1;
                    try
                    {
                        usd_to_currency = number_from_string(usd_to_currency_str);
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

                    currency_conversion_family.family_units.push_back(conversion_currency { name, pretty_print, currency_to_usd, usd_to_currency });
                }

                std::erase_if(conversion_families, [](const auto& item) { return item.unit_family_name.starts_with("Currency"); });
                conversion_families.push_back(currency_conversion_family);

                // apparently these "daily" values update every hour lol
                next_call = std::chrono::minutes(30);
                /*
                auto now = std::chrono::system_clock::now();
                auto time_of_day = std::chrono::hh_mm_ss(now - std::chrono::floor<std::chrono::days>(now));
                auto time_of_day_hours_am_pm = time_of_day.hours().count() % 12;          
                auto total_minutes_since_noon_or_midnight = std::chrono::minutes(time_of_day_hours_am_pm) + time_of_day.minutes();

                // next call times are at 12:05am and 12:05pm, when the "daily" values update
                auto next_call_time = std::chrono::minutes(60 * 12 + 5) - total_minutes_since_noon_or_midnight;
                fixedphilip::log::info(std::format("Updated {} currencies - next call time: {:%Hh %Mm}", 
                    currency_conversion_family.family_units.size(), next_call_time));
                next_call = next_call_time;
                */
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
            auto interaction = slash_command->command.get_command_interaction();
            auto value = interaction.get_value<double>(0);
            auto from = interaction.get_value<std::string>(1);
            auto to = interaction.get_value<std::string>(2);
            int decimals = 2;
            if (interaction.options.size() > 3)
            {
                auto new_decimals = static_cast<int>(interaction.get_value<int64_t>(3));
                if (new_decimals < 0)
                {
                    new_decimals = 0;
                }
                decimals = new_decimals;
            }
            response = convert(value, from, to, decimals);
        }
        else if (auto message_create = event.get_message_create())
        {
            response = "Not implemented, use /convert instead.";
        }

        co_await thinking;
        event.thinking_end(dpp::message(response));
    }
}

FIXEDPHILIP_COMMAND(convert, "Convert between two units");