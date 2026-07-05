#pragma once

namespace fixedphilip::utils
{
	// Wrapper for string literals, primarily used for template arguments
	template <size_t N>
	struct string_literal
	{
		char str[N]{ 0 };
		constexpr string_literal(const char(&str)[N])
		{
			for (int i = 0; i < N; i++)
			{
				this->str[i] = str[i];
			}
		}
	};
}