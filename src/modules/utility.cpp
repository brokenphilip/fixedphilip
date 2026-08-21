#include <discofloor/bot.h>

#include <regex>

namespace discofloor
{
    class utility_module : public module
    {
        static dpp::task<void> run_get_avatar(const run_event& event)
        {
            auto& user = event.get_user_context_menu()->get_user();
            auto avatar = user.get_avatar_url(4096);
            if (avatar.empty())
            {
                event.reply(dpp::message(std::format(":x: **| Failed to get avatar for \"{}\".**", discofloor::get_username(user))).set_flags(dpp::m_ephemeral));
                co_return;
            }
            event.reply(dpp::message(std::format(":frame_photo: **| Avatar for \"{}\":** {}", discofloor::get_username(user), avatar)).set_flags(dpp::m_ephemeral));
        }

        static dpp::task<void> run_get_banner(const run_event& event)
        {
            auto result = co_await event.get_bot()->co_user_get(event.get_user_context_menu()->get_user().id);
            if (result.is_error())
            {
                event.reply(dpp::message(std::format(":x: **| Failed to get additional user info for \"{}\".**", discofloor::get_username(event.get_user_context_menu()->get_user()))).set_flags(dpp::m_ephemeral));
                co_return;
            }
            auto user_identified = result.get<dpp::user_identified>();

            auto banner = user_identified.get_banner_url(4096);
            if (banner.empty())
            {
                if (user_identified.accent_color)
                {
                    banner = std::format("`#{:06x}`", user_identified.accent_color);
                }
                else
                {
                    event.reply(dpp::message(std::format(":x: **| Failed to get banner for \"{}\".**", discofloor::get_username(user_identified))).set_flags(dpp::m_ephemeral));
                    co_return;
                }
            }
            event.reply(dpp::message(std::format(":frame_photo: **| Banner for \"{}\":** {}", discofloor::get_username(user_identified), banner)).set_flags(dpp::m_ephemeral));
        }

        static dpp::task<void> run_extract_emojis(const run_event& event)
        {
            struct extracted_emoji
            {
                dpp::emoji emoji;
                bool unknown_if_animated;

                extracted_emoji(const dpp::emoji& emoji, bool unknown_if_animated)
                    : emoji(emoji), unknown_if_animated(unknown_if_animated) {}
            };
            static std::regex emoji_regex("<(a)?:([^:]+):([0-9]+)>");

            // Extract emojis from a string (chat message, user status, user about me...)
            auto extract_emojis = [](const std::string& str) -> std::vector<extracted_emoji>
            {
                std::vector<extracted_emoji> emojis;
                for (auto it = std::sregex_iterator(str.begin(), str.end(), emoji_regex); it != std::sregex_iterator(); it++)
                {
                    auto& smatch = *it;

                    auto animated = smatch[1].matched;
                    auto name = smatch[2].str();
                    auto id = smatch[3].str();

                    emojis.emplace_back(dpp::emoji(name, id, animated ? dpp::e_animated : 0), false);
                }
                return emojis;
            };

            if (auto message_ctx_menu = event.get_message_context_menu())
            {
                auto message = message_ctx_menu->get_message();

                auto extracted_emojis = extract_emojis(message.content);
                for (auto& reaction : message.reactions)
                {
                    auto id = reaction.emoji_id;
                    if (id != 0)
                    {
                        extracted_emojis.emplace_back(dpp::emoji(reaction.emoji_name, id), true);
                    }
                }

                if (extracted_emojis.empty())
                {
                    event.reply(dpp::message(":frame_photo: **| No emojis found in this message.**").set_flags(dpp::m_ephemeral));
                    co_return;
                }

                std::string reply_message = std::format(":frame_photo: **| Extracted {} emoji{} from this message:**", extracted_emojis.size(), extracted_emojis.size() == 1 ? "" : "s");
                for (int i = 0; i < extracted_emojis.size(); i++)
                {
                    auto& emoji = extracted_emojis[i].emoji;
                    if (!extracted_emojis[i].unknown_if_animated)
                    {
                        reply_message += std::format("\n{}. \"<{}\\:{}\\:{}>\" - <{}>", i + 1, emoji.is_animated() ? "a" : "", emoji.name, std::to_string(emoji.id), emoji.get_url(4096));
                    }
                    else
                    {
                        auto emoji_animated = emoji;
                        emoji_animated.flags |= dpp::e_animated;

                        reply_message += std::format(
                            "\n{}. Reaction emoji (unknown if animated or not)"
                            "\n  - \"<\\:{}\\:{}>\" - <{}>"
                            "\n  - \"<a\\:{}\\:{}>\" - <{}>",
                            i + 1,
                            emoji.name, std::to_string(emoji.id), emoji.get_url(4096, dpp::i_png, false),
                            emoji_animated.name, std::to_string(emoji_animated.id), emoji_animated.get_url(4096, dpp::i_gif, true));
                    }
                        
                }
                event.reply(dpp::message(reply_message).set_flags(dpp::m_ephemeral));
            }
            // uncomment when we're able to get emojis from status and aboutme
            //else if (auto user_ctx_menu = event.get_user_context_menu())
            //{
            //    auto user = user_ctx_menu->get_user();
            //}
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command get_avatar("get avatar", dpp::ctxm_user, bot.me.id, run_get_avatar);
            command get_banner("get banner", dpp::ctxm_user, bot.me.id, run_get_banner);

            command extract_emojis_message("extract emojis", dpp::ctxm_message, bot.me.id, run_extract_emojis);
            //command extract_emojis_user("extract emojis", dpp::ctxm_user, bot.me.id, run_extract_emojis);
            // uncomment when we're able to get emojis from status and aboutme

            return { get_avatar, get_banner, extract_emojis_message };
        }
    public:
        utility_module() : module("utility") {}
    };
    static utility_module instance;
}