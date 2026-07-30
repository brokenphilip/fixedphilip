#pragma once

#include <fixedphilip/utils/string.h>

#include <dpp/coro/task.h>
#include <dpp/nlohmann/json.hpp>

#include <functional>
#include <locale>

namespace fixedphilip::math
{
    // NOTE: a lot of functions rely on number_t being double, needs to be cleaned up
	using number_t = double;
	using conversion_fn = std::function<number_t(number_t)>;

    struct thousands_separator : std::numpunct<char>
    {
        inline char do_thousands_sep() const override { return ' '; }
        inline std::string do_grouping() const override { return "\3"; }

        static std::string format_number(number_t number, int decimals = -1);
    };

    inline number_t string_to_number(const std::string& str) { return std::stod(str); }
    std::string format_number(number_t number, int decimals = -1, bool separate = false);

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
    inline number_t identity(number_t self) { return self; }

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

    // This class handles conversion between measurable units, as well as currencies
    // Essentially just a namespace in practice, but hides implementation in "private" data
    class conversion
    {
        // The conversion pipeline is as follows
        // (string ->) unit -> base -> unit (-> string)
        using string_to_unit_fn = std::function<number_t(const std::string& str)>;
        using unit_to_base_fn = conversion_fn;
        using base_to_unit_fn = conversion_fn;
        using unit_to_string_fn = std::function<std::string(number_t, int, bool)>;

        static inline number_t celsius_to_fahrenheit(number_t celsius) { return celsius * number_t(9) / number_t(5) + number_t(32); }
        static inline number_t fahrenheit_to_celsius(number_t fahrenheit) { return (fahrenheit - number_t(32)) * number_t(5) / number_t(9); }

        // Default string_to_unit function, which allows parsing math expressions
        static number_t parse_expression_throws(const std::string& expression, const std::string& name);
    public:
        struct error : public std::runtime_error
        {
            using std::runtime_error::runtime_error;
        };

        struct unit
        {
            // The "pretty" name of the unit that gets displayed to the user
            std::string display_name;

            // Various aliases of the unit that get searched for when scanning user input
            std::vector<std::string> aliases;

            // Functionms for converting between this unit and the base unit
            // Generally, you want these two functions to be inverse of each other
            unit_to_base_fn unit_to_base;
            base_to_unit_fn base_to_unit;

            // Functions for parsing the number from (and formatting the number to) a string
            string_to_unit_fn string_to_unit;
            unit_to_string_fn unit_to_string;

            // these four useless constructors were brought to you by g++, thanks very cool :3

            // Creates an identity unit with standard string parsing functions
            unit(std::string name, std::initializer_list<std::string> alias_list);

            // Creates a unit with standard string parsing functions
            unit(std::string name, std::initializer_list<std::string> alias_list,
                unit_to_base_fn unit_to_base_function, base_to_unit_fn base_to_unit_function);

            // Creates an identity unit with custom string parsing functions
            unit(std::string name, std::initializer_list<std::string> alias_list,
                string_to_unit_fn string_to_unit_function, unit_to_string_fn unit_to_string_function);

            // Creates a unit with custom string parsing functions
            inline unit(std::string name, std::initializer_list<std::string> alias_list,
                unit_to_base_fn unit_to_base_function, base_to_unit_fn base_to_unit_function,
                string_to_unit_fn string_to_unit_function, unit_to_string_fn unit_to_string_function)
                :   display_name(name), aliases(alias_list),
                    unit_to_base(unit_to_base_function), base_to_unit(base_to_unit_function),
                    string_to_unit(string_to_unit_function), unit_to_string(unit_to_string_function) {};
        };

