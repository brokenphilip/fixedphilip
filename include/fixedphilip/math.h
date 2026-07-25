#pragma once

#include <fixedphilip/utils/string.h>

#include <dpp/coro/task.h>
#include <dpp/nlohmann/json.hpp>

#include <functional>
#include <locale>

namespace fixedphilip::math
{
	using number_t = double;
	using conversion_fn = std::function<number_t(number_t)>;
    inline number_t string_to_number(const std::string& str) { return std::stod(str); }

    struct thousands_separator : std::numpunct<char>
    {
        inline char do_thousands_sep() const override { return ' '; }
        inline std::string do_grouping() const override { return "\3"; }

        static std::string format_number(number_t number, int decimals = -1);
    };

    std::string format_number(number_t number, int decimals = -1, bool separate = false);

	// (string ->) unit -> base -> unit (-> string)
	using string_to_unit_fn = std::function<number_t(const std::string& str)>;
	using unit_to_base_fn = conversion_fn;
	using base_to_unit_fn = conversion_fn;
	using unit_to_string_fn = std::function<std::string(number_t, int, bool)>;

	template <number_t other>
	number_t add(number_t number)
	{
		return number + number_t(other);
	}

	template <number_t other>
	number_t subtract(number_t number)
	{
		return number - number_t(other);
	}

	template <number_t other>
	number_t multiply(number_t number)
	{
		return number * number_t(other);
	}

	template <number_t other>
	number_t divide(number_t number)
	{
		return number / number_t(other);
	}

    // 10^-9: nano
    inline conversion_fn nano_to_base = divide<1'000'000'000.0>;
    inline conversion_fn base_to_nano = multiply<1'000'000'000.0>;

    // 10^-6: micro
    inline conversion_fn micro_to_base = divide<1'000'000.0>;
    inline conversion_fn base_to_micro = multiply<1'000'000.0>;

    // 10^-3: milli
    inline conversion_fn milli_to_base = divide<1'000.0>;
    inline conversion_fn base_to_milli = multiply<1'000.0>;

    // 10^-2: centi
    inline conversion_fn centi_to_base = divide<100.0>;
    inline conversion_fn base_to_centi = multiply<100.0>;

    // 10^-1: deci
    inline conversion_fn deci_to_base = divide<10.0>;
    inline conversion_fn base_to_deci = multiply<10.0>;

    // 10^0 (1): base (default)

    // 10^3: kilo
    inline conversion_fn kilo_to_base = multiply<1'000.0>;
    inline conversion_fn base_to_kilo = divide<1'000.0>;

    // 10^6: mega
    inline conversion_fn mega_to_base = multiply<1'000'000.0>;
    inline conversion_fn base_to_mega = divide<1'000'000.0>;

    // 10^9: giga
    inline conversion_fn giga_to_base = multiply<1'000'000'000.0>;
    inline conversion_fn base_to_giga = divide<1'000'000'000.0>;

    // 10^12: tera
    inline conversion_fn tera_to_base = multiply<1'000'000'000'000.0>;
    inline conversion_fn base_to_tera = divide<1'000'000'000'000.0>;

    // 10^15: peta
    inline conversion_fn peta_to_base = multiply<1'000'000'000'000'000.0>;
    inline conversion_fn base_to_peta = divide<1'000'000'000'000'000.0>;

	inline number_t celsius_to_fahrenheit(number_t celsius) { return celsius * number_t(9) / number_t(5) + number_t(32); }
	inline number_t fahrenheit_to_celsius(number_t fahrenheit) { return (fahrenheit - number_t(32)) * number_t(5) / number_t(9); }

	number_t parse_expression_throws(const std::string& expression, const std::string& name);
	inline number_t return_self(number_t self) { return self; }

    class conversion
    {
    public:
        struct error : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        struct unit
        {
            std::string display_name;
            std::vector<std::string> aliases;

            unit_to_base_fn unit_to_base = return_self;
            base_to_unit_fn base_to_unit = return_self;

