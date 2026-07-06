#pragma once

//#include <format>

#include <dpp/dpp.h>

#include <fixedphilip/utils/file.h>
#include <fixedphilip/utils/string_literal.h>
#include <fixedphilip/log.h>

#define FIXEDPHILIP_DEFAULT_TOKEN "your_bot_token_here"

namespace fixedphilip::discord
{
	class bot
	{
	public:
		struct config : public fixedphilip::file::json_pretty_print
		{
			std::string token = FIXEDPHILIP_DEFAULT_TOKEN;
			std::string prefix = "fp!";

			inline virtual nlohmann::json struct_to_json() override final
			{
				return
				{
					{ "token", token },
					{ "prefix", prefix },
				};
			}
			inline virtual bool json_to_struct(const nlohmann::json& data) override final
			{
				try_at(data, "token", token);
				try_at(data, "prefix", prefix);

				// partial load is fine
				return true;
			}

			bool load_from_file(const std::string& filename);
		};
	private:
		static inline bot* instance_ = nullptr;

		config config_;
		dpp::cluster cluster_;
		dpp::user app_owner_;

		template <typename T>
		using event_t = dpp::task<void>(const T& event);
		static event_t<dpp::message_create_t> on_message_create;
		static event_t<dpp::ready_t> on_ready;
		static event_t<dpp::slashcommand_t> on_slashcommand;

		void get_app_info_async();
		void register_events();
	public:
		inline bot(const config& config) : config_(config), cluster_(config.token, dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members) { instance_ = this; }
		inline ~bot() { instance_ = nullptr; }

		inline bool setup()
		{
			get_app_info_async();
			register_events();
			return true;
		}
		inline void run_blocking() { cluster_.start(); }

		inline auto& cluster() { return cluster_; }
		inline auto& app_owner() { return app_owner_; }

		static inline auto get_instance() { return instance_; }
	};

	template <typename T>
	const T* get_if(const std::string& log_prefix, const dpp::confirmation_callback_t& result)
	{
		if (auto value = std::get_if<T>(&result.value))
		{
			return value;
		}

		if (result.is_error())
		{
			fixedphilip::log::error(std::format("{}: {}", log_prefix, result.get_error().human_readable));
		}
		else
		{
			fixedphilip::log::error(std::format("{}: unknown error (wrong result.value type)", log_prefix));
		}
		return nullptr;
	}
}