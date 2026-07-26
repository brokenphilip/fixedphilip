#pragma once

//#include <format>
#include <variant>

#include <dpp/dpp.h>

#include <fixedphilip/file.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/named_node.h>

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

namespace fixedphilip::discord
{
	// The base of a fixedphilip bot/cluster, expanded to support:
	// - Modules
	// -
	class bot : public dpp::cluster
	{
	public:
		// Settings stored and used inside each fixedphilip bot/cluster
		// Modify this data structure to add new settings to the config/bot classes
		// (instead of modifying them individually)
		struct settings
		{
			std::string prefix = "fp!";
			dpp::presence_status presence_status = dpp::ps_online;

			dpp::activity_type activity_type = dpp::at_listening;
			std::string presence_activity = "%prefix%help | fixedphilip %version%";

			int presence_update_rate_mins = 5;

			// list of disabled modules, separated by maybe not space but some other shit
			// like , or ; or | or idfk
		};

		// Configuration file structure
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
			settings settings;

			virtual nlohmann::json struct_to_json() override final;
			virtual bool json_to_struct(const nlohmann::json& data) override final;

			bool load_from_file(const std::string& filename);
		};

		class command : public dpp::slashcommand
		{
		public:
			struct run_event : public std::variant<dpp::slashcommand_t, dpp::message_create_t>
			{
				inline auto get_slash_command() const { return std::get_if<dpp::slashcommand_t>(this); }
				inline auto get_message_create() const { return std::get_if<dpp::message_create_t>(this); }

				const dpp::event_dispatch_t& event_dispatch() const;

				void reply(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const;
				inline void reply(const std::string& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const { reply(dpp::message(msg), callback); }

				dpp::async<dpp::confirmation_callback_t> co_reply(const dpp::message& msg) const;
				inline dpp::async<dpp::confirmation_callback_t> co_reply(const std::string& msg) const { return co_reply(dpp::message(msg)); }

				void thinking_start() const;
				dpp::async<dpp::confirmation_callback_t> co_thinking_start() const;

				void thinking_end(const dpp::message& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const;
				inline void thinking_end(const std::string& msg, dpp::command_completion_event_t callback = dpp::utility::log_error()) const { thinking_end(dpp::message(msg), callback); }

				void reply_not_impl_use_other() const;
			};

			using run_fn = std::function<dpp::task<void>(const run_event&)>;
		private:
			run_fn run_fn_;
		public:
			inline command(const std::string& name, const std::string& description, const dpp::snowflake application_id, run_fn run_fn)
				: dpp::slashcommand(name, description, application_id), run_fn_(run_fn) {}

			inline command(const std::string& name, const dpp::slashcommand_contextmenu_type type, const dpp::snowflake application_id, run_fn run_fn)
				: dpp::slashcommand(name, type, application_id), run_fn_(run_fn) {}

			inline auto get_run_fn() { return run_fn_; }
			inline dpp::task<void> run(const run_event& event) { co_await run_fn_(event); }

			template <typename T>
			static T try_get_parameter(const dpp::slashcommand_t& command, const std::string& param_name, T default_value)
			{
				if (auto param = command.get_parameter(param_name); auto value = std::get_if<T>(&param))
				{
					return *value;
				}
				return default_value;
			}
		};

		class module : public fixedphilip::utils::named_node<module>
		{
			// TODO: command which lists all modules, active and inactive
			const char* description_;
		public:
			inline module(const char* name, const char* description) : named_node<module>(name), description_(description) {}
			inline virtual ~module() {}

			inline virtual bool init(bot& bot) { return true; }
			inline virtual std::vector<command> commands() { return {}; }
			inline virtual void destroy(bot& bot) {}

			inline auto description() { return description_; }
		};
	private:
		std::vector<module*> loaded_modules;
		std::vector<command> module_commands;

		settings settings_;
		std::shared_mutex settings_mutex_;

		dpp::user app_owner_;
		std::shared_mutex app_owner_mutex;

		std::unordered_map<std::string, dpp::snowflake> slash_command_snowflakes;
		std::shared_mutex slash_command_snowflakes_mutex;

		static void logger(const dpp::log_t&);

		template <typename T>
		using event_t = dpp::task<void>(const T& event);
		static event_t<dpp::log_t> log_event;
		static event_t<dpp::message_create_t> message_create_event;
		static event_t<dpp::ready_t> ready_event;

		dpp::task<void> init_commands();
		//dpp::task<void> init_presence(bool delme);
		//void update_presence(bool delme);

		void fetch_app_info_async();
	public:
		bot(const std::string& token, const settings& settings, uint32_t intents = dpp::i_default_intents,
			uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, bool compressed = true,
			dpp::cache_policy_t policy = dpp::cache_policy::cpol_default, uint32_t pool_threads = std::thread::hardware_concurrency() / 2);

		virtual ~bot();

		// Returns the bot's settings. Make sure to lock its mutex before reading/writing
		inline auto& get_settings() { return settings_; }

		// Returns the bot's settings mutex. Use shared_lock for reads and unique_lock for writes
		inline auto& get_settings_mutex() { return settings_mutex_; }

		// Returns a copy of the dpp::user who owns this app/instance
		inline auto app_owner() 
			{ std::shared_lock _(app_owner_mutex); return app_owner_; }

		// Given a slash command, returns its snowflake
		inline auto slash_command_snowflake(const std::string& slash_command) 
			{ std::shared_lock _(slash_command_snowflakes_mutex); return slash_command_snowflakes[slash_command]; }

		// Server and user counts, for the servers the bot is currently in, as well as all the (guild and user install) users it can see
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
			auto error = std::format("{}: {}", log_prefix, result.get_error().human_readable);
			auto cluster = result.bot;
			if (cluster)
			{
				result.bot->log(dpp::ll_error, error);
			}
			else
			{
				fixedphilip::log::error(error);
			}
			return nullptr;
		}

		if (auto value = std::get_if<T>(&result.value))
		{
			return value;
		}

		// TODO: is this unreachable?
		auto error = std::format("{}: unknown error (wrong result.value type)", log_prefix);
		auto cluster = result.bot;
		if (cluster)
		{
			result.bot->log(dpp::ll_error, error);
		}
		else
		{
			fixedphilip::log::error(error);
		}
		return nullptr;
	}
}