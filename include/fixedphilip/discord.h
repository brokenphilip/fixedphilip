#pragma once

#include <dpp/dispatcher.h> // dpp::confirmation_callback_t, dpp::command_completion_event_t
#include <dpp/restresults.h> // dpp::confirmable_t

#include <fixedphilip/utils/string_literal.h>

namespace fixedphilip::discord
{
	inline dpp::user instance_owner;

	template <typename T>
	class command_completion_event : public dpp::command_completion_event_t
	{
	public:
		using callback_t = std::function<void(const T&)>;
	private:
		callback_t callback_;
		std::string log_prefix_;
	public:
		command_completion_event(const std::string& log_prefix, callback_t callback) : log_prefix_(log_prefix), callback_(callback) {}

		void operator()(const dpp::confirmation_callback_t& result)
		{
			if (auto value = std::get_if<T>(&result.value))
			{
				callback_(*value);
			}
			else
			{
				if (result.is_error())
				{
					fixedphilip::log::error(std::format("{}: {}", log_prefix_, result.get_error().human_readable));
				}
				else
				{
					fixedphilip::log::error(std::format("{}: unknown error (wrong type of result.value)", log_prefix_));
				}
			}
		}
	};
}