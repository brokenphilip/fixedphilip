#include <discofloor/bot.h>

#include <bulbtils/string.h>

#include <fixedphilip/build.h>

namespace discofloor
{
	class presence_module : public module
	{
		class presence_config : public pretty_print_json_file
		{
			// just online/idle is functional?
			const std::unordered_map<dpp::presence_status, std::string> status_to_string
			{
				{ dpp::ps_offline, "offline" },
				{ dpp::ps_online, "online" },
				{ dpp::ps_dnd, "dnd" },
				{ dpp::ps_idle, "idle" },
				{ dpp::ps_invisible, "invisible" },
			};

			// we use the full string for dpp::at_custom (also, emoji doesn't work with custom?)
			const std::unordered_map<dpp::activity_type, std::string> activity_to_string
			{
				{ dpp::at_game, "Playing " },
				{ dpp::at_streaming, "Streaming " },
				{ dpp::at_listening, "Listening to " },
				{ dpp::at_watching, "Watching " },
				{ dpp::at_competing, "Competing in " },
			};
		public:
			dpp::presence_status status = dpp::ps_online;

			dpp::activity_type activity_type = dpp::at_listening;
			std::string activity = "%prefix%help | fixedphilip %version%";

			int update_rate_mins = 5;

			virtual nlohmann::json struct_to_json(const bulbtils::file::settings& save_settings) const override final
			{
				if (activity_type == dpp::at_custom)
				{
					return
					{
						{ "status", status_to_string.at(status) },
						{ "activity", activity },
						{ "update_rate_mins", update_rate_mins },
					};
				}
				else
				{
					return
					{
						{ "status", status_to_string.at(status) },
						{ "activity", activity_to_string.at(activity_type) + activity },
						{ "update_rate_mins", update_rate_mins },
					};
				}
			}
			virtual bool json_to_struct(const nlohmann::json& data, const bulbtils::file::settings& load_settings) override final
			{
				std::string status_string;
				if (json_try_at(data, load_settings, "status", status_string, true))
				{
					auto it = std::find_if(status_to_string.begin(), status_to_string.end(), [&status_string](const auto& pair)
					{
						return pair.second == status_string;
					});
					if (it == status_to_string.end())
					{
						load_settings.error("invalid 'status' (reverting to default) - must be either one of: offline, online, dnd, idle, invisible");
					}
					else
					{
						status = it->first;
					}
				}

				std::string activity_string;
				if (json_try_at(data, load_settings, "activity", activity_string, true))
				{
					auto it = std::find_if(activity_to_string.begin(), activity_to_string.end(), [&activity_string](const auto& pair)
					{
						return activity_string.starts_with(pair.second);
					});
					if (it == activity_to_string.end())
					{
						// no special activity prefix, assume custom status
						activity_type = dpp::at_custom;
						activity = activity_string;
					}
					else
					{
						// special activity prefix found, assign activity and remove the prefix accordingly
						activity_type = it->first;
						activity = activity_string.substr(it->second.length());
					}
				}

				if (json_try_at(data, load_settings, "update_rate_mins", update_rate_mins, true) && update_rate_mins < 0)
				{
					load_settings.error("'update_rate_mins' is out of bounds - reverting to default");
					update_rate_mins = 5;
				}

				// partial load is fine
				return true;
			}
		};

		bot* owner = nullptr;

		presence_config config;

		dpp::timer timer_handle = SIZE_MAX;
		dpp::event_handle ready_handle = SIZE_MAX;

		void update_presence()
		{
			const std::vector<std::pair<std::string, std::string>> token_conversion
			{
				{ "%prefix%", owner->old_style_commands_enabled() ? owner->settings().prefix : "/" },
				{ "%version%", std::to_string(FIXEDPHILIP_BUILD_VERSION_NUM) },
			};

			std::string activity = config.activity;
			dpp::presence_status status = config.status;
			dpp::activity_type type = config.activity_type;

			for (int i = 0; i < token_conversion.size(); i++)
			{
				bulbtils::string::inplace::replace_all(activity, token_conversion[i].first, token_conversion[i].second);
			}
			owner->set_presence(dpp::presence(status, type, activity));
		}

		void ready_event(const dpp::ready_t& event)
		{
			if (dpp::run_once<struct presence_ready_event_init>())
			{
				int update_rate_mins = config.update_rate_mins;
				if (update_rate_mins > 0)
				{
					timer_handle = owner->start_timer([this](const dpp::timer& timer) { update_presence(); }, 60 * update_rate_mins);
				}
				update_presence();
			}
		}

		virtual bool init(bot& bot) override final
		{
			owner = &bot;

			bulbtils::file::settings config_settings
			{
				.filename = "presence.json",
				.create_if_not_found = true,
			};
			bot.append_loggers(config_settings);

			auto result = config.load(config_settings);
			if (result != bulbtils::file::r_success && result != bulbtils::file::r_file_not_found)
			{
				return false;
			}

			ready_handle = bot.on_ready.attach([this](const dpp::ready_t& e) { ready_event(e); });
			return true;
		}

		virtual void destroy(bot& bot) override final
		{
			if (timer_handle != SIZE_MAX)
			{
				bot.stop_timer(timer_handle);
			}
			bot.on_ready.detach(ready_handle);
		}
	public:
		presence_module() : module("presence") {}
	};
	static presence_module instance;
}