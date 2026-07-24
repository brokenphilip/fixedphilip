#pragma once

#include <fixedphilip/utils/string.h>

#include <tinyexpr.h>

#include <string>
#include <vector>
#include <ranges>
#include <algorithm>
#include <variant>
#include <sstream>
#include <functional>
#include <format>

namespace fixedphilip::math
{
    /*

conversion_family<typename BaseType>

using unit_to_base_fn = BaseType(const std::smatch&);
using base_to_unit_fn = std::string(BaseType);

each conversion_unit has its own ^^^ funcs, as well as regex

conversion_family has the convert function
it uses the from unit's regex to get a smatch
[0] entire string, [1...] params
from->unit_to_base gets called
then to->base_to_unit gets called
the resulting string is sent back
but we should either:
1. return std::string and do error handling if necessary
2. return bool for success, return std::string as reference for error/result
3. exceptions??????????
two types of regex must exist probably:
1. when checking for source (w/ numbers)
2. when checking for destination (w/o numbers)
regexes can't be concat'd, maybe they need to be stored as const char* instead
or, alternatively, numbers can be tokened like with presence activity
or, alternatively, second regex to check as "to" (first one checked as "from")

BALLER SHIT: "4 days 15 hours to minutes seconds" (mutliple conversions)

just search for "s" or "seconds" or something, everything to the left is an expression, everything to the right is potentially another expression-unit pair
be aware of ambiguity "m" as "minutes" or "meters" (depends on destination unit, so maybe check that first, if it's still ambiguous check any other potential from units (cuz there can be multiple))
some units can't be divided, such as converting to "euros and dollars" (how many dollars in a euro? doesn't make sense lmfao),
so maybe it's best to have separate unique destination-only units like "ftin", "hhmmss" etc... (but still only use "ft"+"in", "h"+"m"+"s" for the from units)

keep the conversion family system, but instead of number_t(number_t) use strings (to account for ft-in (5'10, 5'10"), hexadecimal (0xABC, ABCh) etc...)
virtual unit conversion functions will have to be virtual
each unit will have to be instantiated on its own (maybe not
 [RUSH],
brokenphilip [RUSH],  — 21-Jul-26 12:53
maybe not
 [RUSH],
brokenphilip [RUSH],  — 21-Jul-26 12:54
you can't type out the unit after the fact either, you must do it here
to print stuff like 5 ft 10 in

*/

	//using number_t = double;
	
	//union number_t
	//{
	//	double number_dbl;
	//	int64_t number_i64;
	//};

	using number_t = std::variant<double, int64_t>;

	// (string ->) unit -> base -> unit (-> string)
	using string_to_unit_fn = std::function<number_t(const std::string&)>;
	using unit_to_base_fn = std::function<number_t(number_t)>;
	using base_to_unit_fn = std::function<number_t(number_t)>;
	using unit_to_string_fn = std::function<std::string(number_t, int, bool)>;

	number_t parse_expression_throws(const std::string& expression)
	{
		int error = 0;
		auto result = te_interp(expression.c_str(), &error);
		if (error)
		{
			throw;
		}
		return result;
	}

	number_t return_self(number_t self)
	{
		return self;
	}

	std::string format_unit(const std::string& name, number_t value, int decimals, bool separate)
	{
		if (decimals < 0)
		{
			return std::format("{} {}", value, name);
		}
		else
		{
			return std::format("{:.{}f} {}", value, decimals, name);
		}
	}

	struct conversion_unit
	{
		std::vector<std::string> aliases;

		unit_to_base_fn unit_to_base = return_self;
		base_to_unit_fn base_to_unit = return_self;

		string_to_unit_fn string_to_unit = parse_expression_throws;
		unit_to_string_fn unit_to_string = [first_alias = aliases[0]](number_t unit, int decimals, bool separate) 
			{ return format_unit(first_alias, unit, decimals, separate); };
	};

	struct conversion_family
	{
		std::string name;

		std::vector<conversion_unit> units;


		enum convert_result
		{
			r_success,
		};

