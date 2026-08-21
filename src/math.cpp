#include <fixedphilip/math.h>
#include <fixedphilip/log.h>

#include <bulbtils/string.h>

#include <tinyexpr.h>

#include <stdexcept>
#include <format>
#include <algorithm>
#include <cmath>

namespace fixedphilip::math
{
	std::string thousands_separator::format_number(number_t number, int decimals)
	{
		std::locale custom_loc(std::locale::classic(), new thousands_separator);
		std::string result_str;
		if (decimals < 0)
		{
			result_str = std::format(custom_loc, "{:L}", number);
		}
		else
		{
			// oh my fucking god whatever already
			std::string number_str = std::format("{:.{}f}", number, decimals);
			result_str = std::format(custom_loc, "{:L}", string_to_number(number_str));
		}
		return result_str;
	}

	std::string format_number(number_t number, int decimals, bool separate)
	{
		std::string result_str;
		if (separate)
		{
			result_str = fixedphilip::math::thousands_separator::format_number(number, decimals);
		}
		else
		{
			if (decimals < 0)
			{
				result_str = std::format("{}", number);
			}
			else
			{
				result_str = std::format("{:.{}f}", number, decimals);
			}
		}
		return result_str;
	}

	number_t conversion::parse_expression_throws(const std::string& expression, const std::string& name)
	{
		int error = 0;
		auto result = te_interp(expression.c_str(), &error);
		if (error)
		{
			// not a conversion error
			throw std::runtime_error(std::format("Failed to parse expression for unit `{}`:\n```\n{}\n{}\n```",
				name, dpp::utility::markdown_escape(expression, true), std::string(error, ' ') + "↑"));
		}
		return result;
	}