            string_to_unit_fn string_to_unit = [name = display_name](const std::string& expression)
                { return parse_expression_throws(expression, name); };
            unit_to_string_fn unit_to_string = [name = display_name](number_t unit, int decimals, bool separate)
                { return format_number(unit, decimals, separate) + " " + name; };
        };

        struct family
        {
            std::string name;
            std::vector<unit> units;
        };

        static void convert(const std::string& input, const std::string& destination_units, int decimals = -1, bool separate = false,
            std::string* result_out = nullptr, std::string* family_name_out = nullptr, number_t* single_dest_result_out = nullptr);

        static bool update_currencies(const nlohmann::json& data);
    private:
        static inline std::vector<family> families
        {
            { "Length",
                {
                    { "nm", { "nm", "nanometer", "nanometers", "nanometre", "nanometres" }, nano_to_base, base_to_nano},
                    { "µm", { "um", "micrometer", "micrometers", "micrometre", "micrometres" }, micro_to_base, base_to_micro },
                    { "mm", { "mm", "millimeter", "millimeters", "millimetre", "millimetres" }, milli_to_base, base_to_milli },
                    { "cm", { "cm", "centimeter", "centimeters", "centimetre", "centimetres" }, centi_to_base, base_to_centi },
                    { "dm", { "dm", "decimeter", "decimeters", "decimetre", "decimetres" }, deci_to_base, base_to_deci },
                    { "m", { "m", "meter", "meters", "metre", "metres" } },
                    { "km", { "km", "kilometer", "kilometers", "kilometre", "kilometres" }, kilo_to_base, base_to_kilo },
                }
            },
            { "Speed",
                {
                    { "km/h", { "km/h", "kmh", "kph", "kmph" } },
                    { "mph", { "mph", "mi/h" }, multiply<1.609344>, divide<1.609344> },
                    { "m/s", { "m/s", "mps"}, multiply<3.6>, divide<3.6> },
                    { "kn", { "kn", "kt", "knot", "knots"}, multiply<1.852>, divide<1.852> },
                }
            },
            { "Temperature",
                {
                    { "°C", { "c", "celsius"} },
                    { "°F", { "f", "fahrenheit"}, fahrenheit_to_celsius, celsius_to_fahrenheit },
                    { "K", { "k", "kelvin"}, subtract<273.15>, add<273.15> },
                }
            },
            { "Time",
                {
                    { "ns", { "ns", "nsec", "nsecs", "nanosecond", "nanoseconds" }, nano_to_base, base_to_nano },
                    { "µs", { "us", "usec", "usecs", "microsecond", "microseconds" }, micro_to_base, base_to_micro },
                    { "ms", { "ms", "msec", "msecs", "millisecond", "milliseconds" }, milli_to_base, base_to_milli },
                    { "sec", { "s", "sec", "secs", "second", "seconds" } },
                    { "min", { "m", "min", "mins", "minute", "minutes" }, multiply<60.0>, divide<60.0> },
                    { "hr", { "h", "hr", "hrs", "hour", "hours" }, multiply<3'600.0>, divide<3'600.0> },
                    { "day(s)", { "day(s)", "d", "day", "days"}, multiply<86'400.0>, divide<86'400.0> },
                    { "solar month(s)", { "mo", "month", "months", "smo", "solar month", "solar months" }, multiply<(365.25 / 12) * 86'400>, divide<(365.25 / 12) * 86'400> },
                    { "calendar month(s)", { "cmo", "calendar month", "calendar months" }, multiply<(365.0 / 12) * 86'400>, divide<(365.0 / 12) * 86'400> },
                    { "solar year(s)", { "y", "yr", "yrs", "sy", "syr", "syrs", "year", "years", "solar year", "solar years" }, multiply<365.25 * 86'400>, divide<365.25 * 86'400> },
                    { "calendar year(s)", { "cy", "cyr", "cyrs", "calendar year", "calendar years" }, multiply<365.0 * 86'400>, divide<365.0 * 86'400> },
                }
            },
        };
    public:
    };
}