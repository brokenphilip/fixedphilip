#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>

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

    // Replace all instances of "from" with "to" in "str"
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

    // Split "str" into individual tokens, by any amount of whitespace
    inline std::vector<std::string> split_by_whitespace(const std::string& str)
    {
        std::istringstream iss(str);
        return { (std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>() };
    }

    namespace inplace
    {
        // Replaces all characters within a string to their lowercase equivalents
        inline void to_lowercase(std::string& source_dest)
        {
            auto to_lowercase_fn = [](unsigned char c) { return std::tolower(c); };
            std::ranges::transform(source_dest, source_dest.begin(), to_lowercase_fn);
        }

        // Removes all extra spaces (and spaces only, NOT other whitespace) from a string
        inline void remove_extra_spaces(std::string& source_dest)
        {
            auto both_are_spaces = [](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); };
            auto new_end = std::unique(source_dest.begin(), source_dest.end(), both_are_spaces);
            source_dest.erase(new_end, source_dest.end());
        }
    }
}