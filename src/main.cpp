#include <fixedphilip/log.h>
#include <fixedphilip/build.h>
#include <fixedphilip/discord.h>
#include <fixedphilip/utils/time.h>

int main()
{
#ifdef __linux__
    setlinebuf(stdout);
#endif
    fixedphilip::log::info("==============================");
    fixedphilip::log::info(std::format("fixedphilip {} by brokenphilip", FIXEDPHILIP_BUILD_VERSION_NUM));
    fixedphilip::log::info(std::format("Built on {}", fixedphilip::build::date_time()));
    fixedphilip::log::info(std::format("Targets " FIXEDPHILIP_BUILD_PLATFORM ", " FIXEDPHILIP_BUILD_CONFIGURATION ", {}-bit", FIXEDPHILIP_BUILD_ARCHITECTURE_NUM));
    fixedphilip::log::info("==============================");

    {
        fixedphilip::discord::bot::config bot_config;
        if (!bot_config.load_from_file("config.json"))
        {
            fixedphilip::log::error("Bot configuration failed - shutting down...");
            return 1;
        }
        fixedphilip::discord::bot bot(bot_config);
        if (!bot.setup())
        {
            fixedphilip::log::error("Bot setup failed - shutting down...");
            return 1;
        }
        bot.run_blocking();
    }

    fixedphilip::log::info("Bot terminated - shutting down...");
    return 0;
}
