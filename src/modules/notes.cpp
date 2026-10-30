#include <discofloor/bot.h>
#include <discofloor/timed_interaction.h>

namespace discofloor
{
    class notes_module : public bot_module
    {
        static inline timed_book* active_book = nullptr;

        static dpp::task<void> run_notes(const run_event& event)
        {
            if (active_book)
            {
                delete active_book;
            }

            std::vector<std::string> example
            {
                "1\\. <t:1793392200:R>\n"
                "> # asdasdasd\n"
                "\n"
                "2\\. <t:1791242753:R>\n"
                "> :pleading_face::sparkles:\n"
                "\n"
                "3\\. <t:1790801771:R>\n"
                "> merenja slikati + prog dom + mat dom + eng dom + mc/c:s @​ 19h ```also code blok cuz fuk it y not```",
                "4\\. <t:1790814381:R>\n"
                "> .\n"
                "\n"
                "5\\. <t:1822336200:R>\n"
                "> -# will this work\n"
                "\n"
                "6\\. <t:1801427400:R>\n"
                "> https://dpp.dev/resolved-objects.html",
                "7\\. <t:1949689800:R>\n"
                "> disc msgs + winamp songs + shazam songs + all songs to phone??? + sort desktop + sort pc files + sort mobile files + bulbtoys prettier menu + bulbtoys dinput hook + find a fucking job\n"
                "\n"
                "8\\. <t:1790803331:R>\n"
                "> https://discord.com/channels/1524539150471004160/1524539151129776200/1553447831694610534\n"
            };

            active_book = new timed_book("### :alarm_clock: | Reminders", timed_book::make_component_vectors_from_string_contents_vector(example),
                timed_book::tm_disable_buttons, event.command_invoker().id, 1, *event.get_interaction_create(), 60);
            co_return;
        }

        static std::vector<dpp::component> note2(const std::string& content, bool add_separator)
        {
            dpp::component text_display;
            text_display.set_type(dpp::cot_text_display);
            text_display.set_content(content);

            // buttons need to be added to an action row first
            // they can't be added directly to the container
            // afterwards we add the whole action row to the container
            dpp::component actions;
            actions.set_type(dpp::cot_action_row);

            dpp::component button_remove;
            button_remove.set_type(dpp::cot_button);
            button_remove.set_emoji("🗑️");
            button_remove.set_style(dpp::cos_danger);
            button_remove.set_id(std::to_string(rand()));

            dpp::component button_edit;
            button_edit.set_type(dpp::cot_button);
            button_edit.set_emoji("✏️");
            button_edit.set_style(dpp::cos_secondary);
            button_edit.set_id(std::to_string(rand()));

            //dpp::component button_jump;
            //button_jump.set_type(dpp::cot_button);
            //button_jump.set_label(std::format("{} / {}", index + 1, page_count));
            //button_jump.set_style(dpp::cos_primary);
            //button_jump.set_id(generate_jump_page_custom_id());

            dpp::component button_reschedule;
            button_reschedule.set_type(dpp::cot_button);
            button_reschedule.set_emoji("⏲️");
            button_reschedule.set_style(dpp::cos_secondary);
            button_reschedule.set_id(std::to_string(rand()));

            actions.add_component_v2(button_remove);
            actions.add_component_v2(button_edit);
            //actions.add_component_v2(button_jump);
            actions.add_component_v2(button_reschedule);

            if (add_separator)
            {
                dpp::component main_separator;
                main_separator.set_type(dpp::cot_separator);
                main_separator.set_spacing(dpp::sep_large);
                main_separator.set_divider(false);
                return { text_display, actions, main_separator };
            }
            return { text_display, actions };
        }

        static dpp::task<void> run_notes2(const run_event& event)
        {
            if (active_book)
            {
                delete active_book;
            }

            auto note_1 = note2(
                "1\\. <t:1793392200:R>\n"
                "> # asdasdasd", true
            );

            auto note_2 = note2(
                "2\\. <t:1791242753:R>\n"
                "> :pleading_face::sparkles:", true
            );

            auto note_3 = note2(
                "3\\. <t:1790801771:R>\n"
                "> merenja slikati + prog dom + mat dom + eng dom + mc/c:s @​ 19h ```also code blok cuz fuk it y not```", false
            );

            auto note_4 = note2(
                "4\\. <t:1790814381:R>\n"
                "> .", true
            );

            auto note_5 = note2(
                "5\\. <t:1822336200:R>\n"
                "> -# will this work", true
            );

            auto note_6 = note2(
                "6\\. <t:1801427400:R>\n"
                "> https://dpp.dev/resolved-objects.html", false
            );

            auto note_7 = note2(
                "7\\. <t:1949689800:R>\n"
                "> disc msgs + winamp songs + shazam songs + all songs to phone??? + sort desktop + sort pc files + sort mobile files + bulbtoys prettier menu + bulbtoys dinput hook + find a fucking job", true
            );

            auto note_8 = note2(
                "8\\. <t:1790803331:R>\n"
                "> https://discord.com/channels/1524539150471004160/1524539151129776200/1553447831694610534", false
            );

            std::vector<std::vector<dpp::component>> component_vectors;

            auto& page_1 = component_vectors.emplace_back();
            page_1.insert(page_1.end(), note_1.begin(), note_1.end());
            page_1.insert(page_1.end(), note_2.begin(), note_2.end());
            page_1.insert(page_1.end(), note_3.begin(), note_3.end());

            auto& page_2 = component_vectors.emplace_back();
            page_2.insert(page_2.end(), note_4.begin(), note_4.end());
            page_2.insert(page_2.end(), note_5.begin(), note_5.end());
            page_2.insert(page_2.end(), note_6.begin(), note_6.end());

            auto& page_3 = component_vectors.emplace_back();
            page_3.insert(page_3.end(), note_7.begin(), note_7.end());
            page_3.insert(page_3.end(), note_8.begin(), note_8.end());

            active_book = new timed_book("### :alarm_clock: | Reminders", component_vectors, timed_book::tm_disable_buttons, event.command_invoker().id, 1, *event.get_interaction_create(), 60);
            co_return;
        }

        static dpp::task<void> run_modnote(const run_event& event)
        {
            if (!active_book)
            {
                event.reply("nuh uh");
            }

            active_book->modify([](std::string& title, std::vector<std::vector<dpp::component>>& contents)
            {
                title = "modified :3";
                contents.erase(contents.begin() + 1);
                return 1;
            });
            event.reply("OK");
            co_return;
        }

        virtual std::vector<bot_command> commands(bot& bot) override final
        {
            bot_command notes("notes", "Noooooootes :3", bot.me.id, run_notes);
            bot_command notes2("notes2", "Noooooootes2 :3", bot.me.id, run_notes2);
            bot_command modnote("modnote", "modifyn ote", bot.me.id, run_modnote);

            return { notes, notes2, modnote };
        }

        inline virtual void destroy(bot& bot) override final
        {
            if (active_book)
            {
                delete active_book;
            }
        }
    public:
        notes_module() : bot_module("notes") {}
    };
    static notes_module instance;
}