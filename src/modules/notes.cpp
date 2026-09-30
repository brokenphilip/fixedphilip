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

            active_book = new timed_book("### :alarm_clock: | Reminders", example, timed_book::tm_disable_buttons, event.command_invoker().id, 0, *event.get_interaction_create(), 60);
            co_return;
        }

        virtual std::vector<bot_command> commands(bot& bot) override final
        {
            bot_command notes("notes", "Noooooootes :3", bot.me.id, run_notes);

            return { notes };
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