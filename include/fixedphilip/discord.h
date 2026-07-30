#pragma once

//#include <format>

#include <dpp/dpp.h>

#include <fixedphilip/file.h>
#include <fixedphilip/log.h>

#include <fixedphilip/utils/named_node.h>
#include <fixedphilip/utils/time.h>

#include <variant>

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

namespace fixedphilip::discord
{
	// Settings stored and used inside each fixedphilip bot/cluster
	// These settings can be loaded from and saved to a config file, see the config struct below
	// Modify this data structure to add new settings to the config/bot classes
	struct bot_settings
	{
		// Chat prefix for old-style commands (can be set to blank to disable)
		std::string prefix = "fp!";

		// List of disabled modules by name
		// Each name accepts one wildcard ('*')
		std::vector<std::string> disabled_modules = {};

		inline nlohmann::json struct_to_json()
		{
			return
			{
				{ "prefix", prefix },
				{ "disabled_modules", disabled_modules },
			};
		}

		inline bool json_to_struct(const nlohmann::json& data)
		{
			fixedphilip::file::json_try_at(data, "prefix", prefix);
			fixedphilip::file::json_try_at(data, "disabled_modules", disabled_modules);

			return true;
		}
	};

	// The base of a fixedphilip bot/cluster, expanded to support:
	// - Modules
	// -
	class bot : public dpp::cluster
	{
	public:
		// Configuration file structure which can be used to load fixedphilip bot/cluster settings
		struct config : public fixedphilip::file::json_pretty_print
		{
			std::string token = FIXEDPHILIP_DEFAULT_TOKEN;
			bot_settings settings;

			virtual nlohmann::json struct_to_json() override final;
			virtual bool json_to_struct(const nlohmann::json& data) override final;

			// Use this instead of load() to load the config
			// If it returns true, proceed with instantiating the bot
			bool load_from_file(const std::string& filename);
		};

		// Slash command wrapper with an accompanying run function that supports:
		// - old-style (chat prefix) commands, unless disabled (blank prefix)
		// - "CHAT_INPUT" ie. regular (chat) slash commands
		// - "USER" ie. (right-click) user context menu commands
		// - "MESSAGE" ie. (right-click) message context menu commands
		class command : public dpp::slashcommand
		{
		public:
			// check cormands.txt it woul be very cool i tink
			struct run_event : public std::variant<dpp::slashcommand_t, dpp::message_create_t, dpp::message_context_menu_t, dpp::user_context_menu_t>
			{
				bot* get_bot() const;

				inline auto get_slash_command() const { return std::get_if<dpp::slashcommand_t>(this); }
				inline auto get_message_create() const { return std::get_if<dpp::message_create_t>(this); }
				inline auto get_message_context_menu() const { return std::get_if<dpp::message_context_menu_t>(this); }
				inline auto get_user_context_menu() const { return std::get_if<dpp::user_context_menu_t>(this); }

				const dpp::interaction_create_t* get_interaction_create() const;

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

				template <typename T>
				T try_get_command_parameter(const std::string& param_name, T default_value) const
				{
					auto interaction = get_interaction_create();
					if (!interaction)
					{
						// todo
						return default_value;
					}
					if (auto param = interaction->get_parameter(param_name); auto value = std::get_if<T>(&param))
					{
						return *value;
					}
					return default_value;
				}
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

		//
		class module : public fixedphilip::utils::named_node<module>
		{
			// TODO: command which lists all modules, active and inactive
			const char* description_;
		public:
			inline module(const char* name, const char* description) : named_node<module>(name), description_(description) {}
			inline virtual ~module() {}

			inline virtual bool init(bot& bot) { return true; }
			inline virtual std::vector<command> commands(bot& bot) { return {}; }
			inline virtual void destroy(bot& bot) {}

			inline auto description() { return description_; }
		};
	private:
		// Constructed on startup and read-only - no need for a mutex
		bot_settings settings_;

		// Constructed on startup and read-only - no need for a mutex
		fixedphilip::utils::time::raii_stopwatch bot_uptime_;
		std::chrono::system_clock::time_point bot_start_time_ = std::chrono::system_clock::now();

		std::vector<module*> loaded_modules_;
		std::shared_mutex loaded_modules_mutex_;

		struct module_command : public command
		{
			dpp::event_handle event_handles[2] { SIZE_MAX };
			dpp::snowflake snowflake {};
			
			inline module_command(const command& cmd) : command(cmd) {}
		};
		std::vector<module_command> module_commands_;
		std::shared_mutex module_commands_mutex_;

		dpp::user app_owner_;
		std::shared_mutex app_owner_mutex_;

		std::atomic_bool ready_init_done_ = false;

		static void logger(const dpp::log_t&);

		template <typename T>
		using event_t = dpp::task<void>(const T& event);
		static event_t<dpp::log_t> log_event;
		static event_t<dpp::ready_t> ready_event;

		void create_commands_async();
		void fetch_app_info_async();
	public:
		bot(const std::string& token, const bot_settings& settings, uint32_t intents = dpp::i_default_intents,
			uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, bool compressed = true,
			dpp::cache_policy_t policy = dpp::cache_policy::cpol_default, uint32_t pool_threads = std::thread::hardware_concurrency() / 2);

		virtual ~bot();

		// Returns a copy of the list of loaded modules
		// Should you decide to modify a loaded module, you are responsible for its thread safety
		inline auto loaded_modules()
			{ std::shared_lock _(loaded_modules_mutex_); return loaded_modules_; }

		// Returns a copy of the bot's settings
		inline auto settings() { return settings_; }

		// Returns a copy of the dpp::user who owns this app/instance
		inline auto app_owner() 
			{ std::shared_lock _(app_owner_mutex_); return app_owner_; }

		// Given a slash command name, returns its snowflake
		// Only works for CHAT_INPUT commands (context menu commands will not work)
		dpp::snowflake slash_command_snowflake(const std::string& slash_command);

		// Add (late-load) a module to the bot - returns true on success
		// Returns false if called too early (must be after on_ready_init)
		// Also returns false if the module failed to load or is disabled by config/settings file
		bool add_module(module* module_to_add);

		// Remove (early-unload) a module from the bot - returns true on success
		// Returns false if this module is not loaded
		bool remove_module(module* module_to_remove);

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
		auto cluster = result.bot;
		if (result.is_error())
		{
			auto error = std::format("{}: {}", log_prefix, result.get_error().human_readable);
			if (cluster)
			{
				cluster->log(dpp::ll_error, error);
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