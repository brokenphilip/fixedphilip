#pragma once

#include <fixedphilip/log.h>

namespace fixedphilip::utils::file
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
		inline result load(const settings& load_settings)
		{
			auto& filename = load_settings.filename;
			auto log = load_settings.log;

			if (!std::filesystem::exists(filename))
			{
				if (load_settings.create_if_not_found)
				{
					if (log)
					{
						fixedphilip::log::warning(std::format("Error loading '{}' - file not found, writing default data...", filename));
					}

					// copy settings for saving defaults, but use our own logs instead (if enabled)
					settings save_settings = load_settings;
					save_settings.log = false;

					auto result = save(save_settings);
					if (log)
					{
						if (result == r_open_file_error)
						{
							fixedphilip::log::error(std::format("Failed to open file '{}' for writing default data", filename));
						}
						else if (result == r_write_error)
						{
							fixedphilip::log::error(std::format("Failed to write default data to file '{}'", filename));
						}
					}

					// if successful, default file was created - but still return "file not found"
					return (result == r_success) ? r_file_not_found : result;
				}

				if (log)
				{
					fixedphilip::log::error(std::format("Error loading '{}' - file not found", filename));
				}
				return r_file_not_found;
			}

			std::ifstream config_file(filename);
			if (!config_file)
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error loading '{}' - failed to open file", filename));
				}
				return r_open_file_error;
			}

			auto size = std::filesystem::file_size(filename);
			std::string data(size, '\0');
			if (!(config_file.read(&data[0], size)) && !(config_file.eof()))
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error loading '{}' - failed to read from file", filename));
				}
				return r_read_error;
			}

			//fixedphilip::log::info(std::format("\n\n\n{}\n\n\n", data));

			if (!load_to_struct(data))
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error loading '{}' - failed to parse data from file", filename));
				}
				return r_parse_error;
			}
			return r_success;
		}

		// Load data structure from file, based on load settings. On success, returns 'r_success' - on failure, returns:
		// - 'r_open_file_error' if it failed to open the file for writing
		// - 'r_write_error' if it failed to write contents to the file
		// - 'r_file_not_found' if the file was not found and create_if_not_found is disabled
		inline result save(const settings& save_settings)
		{
			auto& filename = save_settings.filename;
			auto log = save_settings.log;

			if (!std::filesystem::exists(filename) && !save_settings.create_if_not_found)
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error saving '{}' - file not found", filename));
				}
				return r_file_not_found;
			}

			std::ofstream file(filename);
			if (!file)
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error saving '{}' - failed to open file", filename));
				}
				return r_open_file_error;
			}

			if (!(file << save_from_struct()))
			{
				if (log)
				{
					fixedphilip::log::error(std::format("Error saving '{}' - failed to write to file", filename));
				}
				return r_write_error;
			}

			return r_success;
		}
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