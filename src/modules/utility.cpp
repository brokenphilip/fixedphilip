#include <discofloor/bot.h>
#include <discofloor/utility.h>

#include <fixedphilip/math.h>

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

        static dpp::task<void> run_say(const run_event& event)
        {
            auto cluster = event.get_bot();
            if (event.command_invoker() != cluster->app_owner())
            {
                event.reply(":no_entry: **| Only the instance owner can run this command.**");
                co_return;
            }

            if (auto message_command = event.get_message_command())
            {
                static auto say_mention = cluster->get_command("say", dpp::ctxm_chat_input).value().get_mention();

                // can't prompt dialogs via old-style chat commands
                message_command->reply(":x: **| You must use " + say_mention + " instead**");
                co_return;
            }

            std::vector<dpp::component> components;

            auto& text_input = components.emplace_back();
            text_input.set_id("say_message");
            text_input.set_type(dpp::cot_text);
            text_input.set_label("What do I say?");
            text_input.set_text_style(dpp::text_paragraph);
            text_input.set_required(false); // sending empty messages will silently fail, it's fine

            auto& think_for = components.emplace_back();
            think_for.set_id("say_think_for");
            think_for.set_type(dpp::cot_text);
            think_for.set_label("How long do I think for?");
            think_for.set_text_style(dpp::text_paragraph);
            think_for.set_default_value("0 s");
            think_for.set_required(true);

            if (event.get_cmd_optional_param_value<bool>("files", false))
            {
                auto& attachments = components.emplace_back();
                attachments.set_id("say_attachments");
                attachments.set_type(dpp::cot_file_upload);
                attachments.set_label("Make me attach files");
                //attachments.set_required(false); // seems to be broken dpp side for now? hence why we need the files param
                attachments.set_min_values(1);
                attachments.set_max_values(10);
            }
            event.get_slash_command()->dialog(dpp::interaction_modal_response("say_modal", "Say", components));
        }

        dpp::event_handle form_submit_handle = SIZE_MAX;
        static dpp::task<void> form_submit_event(const dpp::form_submit_t& event)
        {
            if (!event.custom_id.starts_with("say_modal"))
            {
                co_return;
            }

            dpp::message msg(std::get<std::string>(event.components[0].value));

            fixedphilip::math::number_t think_for = 0.0;
            try
            {
                fixedphilip::math::conversion::convert(std::get<std::string>(event.components[1].value), "s", -1, false, nullptr, nullptr, &think_for);
            }
            catch (std::exception& e)
            {
                event.reply(dpp::message(std::format(":x: **| Failed to parse thinking duration:** {}", e.what())).set_flags(dpp::m_ephemeral));
                co_return;
            }

            for (auto& [snowflake, attachment] : event.command.resolved.attachments)
            {
                auto result = co_await event.owner->co_request(attachment.url, dpp::m_get);
                if (result.status != 200)
                {
                    event.reply(dpp::message(":x: **| Failed to download at least one attachment**").set_flags(dpp::m_ephemeral));
                    co_return;
                }
                msg.add_file(attachment.filename, result.body, attachment.content_type);
            }

            if (think_for > 0.0)
            {
                co_await event.co_thinking();
                co_await event.owner->co_sleep(think_for);
                event.edit_original_response(msg);
                co_return;
            }
            event.reply(msg);
        }

        virtual std::vector<command> commands(bot& bot) override final
        {
            command get_avatar("get avatar", dpp::ctxm_user, bot.me.id, run_get_avatar);
            command get_banner("get banner", dpp::ctxm_user, bot.me.id, run_get_banner);

            command extract_emojis_message("extract emojis", dpp::ctxm_message, bot.me.id, run_extract_emojis);
            //command extract_emojis_user("extract emojis", dpp::ctxm_user, bot.me.id, run_extract_emojis);
            // uncomment when we're able to get emojis from status and aboutme

            auto say_desc = "Makes " + bot.me.username + " say its unique thoughts (totally not controlled by " + bot.app_owner().username + ")";
            command say("say", say_desc, bot.me.id, run_say);
            say.add_option(dpp::command_option(dpp::co_boolean, "files", "Has files"));

            return { get_avatar, get_banner, extract_emojis_message, say };
        }

        virtual bool init(bot& bot) override final
        {
            form_submit_handle = bot.on_form_submit.attach(form_submit_event);
            return true;
        }

        virtual void destroy(bot& bot) override final
        {
            bot.on_form_submit.detach(form_submit_handle);
        }
    public:
        utility_module() : module("utility") {}
    };
    static utility_module instance;
}