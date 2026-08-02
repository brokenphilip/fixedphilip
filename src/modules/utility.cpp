#include <fixedphilip/discord.h>

namespace fixedphilip::discord
{
    class utility_module : public bot::module
    {
        static dpp::task<void> run_get_avatar(const bot::command::run_event& event)
        {
            auto cluster = event.get_bot();
            if (!cluster)
            {
                event.reply(":warning: **| An internal error occurred.**");
                fixedphilip::log::error("run_get_avatar: bot was null");
                co_return;
            }

            auto& user = event.get_user_context_menu()->get_user();
            auto avatar = user.get_avatar_url(4096);
            if (avatar.empty())
            {
                event.reply(dpp::message(std::format(":x: **| Failed to get `{}`'s avatar.**", user.username)).set_flags(dpp::m_ephemeral));
                co_return;
            }
            event.reply(dpp::message(std::format(":frame_photo: **| `{}`'s avatar is:** {}", user.username, avatar)).set_flags(dpp::m_ephemeral));
        }

        static dpp::task<void> run_get_banner(const bot::command::run_event& event)
        {
            auto cluster = event.get_bot();
            if (!cluster)
            {
                event.reply(dpp::message(":warning: **| An internal error occurred.**").set_flags(dpp::m_ephemeral));
                fixedphilip::log::error("run_get_banner: bot was null");
                co_return;
            }

            auto result = co_await cluster->co_user_get(event.get_user_context_menu()->get_user().id);
            if (auto user_identified = fixedphilip::discord::get_if<dpp::user_identified>("run_get_banner, co_user_get", result))
            {
                auto banner = user_identified->get_banner_url(4096);
                if (banner.empty())
                {
                    if (user_identified->accent_color)
                    {
                        banner = std::format("`#{:06x}`", user_identified->accent_color);
                    }
                    else
                    {
                        event.reply(dpp::message(std::format(":x: **| Failed to get `{}`'s banner.**", user_identified->username)).set_flags(dpp::m_ephemeral));
                        co_return;
                    }
                }
                event.reply(dpp::message(std::format(":frame_photo: **| `{}`'s banner is:** {}", user_identified->username, banner)).set_flags(dpp::m_ephemeral));
            }
        }

        virtual std::vector<bot::command> commands(bot& bot) override final
        {
            bot::command get_avatar("get avatar", dpp::ctxm_user, bot.me.id, run_get_avatar);
            bot::command get_banner("get banner", dpp::ctxm_user, bot.me.id, run_get_banner);
            return { get_avatar, get_banner };
        }
    public:
        utility_module() : bot::module("utility", "Utility commands") {}
    };
    static utility_module instance;
}