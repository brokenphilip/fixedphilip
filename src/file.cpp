#include <fixedphilip/file.h>

namespace fixedphilip::file
{
	result base::load(const settings& load_settings)
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

		//fixedphilip::log::info(std::format("READ {}\n\n\n{}\n\n\n", filename, data));

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
	inline result base::save(const settings& save_settings)
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
}