	void conversion::convert(const std::string& input, const std::string& destination_units, int decimals, bool separate, std::string* result_out, std::string* family_name_out, number_t* single_dest_result_out)
	{
		if (input.empty())
		{
			throw fixedphilip::math::conversion::error("No input specified");
		}
		if (destination_units.empty())
		{
			throw fixedphilip::math::conversion::error("No destination units specified");
		}

		auto input_lower = input;
		bulbtils::string::inplace::to_lowercase(input_lower);

		auto dest_units_lower = destination_units;
		bulbtils::string::inplace::to_lowercase(dest_units_lower);

		auto input_splits = bulbtils::string::split_by_whitespace(input_lower);
		auto dest_units_splits = bulbtils::string::split_by_whitespace(dest_units_lower);

		struct unit_search_result
		{
			conversion::unit* unit;
			size_t index;
		};
		struct all_unit_search_result : public unit_search_result
		{
			bool input; // false -> dest_units
			std::string family;

			all_unit_search_result(conversion::unit* unit, size_t index, const std::string& family, bool input) : unit_search_result(unit, index), family(family), input(input) {}
		};
		std::vector<all_unit_search_result> all_results;

		for (auto& family : families)
		{
			std::vector<unit_search_result> from_results, to_results;

			int bad_dest_unit_count = 0;

			for (auto& unit : family.units)
			{
				for (auto& alias : unit.aliases)
				{
					for (int i = 0; i < input_splits.size(); i++)
					{
						if (alias == input_splits[i])
						{
							from_results.emplace_back(&unit, i);
							all_results.emplace_back(&unit, i, family.name, true);
						}
					}

					for (int i = 0; i < dest_units_splits.size(); i++)
					{
						if (alias == dest_units_splits[i])
						{
							// proportionality test
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
							all_results.emplace_back(&unit, i, family.name, false);
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
				// formatting input for output (spaces in display name are OK)
				input_splits[from_result.index] = from_result.unit->display_name;

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

			// NO MORE THROWS BEYOND THIS POINT
			if (family_name_out)
			{
				*family_name_out = family.name;
			}

			std::string input_str = "";
			for (int i = 0; i < input_splits.size(); i++)
			{
				if (i > 0)
				{
					input_str += " ";
				}
				input_str += input_splits[i];
			}

			if (to_results.size() == 1)
			{
				auto result = to_results[0].unit->base_to_unit(result_in_base_units);
				if (result_out)
				{
					*result_out = input_str + " = " + to_results[0].unit->unit_to_string(result, decimals, separate);
				}
				if (single_dest_result_out)
				{
					*single_dest_result_out = result;
				}
				return;
			}

			number_t result_in_smallest_units = to_results[to_results.size() - 1].unit->base_to_unit(result_in_base_units);

			std::string result_str = input_str + " = ";
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
			if (result_out)
			{
				*result_out = result_str;
			}
			return;
		}

		if (all_results.empty())
		{
			throw fixedphilip::math::conversion::error(std::format("Failed to find a suitable conversion for \"{}\" to \"{}\" - no units detected", 
				dpp::utility::markdown_escape(input, true), dpp::utility::markdown_escape(destination_units, true)));
		}
		else
		{
			std::string error_str = std::format("Failed to find a suitable conversion for \"{}\" to \"{}\" - {} unit{} detected:", 
				dpp::utility::markdown_escape(input, true), dpp::utility::markdown_escape(destination_units, true), all_results.size(), all_results.size() == 1 ? "" : "s");
			for (auto& result : all_results)
			{
				error_str += std::format("\n• {} `{}` (\"{}\", {})", result.input ? "From" : "To", result.input ? input_splits[result.index] : dest_units_splits[result.index], result.unit->display_name, result.family);
			}
			throw fixedphilip::math::conversion::error(error_str);
		}
	}

	bool conversion::update_currencies(const nlohmann::json& data)
	{
		std::string family_name = "";
		try
		{
			std::string date = data.begin().value().at("date");
			family_name = std::format("Currency ({})", date);
		}
		catch (const std::exception& e)
		{
			fixedphilip::log::error(std::format("Exception reading date from currency conversion json file: {}", e.what()));
			return false;
		}
		if (family_name.empty() || family_name == "Currency ()")
		{
			fixedphilip::log::warning("Currency conversion family name is empty");
			family_name = "Currency";
		}

		struct conversion_currency : public fixedphilip::math::conversion::unit
		{
			conversion_currency(std::string full_name, std::string alias, fixedphilip::math::number_t currency_to_one_usd, fixedphilip::math::number_t one_usd_to_currency)
				: fixedphilip::math::conversion::unit(full_name, { alias })
			{
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

		fixedphilip::math::conversion::family currency_conversion_family { family_name, {} };

		for (auto& [currency_key, currency_info] : data.items())
		{
			std::string name = "";
			try
			{
				name = currency_info.at("code");
				bulbtils::string::inplace::to_lowercase(name);
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

			currency_conversion_family.units.push_back(conversion_currency { pretty_print, name, currency_to_usd, usd_to_currency });
		}

		if (currency_conversion_family.units.empty())
		{
			fixedphilip::log::error("The new currency conversion family is empty and will not be updated");
			return false;
		}
		currency_conversion_family.units.push_back(conversion_currency { "U.S. Dollar", "usd", 1, 1 });

		std::erase_if(fixedphilip::math::conversion::families, [](const auto& item) { return item.name.starts_with("Currency"); });
		fixedphilip::math::conversion::families.push_back(currency_conversion_family);
		return true;
	}

	conversion::unit::unit(std::string name, std::initializer_list<std::string> alias_list)
		: display_name(name), aliases(alias_list)
	{
		unit_to_base = identity;
		base_to_unit = identity;

		string_to_unit = [name](const std::string& expression) { return parse_expression_throws(expression, name); };
		unit_to_string = [name](number_t unit, int decimals, bool separate) { return format_number(unit, decimals, separate) + " " + name; };
	}

	conversion::unit::unit(std::string name, std::initializer_list<std::string> alias_list, unit_to_base_fn unit_to_base_function, base_to_unit_fn base_to_unit_function)
		: display_name(name), aliases(alias_list), unit_to_base(unit_to_base_function), base_to_unit(base_to_unit_function)
	{
		string_to_unit = [name](const std::string& expression) { return parse_expression_throws(expression, name); };
		unit_to_string = [name](number_t unit, int decimals, bool separate) { return format_number(unit, decimals, separate) + " " + name; };
	}

	conversion::unit::unit(std::string name, std::initializer_list<std::string> alias_list, string_to_unit_fn string_to_unit_function, unit_to_string_fn unit_to_string_function)
		: display_name(name), aliases(alias_list), string_to_unit(string_to_unit_function), unit_to_string(unit_to_string_function)
	{
		unit_to_base = identity;
		base_to_unit = identity;
	}
}