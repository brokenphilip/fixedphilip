#pragma once

#include <string>

namespace fixedphilip::utils::string
{
    // Wrapper for string literals, primarily used for template arguments
    template <size_t N>
    struct literal
    {
        char str[N]{ 0 };
        constexpr literal(const char(&str)[N])
        {
            for (int i = 0; i < N; i++)
            {
                this->str[i] = str[i];
            }
        }
    };

    void replace_all(std::string& str, const std::string& from, const std::string& to)
    {
        if (from.empty())
        {
            return;
        }

        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }
}