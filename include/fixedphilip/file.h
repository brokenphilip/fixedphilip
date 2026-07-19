#pragma once

#include <fixedphilip/log.h>

#include <dpp/nlohmann/json.hpp>

namespace fixedphilip::file
{
	struct settings
	{
		// Name of the file being saved, relative to the current directory
		std::string filename = "";

		// Should files be created if they aren't found?
		// If used with load(), a file containing default data will be created.
		bool create_if_not_found = false;

		// Should logs be printed when saving/loading files?
		bool log = true;
	};

	enum result
	{
		r_success,
		r_file_not_found,
		r_open_file_error,
		r_read_error,
		r_write_error,
		r_parse_error,
	};

	struct base
	{
		// Use this function to convert your data structure to a string,
		// which will be written to a file
		virtual std::string save_from_struct() = 0;

		// Use this function to convert the string data loaded from a file
		// to your data structure. Return false to pass 's_parse_error' to load()
		virtual bool load_to_struct(const std::string& data) = 0;

		// Load data structure from file, based on load settings. On success, returns 'r_success' - on failure, returns:
		// - 'r_open_file_error' if it failed to open the file for reading (or writing default data)
		// - 'r_write_error' if it failed to write defaults to the file
		// - 'r_read_error' if it failed to read the file
		// - 'r_file_not_found' if the file was not found (default data will be written if create_if_not_found is enabled)
		// - 'r_parse_error' if the data couldn't be parsed (load_to_struct == false)
		result load(const settings& load_settings);

		// Load data structure from file, based on load settings. On success, returns 'r_success' - on failure, returns:
		// - 'r_open_file_error' if it failed to open the file for writing
		// - 'r_write_error' if it failed to write contents to the file
		// - 'r_file_not_found' if the file was not found and create_if_not_found is disabled
		result save(const settings& save_settings);
	};

	template <int indent = -1, char indent_char = ' '>
	struct json : public base
	{
		// Helper wrapper function for json.at() with logging output
		// Returns true if json.at() was successful, false otherwise
		template <typename T>
		bool try_at(const nlohmann::json& data, const std::string& key, T& member_variable)
		{
			try
			{
				member_variable = data.at(key);
				return true;
			}
			catch (const std::exception& e)
			{
				fixedphilip::log::error(std::format("Exception reading '{}' json key: {}", key, e.what()));
				return false;
			}
		}

		// Use this function to convert your data structure to a json,
		// which will be written to a file
		virtual nlohmann::json struct_to_json() = 0;

		// Use this function to convert the string data loaded from a file
		// to your data structure. Return false to pass 's_parse_error' to load()
		// NOTE: If partial success is legal, it's better to return true than false,
		// and handle uncaught exceptions on a per-case (member variable) basis
		virtual bool json_to_struct(const nlohmann::json& data) = 0;

		virtual std::string save_from_struct() override //final
		{
			return struct_to_json().dump(indent, indent_char);
		}

		virtual bool load_to_struct(const std::string& data) override //final
		{
			try
			{
				return json_to_struct(nlohmann::json::parse(data));
			}
			catch (const std::exception& e)
			{
				fixedphilip::log::error(std::format("Exception parsing json file: {}", e.what()));
				return false;
			}
		}
	};

	using json_pretty_print = json<4, ' '>;
}