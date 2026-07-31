#include <fixedphilip/discord.h>
#include <fixedphilip/file.h>
#include <fixedphilip/build.h>

#include <fixedphilip/utils/string.h>

namespace fixedphilip::discord
{
	class presence_module : public bot::module
	{
		class config : public fixedphilip::file::json_pretty_print
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

			virtual nlohmann::json struct_to_json() override final
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
			virtual bool json_to_struct(const nlohmann::json& data) override final
			{
				std::string status_string;
				if (fixedphilip::file::json_try_at(data, "status", status_string))
				{
					auto it = std::find_if(status_to_string.begin(), status_to_string.end(), [&status_string](const auto& pair)
					{
						return pair.second == status_string;
					});
					if (it == status_to_string.end())
					{
						fixedphilip::log::error("invalid 'status' (reverting to default) - must be either one of: offline, online, dnd, idle, invisible");
					}
					else
					{
						status = it->first;
					}
				}

				std::string activity_string;
				if (fixedphilip::file::json_try_at(data, "activity", activity_string))
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

				if (fixedphilip::file::json_try_at(data, "update_rate_mins", update_rate_mins) && update_rate_mins < 0)
				{
					fixedphilip::log::error("'update_rate_mins' is out of bounds - reverting to default");
					update_rate_mins = 5;
				}

				// partial load is fine
				return true;
			}
		};

		presence_module::config config;
		//std::shared_mutex config_mutex;
		dpp::timer timer = SIZE_MAX;

		void update_presence(bot& bot)
		{
			const std::vector<std::pair<std::string, std::string>> token_conversion
			{
				{ "%prefix%", bot.settings().prefix },
				{ "%version%", std::to_string(FIXEDPHILIP_BUILD_VERSION_NUM) },
			};

			std::string activity = config.activity;
			dpp::presence_status status = config.status;
			dpp::activity_type type = config.activity_type;
			//{
			//	std::shared_lock _(config_mutex);
			//	activity = config.activity;
			//	status = config.status;
			//	type = config.activity_type;
			//}

			for (int i = 0; i < token_conversion.size(); i++)
			{
				fixedphilip::utils::string::replace_all(activity, token_conversion[i].first, token_conversion[i].second);
			}
			bot.set_presence(dpp::presence(status, type, activity));
		}

		void init_presence(bot& bot)
		{
			update_presence(bot);

			int update_rate_mins = config.update_rate_mins;
			//{
			//	std::shared_lock _(config_mutex);
			//	update_rate_mins = config.update_rate_mins;
			//}
			if (update_rate_mins > 0)
			{
				timer = bot.start_timer([this, &bot](const dpp::timer& timer) -> dpp::task<void>
				{
					update_presence(bot);
					co_return;
				},
				60 * update_rate_mins);
			}
		}

		static dpp::task<void> ready_event(const dpp::ready_t& event)
		{
			if (dpp::run_once<struct presence_ready_event_init>())
			{
				auto cluster = static_cast<bot*>(event.owner);
				if (!cluster)
				{
					fixedphilip::log::error("presence_ready_event_init: owner was null");
					co_return;
				}
				presence_module* presence = nullptr;
				for (auto& module : cluster->loaded_modules())
				{
					if (std::string("presence") == module->name())
					{
						presence = static_cast<presence_module*>(module);
						break;
					}
				}
				if (!presence)
				{
					cluster->log(dpp::ll_error, "presence_ready_event_init: presence module was null");
					co_return;
				}
				presence->init_presence(*cluster);
			}
		}

		virtual bool init(bot& bot) override final
		{
			{
				//std::unique_lock _(config_mutex);

				fixedphilip::file::settings config_settings
				{
					.filename = "presence.json",
					.create_if_not_found = true,
					.log = true,
				};

				auto result = config.load(config_settings);
				if (result != fixedphilip::file::r_success && result != fixedphilip::file::r_file_not_found)
				{
					return false;
				}
			}
			bot.on_ready.attach(ready_event);
			return true;
		}

		virtual void destroy(bot& bot) override final
		{
			int update_rate_mins = config.update_rate_mins;
			//{
			//	std::shared_lock _(config_mutex);
			//	update_rate_mins = config.update_rate_mins;
			//}
			if (update_rate_mins > 0)
			{
				bot.stop_timer(timer);
			}
		}
	public:
		presence_module() : bot::module("presence", "Manages the bot's activity/status presence") {}
	};
	static presence_module instance;
}