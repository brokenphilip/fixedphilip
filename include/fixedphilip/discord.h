#pragma once

//#include <format>

#include <dpp/dpp.h>

#include <fixedphilip/file.h>
#include <fixedphilip/log.h>

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

namespace fixedphilip::discord
{
	class bot
	{
	public:
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
			std::string token = FIXEDPHILIP_DEFAULT_TOKEN;
			std::string prefix = "fp!";
			dpp::presence_status presence_status = dpp::ps_online;

			dpp::activity_type activity_type = dpp::at_listening;
			std::string presence_activity = "%prefix%help | fixedphilip %version%";

			int presence_update_rate_mins = 5;

			virtual nlohmann::json struct_to_json() override final;
			virtual bool json_to_struct(const nlohmann::json& data) override final;

			bool load_from_file(const std::string& filename);
		};
	private:
		static inline bot* instance_ = nullptr;

		config config_;
		dpp::cluster cluster_;
		dpp::user app_owner_;
		std::unordered_map<std::string, dpp::snowflake> slash_command_snowflakes_;

		template <typename T>
		using event_t = dpp::task<void>(const T& event);
		static event_t<dpp::message_create_t> on_message_create;
		static event_t<dpp::ready_t> on_ready;
		static event_t<dpp::slashcommand_t> on_slashcommand;

		dpp::task<void> init_commands();
		dpp::task<void> init_presence();
		void update_presence();

		void fetch_app_info_async();
		void register_events();
	public:
		inline bot(const config& config) : config_(config), cluster_(config.token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members) { instance_ = this; }
		inline ~bot() { instance_ = nullptr; }

		inline bool setup()
		{
			fetch_app_info_async();
			register_events();
			return true;
		}
		inline void run_blocking() { cluster_.start(); }

		inline const auto& prefix() { return config_.prefix; }
		inline auto& cluster() { return cluster_; }
		inline auto& app_owner() { return app_owner_; }
		inline auto slash_command_snowflake(const std::string& slash_command) { return slash_command_snowflakes_[slash_command]; }

		static inline auto get_instance() { return instance_; }

		struct counts
		{
			// Amount of servers we're currently in
			int servers = -1;

			// Amount of unique, non-bot users in the servers we're in
			int users = -1;

			// If true, approximate user counts are used instead of exact (unique, non-bot)
			bool users_fallback = false;

			// Amount of users that installed our app
			int user_installs = -1;

			// Total amount of users from servers and app installs
			// NOTE: if either values are invalid, this is invalid too
			int total_users = -1;
		};
		dpp::task<counts> co_get_counts();
	};

	template <typename T>
	const T* get_if(const std::string& log_prefix, const dpp::confirmation_callback_t& result)
	{
		if (result.is_error())
		{
			fixedphilip::log::error(std::format("{}: {}", log_prefix, result.get_error().human_readable));
			return nullptr;
		}

		if (auto value = std::get_if<T>(&result.value))
		{
			return value;
		}

		// TODO: is this unreachable?
		fixedphilip::log::error(std::format("{}: unknown error (wrong result.value type)", log_prefix));
		return nullptr;
	}
}