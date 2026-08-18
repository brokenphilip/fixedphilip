#include <discofloor/bot.h>

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

        virtual std::vector<command> commands(bot& bot) override final
        {
            command get_avatar("get avatar", dpp::ctxm_user, bot.me.id, run_get_avatar);
            command get_banner("get banner", dpp::ctxm_user, bot.me.id, run_get_banner);
            return { get_avatar, get_banner };
        }
    public:
        utility_module() : module("utility") {}
    };
    static utility_module instance;
}