		convert_result convert(const std::string& input, const std::string& destination_units, BaseType* result)
		{
			if (input.empty())
			{
				// Error: no input specified
			}

			if (destination_units.empty())
			{
				// Error: no destination units specified
			}

			auto input_lower = input;
			fixedphilip::utils::string::inplace::to_lowercase(input_lower);
			//auto input_splits = input_lower | std::views::split(' ') | std::ranges::to<std::vector<std::string>>();

			std::istringstream input_iss(input_lower);
			std::vector<std::string> input_splits;

			std::string input_split_temp;
			while (input_iss >> input_split_temp)
			{
				input_splits.push_back(input_split_temp);
			}
			
			auto dest_units_lower = destination_units;
			fixedphilip::utils::string::inplace::to_lowercase(dest_units_lower);
			//auto dest_units_splits = dest_units_lower | std::views::split(' ') | std::ranges::to<std::vector<std::string>>();

			std::istringstream dest_units_iss(dest_units_lower);
			std::vector<std::string> dest_units_splits;

			std::string dest_unit_split_temp;
			while (dest_units_iss >> dest_unit_split_temp)
			{
				dest_units_splits.push_back(dest_unit_split_temp);
			}

			struct find_result
			{
				conversion_unit& unit;
				size_t index;
			};
			std::vector<find_result> from_results, to_results;

			for (auto& unit : units)
			{
				for (auto& alias : unit.aliases)
				{
					for (int i = 0; i < input_splits.size(); i++)
					{
						if (alias == input_splits[i])
						{
							from_results.emplace_back(unit, i);
						}
					}

					for (int i = 0; i < dest_units_splits.size(); i++)
					{
						if (alias == dest_units_splits[i])
						{
							to_results.emplace_back(unit, i);
						}
					}
				}
			}

			if (from_results.empty())
			{
				// Error: no compatible "from" units
			}

			if (to_results.empty())
			{
				// Error: no compatible "to" units
			}

			if (to_results.size() != dest_units_splits.size())
			{
				std::vector<std::string> bad_units;

				for (int i = 0; i < dest_units_splits.size(); i++)
				{
					if (!std::any_of(to_results.begin(), to_results.end(), [i](const find_result& fr) { fr.index == i; }))
					{
						bad_units.push_back(dest_units_splits[i]);
					}
				}

				// Error: unrecognized unit(s) - bad_units
			}

			// sort by index, ascending
			std::sort(from_results.begin(), from_results.end(), [](const find_result& a, const find_result& b)
			{
				return a.index < b.index;
			});

			auto last_from_index = from_results.back().index;
			if (last_from_index != input_splits.size() - 1)
			{
				std::string bad_tokens = "";

				bool first = true;
				for (int i = last_from_index; i < input_splits.size(); i++)
				{
					if (!first)
					{
						bad_tokens += " ";
					}
					bad_tokens += input_splits[i];

					first = false;
				}
				// Error: unindentified token(s) - bad_tokens
			}

			// sort by unit-to-base, descending
			std::sort(to_results.begin(), to_results.end(), [](const find_result& a, const find_result& b)
			{
				return false;
			});

			/*

			check if all to_results unit-to-bases are linear
			
			eg. for unit-to-base(1)

			hours minutes seconds
			3600  60      1
			
			unit-to-base(2) should equal to

			hours minutes seconds
			7200  120     2

			this will fail for kelvin, fahrenheit and similar

			additionally, check if any unit-to-bases are equal
			these are also incompatible
			
			*/

			BaseType result_in_base_units = {};
			for (auto& from_result : from_results)
			{
				// result_in_base_units += from_result.unit.unit_to_base(...);
				// do not parse expressions here, leave that to the units themselves
				// bcs some units only parse text and don't actually convert, eg. hex to binary or similar
			}

			for (auto& to_result : to_results)
			{
				// convert in order, eg. hours -> minutes -> seconds
			}
		}
	};

	std::vector<conversion_family> conversion_families;

	void convert(const std::string& input, const std::string& destination_units)
	{
		for (auto& conversion_family : conversion_families)
		{
			for (auto& unit : conversion_family.units)
			{

			}
		}
	}
}