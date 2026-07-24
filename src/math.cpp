#include <fixedphilip/math.h>

#include <tinyexpr.h>

#include <stdexcept>
#include <format>
#include <algorithm>
#include <cmath>

namespace fixedphilip::math
{
	number_t parse_expression_throws(const std::string& expression)
	{
		int error = 0;
		auto result = te_interp(expression.c_str(), &error);
		if (error)
		{
			throw std::runtime_error(std::format("Failed to parse expression:\n```\n{}\n{}\n```", expression, std::string(error, ' ') + "↑"));
		}
		return result;
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

	std::string convert(const std::string& input, const std::string& destination_units, int decimals, bool separate, number_t* result_if_single_dest_unit)
	{
		if (input.empty())
		{
			throw std::runtime_error("No input specified");
		}
		if (destination_units.empty())
		{
			throw std::runtime_error("No destination units specified");
		}

		auto input_lower = input;
		fixedphilip::utils::string::inplace::to_lowercase(input_lower);

		auto dest_units_lower = destination_units;
		fixedphilip::utils::string::inplace::to_lowercase(dest_units_lower);

		auto input_splits = fixedphilip::utils::string::split_by_whitespace(input_lower);
		auto dest_units_splits = fixedphilip::utils::string::split_by_whitespace(dest_units_lower);

		for (auto& conversion_family : conversion_families)
		{
			struct unit_search_result
			{
				conversion_unit* unit;
				size_t index;
			};
			std::vector<unit_search_result> from_results, to_results;

			int bad_dest_unit_count = 0;

			for (auto& unit : conversion_family.units)
			{
				for (auto& alias : unit.aliases)
				{
					for (int i = 0; i < input_splits.size(); i++)
					{
						if (alias == input_splits[i])
						{
							from_results.emplace_back(&unit, i);
						}
					}

					for (int i = 0; i < dest_units_splits.size(); i++)
					{
						if (alias == dest_units_splits[i])
						{
							// linearity test
							constexpr number_t control = 69'420;
							auto one = 2 * unit.unit_to_base(control) / unit.unit_to_base(control * 2);
							auto is_one = 0.99 < one && one < 1.01;
							if (!is_one)
							{
								bad_dest_unit_count++;
							}
							if (bad_dest_unit_count > 1)
							{
								// cannot convert to multiple units if they're not linear
								// maybe what we're looking for is in another family?
								goto next_family;
							}

							to_results.emplace_back(&unit, i);
						}
					}
				}
			}

			if (bad_dest_unit_count > 1)
			{
				next_family:
				continue;
			}

			if (from_results.empty())
			{
				// No compatible "from" units, maybe they're in a different family?
				continue;
			}
			if (to_results.empty())
			{
				// No compatible "to" units, maybe they're in a different family?
				continue;
			}
			if (to_results.size() != dest_units_splits.size())
			{
				// Not all "to" units are compatible, maybe they're all compatible in a different family?
				continue;
			}

			// sort by index, ascending
			std::sort(from_results.begin(), from_results.end(), [](const unit_search_result& a, const unit_search_result& b)
			{
				return a.index < b.index;
			});

			if (from_results.back().index != input_splits.size() - 1)
			{
				// Not all "from" units are compatible, maybe they're all compatible in a different family?
				// NOTE: it's also possible the user erroneously placed an invalid token (text or numbers) at the end
				continue;
			}

			if (to_results.size() > 1)
			{
				bool bad_units = false;

				// sort by unit-to-base, descending
				std::sort(to_results.begin(), to_results.end(), [&bad_units](const unit_search_result& a, const unit_search_result& b)
				{
					auto a_unit_to_base = a.unit->unit_to_base(number_t(1));
					auto b_unit_to_base = b.unit->unit_to_base(number_t(1));

					if (a_unit_to_base == b_unit_to_base)
					{
						bad_units = true;
					}

					return a_unit_to_base > b_unit_to_base;
				});

				if (bad_units)
				{
					// two or more units are seemingly identical, which is not allowed for multiple destination units
					// maybe this is not the case in a different family?
					continue;
				}
			}

			number_t result_in_base_units = {};

			int last_index = 0;
			for (auto& from_result : from_results)
			{
				std::string input_string = "";
				bool first_token = true;
				for (int i = last_index; i < from_result.index; i++)
				{
					if (!first_token)
					{
						input_string += " ";
					}
					input_string += input_splits[i];
					first_token = false;
				}

				// this can throw
				number_t result_in_units = from_result.unit->string_to_unit(input_string);

				if (last_index == 0)
				{
					result_in_base_units = from_result.unit->unit_to_base(result_in_units);
				}
				else
				{
					result_in_base_units = result_in_base_units + from_result.unit->unit_to_base(result_in_units);
				}
				last_index = from_result.index + 1;
			}

			if (to_results.size() == 1)
			{
				auto result = to_results[0].unit->base_to_unit(result_in_base_units);
				if (result_if_single_dest_unit)
				{
					*result_if_single_dest_unit = result;
				}
				return "### :repeat: **| " + conversion_family.name + ":**\n> " + input + " = " + to_results[0].unit->unit_to_string(result, decimals, separate);
			}

			number_t result_in_smallest_units = to_results[to_results.size() - 1].unit->base_to_unit(result_in_base_units);

			std::string result_str = "### :repeat: **| " + conversion_family.name + ":**\n> " + input + " = ";
			for (int i = 0; i < to_results.size(); i++)
			{
				number_t how_many_of_this_unit_are_there_in_last_unit = to_results[to_results.size() - 1].unit->base_to_unit(to_results[i].unit->unit_to_base(1));

				auto result = std::floor(result_in_smallest_units / how_many_of_this_unit_are_there_in_last_unit);
				if (i < to_results.size() - 1)
				{
					result_in_smallest_units = std::fmod(result_in_smallest_units, how_many_of_this_unit_are_there_in_last_unit);
				}
				if (i > 0)
				{
					result_str += " ";
				}
				result_str += to_results[i].unit->unit_to_string(result, decimals, separate);
			}
			return result_str;
		}
		throw std::runtime_error(std::format("Failed to find a suitable conversion for `{}` to `{}`", input, destination_units));
	}
}