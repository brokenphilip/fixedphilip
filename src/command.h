#pragma once

namespace dpp
{
	class slashcommand_t;
}

namespace fixedphilip
{
	class Command
	{
	public:
		using RunFn = void(const dpp::slashcommand_t& event);
	private:
		static inline Command* first = nullptr;
		static inline Command* last = nullptr;

		Command* prev = nullptr;
		Command* next = nullptr;

		const char* name;
		const char* description;

		RunFn* run;
	public:
		Command(const char* name, const char* description, RunFn* run);
		~Command();

		inline auto GetPrev() { return prev; }
		inline auto GetNext() { return next; }

		inline auto GetName() { return name; }
		inline auto GetDescription() { return description; }

		inline auto Run(const dpp::slashcommand_t& event) { run(event); }

		static inline auto GetFirst() { return first; }
		static inline auto GetLast() { return last; }
	};
}

#define FIXEDPHILIP_COMMAND(name, description) static fixedphilip::Command name##_##(#name, description, &name::Run)