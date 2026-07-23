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

    inline void replace_all(std::string& str, const std::string& from, const std::string& to)
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

    namespace inplace
    {
        inline void to_lowercase(std::string& source_dest)
        {
            auto to_lowercase_fn = [](unsigned char c) { return std::tolower(c); };
            std::ranges::transform(source_dest, source_dest.begin(), to_lowercase_fn);
        }

        inline void remove_multi_whitespace(std::string& source_dest)
        {
            auto both_are_spaces = [](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); };
            auto new_end = std::unique(source_dest.begin(), source_dest.end(), both_are_spaces);
            source_dest.erase(new_end, source_dest.end());
        }
    }
}