        struct family
        {
            std::string name;
            std::vector<unit> units;
        };
    private:
        // An internal list of unit families, used for conversion
        // Currencies are added to the first successful call to "update_currencies"
        static inline std::vector<family> families
        {
            { "Area",
                {
                    { "mm²", { "sq.mm", "sqmm", "mm2", "mm^2"/*, "square millimeter", "square millimeters", "square millimetre", "square millimetres"*/ }, micro_to_base, base_to_micro },
                    { "cm²", { "sq.cm", "sqcm", "cm2", "cm^2"/*, "square centimeter", "square centimeters", "square centimetre", "square centimetres"*/ }, divide<10'000.0>, multiply<10'000.0> },
                    { "dm²", { "sq.dm", "sqdm", "dm2", "dm^2"/*, "square decimeter", "square decimeters", "square decimetre", "square decimetres"*/ }, centi_to_base, base_to_centi },
                    { "m²", { "sq.m", "sqm", "m2", "m^2"/*, "square meter", "square meters", "square metre", "square metres"*/ } },
                    { "a", { "a", "are" }, multiply<100.0>, divide<100.0> },
                    { "ha", { "ha", "hectare" }, multiply<10'000.0>, divide<10'000.0> },
                    { "km²", { "sq.km", "sqkm", "km2", "km^2"/*, "square kilometer", "square kilometers", "square kilometre", "square kilometres"*/ }, mega_to_base, base_to_mega },

                    { "in²", { "sq.in", "sqin", "in2", "in^2"/*, "square inch", "square inches"*/ }, multiply<0.00064516>, divide<0.00064516> },
                    { "ft²", { "sq.ft", "sqft", "ft2", "ft^2"/*, "square foot", "square feet"*/ }, multiply<0.09290304>, divide<0.09290304> },
                    { "yd²", { "sq.yd", "sqyd", "yd2", "yd^2"/*, "square yard", "square yards"*/ }, multiply<0.83612736>, divide<0.83612736> },
                    { "ac", { "ac", "acre" }, multiply<4'046.8564224>, divide<4'046.8564224> },
                    { "mi²", { "sq.mi", "sqmi", "mi2", "mi^2"/*, "square mile", "square miles"*/ }, multiply<2'589'988.110336>, divide<2'589'988.110336> },

                    { "Hu²", { "sq.hu", "sqhu", "hu2", "hu^2"/*, "square hammer unit", "square hammer units"*/ }, divide<2'755.55704356>, multiply<2'755.55704356> },
                }
            },
            { "Length",
                {
                    { "nm", { "nm", "nanometer", "nanometers", "nanometre", "nanometres" }, nano_to_base, base_to_nano },
                    { "µm", { "um", "micrometer", "micrometers", "micrometre", "micrometres" }, micro_to_base, base_to_micro },
                    { "mm", { "mm", "millimeter", "millimeters", "millimetre", "millimetres" }, milli_to_base, base_to_milli },
                    { "cm", { "cm", "centimeter", "centimeters", "centimetre", "centimetres" }, centi_to_base, base_to_centi },
                    { "dm", { "dm", "decimeter", "decimeters", "decimetre", "decimetres" }, deci_to_base, base_to_deci },
                    { "m", { "m", "meter", "meters", "metre", "metres" } },
                    { "km", { "km", "kilometer", "kilometers", "kilometre", "kilometres" }, kilo_to_base, base_to_kilo },

                    { "in", { "in", "inch", "inches", "\"" }, multiply<0.0254>, divide<0.0254> },
                    { "ft", { "ft", "foot", "feet", "'" }, multiply<0.3048>, divide<0.3048> },
                    { "yd", { "yd", "yard", "yards" }, multiply<0.9144>, divide<0.9144> },
                    { "mi", { "mi", "mile", "miles" }, multiply<1609.344>, divide<1609.344> },

                    { "nmi", { "nmi", "nm"/*, "nautical mile", "nautical miles"*/ }, multiply<1'852.0>, divide<1'852.0> },

                    { "Hu", { "hu"/*, "hammer unit", "hammer units"*/ }, divide<52.4934>, multiply<52.4934> },
                }
            },
            { "Speed",
                {
                    { "km/h", { "km/h", "kmh", "kph", "kmph" } },
                    { "m/s", { "m/s", "mps"}, multiply<3.6>, divide<3.6> },

                    { "mph", { "mph", "mi/h" }, multiply<1.609344>, divide<1.609344> },

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
                    { "solar month(s)", { "mo", "month", "months", "smo"/*, "solar month", "solar months"*/ }, multiply<(365.25 / 12) * 86'400>, divide<(365.25 / 12) * 86'400> },
                    { "calendar month(s)", { "cmo"/*, "calendar month", "calendar months"*/ }, multiply<(365.0 / 12) * 86'400>, divide<(365.0 / 12) * 86'400> },
                    { "solar year(s)", { "y", "yr", "yrs", "sy", "syr", "syrs", "year", "years"/*, "solar year", "solar years"*/ }, multiply<365.25 * 86'400>, divide<365.25 * 86'400> },
                    { "calendar year(s)", { "cy", "cyr", "cyrs"/*, "calendar year", "calendar years"*/ }, multiply<365.0 * 86'400>, divide<365.0 * 86'400> },
                }
            },
            { "Mass",
                {
                    { "ng", { "ng", "nanogram", "nanograms" }, nano_to_base, base_to_nano },
                    { "µg", { "ug", "microgram", "micrograms" }, micro_to_base, base_to_micro },
                    { "mg", { "mg", "milligram", "milligrams" }, milli_to_base, base_to_milli },
                    { "g", { "g", "gram", "grams" } },
                    { "kg", { "kg", "kilogram", "kilograms" }, kilo_to_base, base_to_kilo },
                    { "t", { "t", "ton", "tonne"/*, "metric ton"*/ }, mega_to_base, base_to_mega }, // todo - use this for long/short ton?

                    { "oz", { "oz", "ounce", "ounces" }, multiply<28.34952>, divide<28.34952> },
                    { "lb", { "lb", "lbs", "pound", "pounds" }, multiply<493.59237>, divide<493.59237> },
                    { "st", { "st", "stone", "stones" }, multiply<6'350.29497>, divide<6'350.29497> },
                    //{ "long ton(s)", { "long ton", "long tons", "imperial ton", "imperial tons", "british ton", "british tos", "displacement ton", "displacement tons" }, divide<1'016'047.0>, multiply<1'016'047.0> },
                    //{ "short ton(s)", { "short ton", "short tons", "tn", "st", "us ton", "us tons" }, divide<907'180.0>, multiply<907'180.0> },
                }
            },
        };
    public:
        // Convert between a list of "input" units (and their values), to a list of "destination" units
        // Throws conversion::error if conversion was unsuccessful, and std::runtime_error for more-general user input errors
        // Pass "result_out" to get the resulting string and "family_name_out" to get the resulting unit family
        // If you've supplied only one "destination" unit, you may also pass "single_dest_result_out" to get the numeric result
        static void convert(const std::string& input, const std::string& destination_units, int decimals = -1, bool separate = false,
            std::string* result_out = nullptr, std::string* family_name_out = nullptr, number_t* single_dest_result_out = nullptr);

        // Update the internal list of unit families with the latest currency exchange information
        static bool update_currencies(const nlohmann::json& data);
    };
}
