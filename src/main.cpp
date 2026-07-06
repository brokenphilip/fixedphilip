#include <format>

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <fixedphilip/command.h>
#include <fixedphilip/build.h>
#include <fixedphilip/log.h>
#include <fixedphilip/utils/stopwatch.h>
#include <fixedphilip/utils/file.h>
#include <fixedphilip/discord.h>

/* When you invite the bot, be sure to invite it with the
 * scopes 'bot' and 'applications.commands', e.g.
 * https://discord.com/oauth2/authorize?client_id=940762342495518720&scope=bot+applications.commands&permissions=139586816064
 */

int main()
{
    fixedphilip::utils::program_uptime.start();
    fixedphilip::utils::program_start = std::chrono::system_clock::now();

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

    fixedphilip::log::info("Cluster shards terminated - shutting down...");
    fixedphilip::utils::program_uptime.stop();
    return 0;
}
