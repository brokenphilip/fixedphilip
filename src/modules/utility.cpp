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
                event.reply(dpp::message(std::format(":x: **| Failed to get `{}`'s avatar.**", user.username)).set_flags(dpp::m_ephemeral));
                co_return;
            }
            event.reply(dpp::message(std::format(":frame_photo: **| `{}`'s avatar is:** {}", user.username, avatar)).set_flags(dpp::m_ephemeral));
        }

        static dpp::task<void> run_get_banner(const run_event& event)
        {
            auto result = co_await event.get_bot()->co_user_get(event.get_user_context_menu()->get_user().id);
            if (result.is_error())
            {
                event.reply(dpp::message(std::format(":x: **| Failed to get `{}`'s additional user info.**", event.get_user_context_menu()->get_user().username)).set_flags(dpp::m_ephemeral));
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
                    event.reply(dpp::message(std::format(":x: **| Failed to get `{}`'s banner.**", user_identified.username)).set_flags(dpp::m_ephemeral));
                    co_return;
                }
            }
            event.reply(dpp::message(std::format(":frame_photo: **| `{}`'s banner is:** {}", user_identified.username, banner)).set_flags(dpp::m_ephemeral));
        }

        static dpp::task<void> run_extract_emojis(const run_event& event)
        {
            auto extract_emojis = [](const std::string& message) -> std::vector<std::string>
            {
                static std::regex emoji_regex("<(a)?:([^:]+):([0-9]+)>");

                std::vector<std::string> emojis;
                for (auto it = std::sregex_iterator(message.begin(), message.end(), emoji_regex); it != std::sregex_iterator(); it++)
                {
                    auto& smatch = *it;

                    auto animated = smatch[1].matched;
                    auto name = smatch[2].str();
                    auto id = smatch[3].str();

                    dpp::emoji emoji(name, id, animated ? dpp::e_animated : 0);
                    
                    std::string emoji_extracted = std::format("`<{}:{}:{}>` - <{}>", animated ? "a" : "", name, id, emoji.get_url(4096));
                    if (std::find(emojis.begin(), emojis.end(), emoji_extracted) == emojis.end())
                    {
                        emojis.push_back(emoji_extracted);
                    }
                }
                return emojis;
            };

            std::regex emoji_regex("<(a)?:([^:]+):([0-9]+)>");

            if (auto message_ctx_menu = event.get_message_context_menu())
            {
                auto results = extract_emojis(message_ctx_menu->get_message().content);

                if (results.empty())
                {
                    event.reply(dpp::message(":frame_photo: **| No emojis found in this message.**").set_flags(dpp::m_ephemeral));
                }
                else
                {
                    std::string reply_message = std::format(":frame_photo: **| Extracted {} emoji{} from this message:**", results.size(), results.size() == 1 ? "" : "s");
                    for (int i = 0; i < results.size(); i++)
                    {
                        reply_message += std::format("\n{}. {}", i + 1, results[i]);
                    }
                    event.reply(dpp::message(reply_message).set_flags(dpp::m_ephemeral));
                }
            }
            //else if (auto user_ctx_menu = event.get_user_context_menu())
            //{
            //    auto user = user_ctx_menu->get_user();
            //}
            co_return;
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command get_avatar("get avatar", dpp::ctxm_user, bot.me.id, run_get_avatar);
            command get_banner("get banner", dpp::ctxm_user, bot.me.id, run_get_banner);

            command extract_emojis_message("extract emojis", dpp::ctxm_message, bot.me.id, run_extract_emojis);
            //command extract_emojis_user("extract emojis", dpp::ctxm_user, bot.me.id, run_extract_emojis);

            return { get_avatar, get_banner, extract_emojis_message };
        }
    public:
        utility_module() : module("utility") {}
    };
    static utility_module instance;
}