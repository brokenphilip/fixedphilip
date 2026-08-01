# fixedphilip
<img src="https://raw.githubusercontent.com/brokenphilip/fixedphilip/refs/heads/main/assets/logo_128.png" align="right">

> *"when is brokenphilip getting fixed?!" -several concerned internet strangers*

fixedphilip is a general-purpose Discord bot written in C++20, utilizing the [D++](https://dpp.dev/) library.

The bot aims to provide a convenient set of unique features for everyday use, and it is designed to support both modern slash-commands (for guild/server and user installs), as well as optional old-style (chat prefix) commands (provided the "Message Content" privileged intent is enabled).

The project is configured to build for (and run on) Windows and ARM64 using the Visual Studio development environment, but the CMake presets file can be modified to support any platform that the D++ library itself supports.

Some of fixedphilip's most notable features include:
- Conversion between most measurable units and all currencies using `/convert`
- Information and notifications/subscriptions (on a municipality, settlement or street level) regarding planned power outages in Serbia using `/eds`*
- Advanced `/remind`er management*
- Full-fledged module system, making it easy for developers to create and manage fixedphilip feature sets

> [!NOTE]
> Asterisk (*) indicates a planned feature.
>
> The `/eds` command is only available through my own hosted instance, as it is closed-source.

> [!IMPORTANT]
> Currently, the bot is intended for **personal use only**:
> - I will not be providing a guild/user invite link for my own hosted instance to the general public, and
> - Feature requests may be rejected on a biased, personal use-case basis

## Setup
### ...on Windows
Ensure the filesystem structure matches the following:
```
dpp.dll
fixedphilip.exe
libcrypto-1_1-x64.dll
libssl-1_1-x64.dll
opus.dll
zlib1.dll
```

### ...on ARM64
Ensure the required runtime libraries are installed before running `./fixedphilip`:
```sh
#! /usr/bin/env sh

# install runtime libraries
sudo apt install libopus0 openssl zlib1g

# install dpp library in the current directory if you didn't build your own
wget -O dpp.deb https://dl.dpp.dev/latest/linux-rpi-arm64
sudo dpkg -i dpp.deb
```

## Running
Upon your first launch, it will create a default `config.json`. You must edit this file in a text editor and provide your own [bot token](https://dpp.dev/creating-a-bot-application.html), after which you can run fixedphilip as normal.

fixedphilip supports the following optional launch parameters:
- `--total-shards` - Equivalent to the `shards` parameter of the `dpp::cluster` constructor
  - *"The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster. If you omit this value, the library will attempt to query the Discord API for the correct number of shards to start."*
  - By default, this value is set to 0 (ie. "omitted")
- `--cluster-id` - Equivalent to the `cluster_id` parameter of the `dpp::cluster` constructor
  - *"The ID of this cluster, should be between 0 and MAXCLUSTERS-1"*
  - By default, this value is set to 0
- `--max-clusters` - Equivalent to the `maxclusters` parameter of the `dpp::cluster` constructor
  - *"The total number of clusters that are active, which may be on separate processes or even separate machines."*
  - By default, this value is set to 1
- `--intents` - Equivalent to the `intents` parameter of the `dpp::cluster` constructor
  - *"A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers."*
  - By default, this value is set to `dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_members`
- `--config-file` - the absolute or relative path to the config file
  - By default, this value is set to `config.json`

Aside from the `token`, the `config.json` also contains the following keys:
- `prefix` - the default/global chat prefix for old-style commands
  - If you wish to disable old-style commands, set the prefix to a blank string ("")
- `disabled_modules` - an array of strings containing the names of the modules you want to disable
  - If you don't want to disable any modules, leave the array empty
  
If the `presence` module is enabled, the following keys can be found in `presence.json`:
- `presence_activity` - the activity text shown in the bot's presence (member list and profile)
  - If you wish to disable the bot's presence altogether, set this to a blank string ("")
  - Accepts prefixes "Playing ...", "Streaming ...", "Listening to ...", "Watching ..." and "Competing in ..."
  - Also accepts tokens, which automatically get replaced when presence gets updated:
    - The `%prefix%` token is replaced with the default/global chat prefix for old-style commands
    - The `%version%` token is replaced with the currently running fixedphilip version
- `presence_status` - the status icon/color shown in the bot's presence (member list and profile)
  - Accepts "offline", "online", "dnd", "idle" and "invisible"
- `presence_update_rate_mins` - how often the bot's presence should update (0 means the status only gets set once on startup)

Removing any key resets its value to default.

## Building
To get started, clone the repository while recursing submodules, but ignore `src/commands/private`, as this submodule/folder is reserved for my own private closed-source commands.

### ...to Windows using Visual Studio
To build fixedphilip for Windows, OpenSSL and a reasonably modern version of VS must be installed.

Using the Visual Studio installer, modify your VS installation and select the following workload:
- Desktop development with C++

Start VS, and in the menu bar, navigate to **File -> Open -> Project/Solution**, and select `CMakeLists.txt` to open fixedphilip as a VS project.

Ensure "Local Machine" and your desired build configuration (x64 or x86, Debug or Release) are selected in the Toolbar, then in the VS menu bar, navigate to **Build -> Build All**.

After building, open `fixedphilip\out\build\<configuration>` and copy `fixedphilip.exe` to a location (folder) of your choosing. Additionally, copy all `*.dll` files from `fixedphilip\out\build\<configuration>\DPP\library` to the same location. Finally, the filesystem structure of the location (folder) of your choosing should look as follows:
```
dpp.dll
fixedphilip.exe
libcrypto-1_1-x64.dll
libssl-1_1-x64.dll
opus.dll
zlib1.dll
```

Proceed to setup and run fixedphilip as explained above.

### ...to ARM64 using Visual Studio
To build fixedphilip for ARM64/aarch64, Windows Subsystem for Linux and a reasonably modern version of VS must be installed.

Using the VS installer, modify your VS installation and select the following workloads:
- Desktop development with C++
- Linux, Mac, and embedded development with C++

Set up your WSL installation for building to ARM64. The following commands apply to Debian, but with little to no tweaks it should work on other distros as well:
```sh
#! /usr/bin/env sh

# ensure we can recognize ARM64
sudo dpkg --add-architecture arm64

# refresh update lists
sudo apt update

# upgrade everything
sudo apt upgrade

# install ssh (for rsync)
# install wget (for dpp)
# install build tools (for building)
# install ARM64 specific development libraries (for building to ARM64)
sudo apt install openssh-client wget rsync build-essential gcc-aarch64-linux-gnu \
                 g++-aarch64-linux-gnu cmake ninja-build libopus-dev:arm64 \
                 libssl-dev:arm64 zlib1g-dev:arm64

# install ARM64 dpp library in the current directory if you're not building your own
wget -O dpp.deb https://dl.dpp.dev/latest/linux-rpi-arm64
sudo dpkg -i dpp.deb
```
Start VS, and in the menu bar, navigate to **File -> Open -> Project/Solution**, and select `CMakeLists.txt` to open fixedphilip as a VS project.

Ensure "WSL: Debian" (or your distro of choice) and your desired build configuration (Debug or Release) are selected in the Toolbar, then in the VS menu bar, navigate to **Build -> Build All**.

After building, `rsync` fixedphilip to your target system, to a location (folder) of your choosing.

Proceed to setup and run fixedphilip as explained above.

## Development
While there is no official documentation (aside from the D++ library documentation provided on [their website](https://dpp.dev/)), most header file functions and data structures are either self-explanatory or thoroughly documented via comments.
### Creating modules
Adding new features to fixedphilip is done by creating modules. Each module's source file is located under `src/modules/*.cpp`, and is usually named after the module. This ensures each command is its own separated compilation unit that can be freely disabled at any time.
```cpp
// This include file contains all the necessary data structures to create a fixedphilip module
// Feel free to include whatever else you may need from this project, or any other library
#include <fixedphilip/discord.h>

// The namespace can be anything really, so long as we're not polluting the global one
// To avoid writing word salad types, and for consistency sake, it's best to use this namespace
namespace fixedphilip::discord
{
    // Each module should inherit from the base class, as the base class is pretty useless on its own
    // You can name it whatever - this repository, for consistency, names it "<module_name>_module"
    class example_module : public bot::module
    {
        // This function is called when the "/test" command is being executed by a (non-bot) user
        // Depending on the type of your command, "run_event" is a variant that can be one of either:
        // 
        // - "dpp::slashcommand_t" or "dpp::message_create_t" if your command is "dpp::ctxm_chat_input"
        //   - These are the default chat-based (ie. "CHAT_INPUT") slash commands
        //   - When created, the bot also listens for old-style (chat prefix) commands (unless disabled)
        //   - Slash command provide the former, old-style commands provide the latter variant
        // 
        // - "dpp::message_context_menu_t" if your command is "dpp::ctxm_message" (ie. "MESSAGE")
        //   - These are context-menu ("right click") application "commands" you can perform on messages
        // 
        // - "dpp::user_context_menu_t" if your command is "dpp::ctxm_user" (ie. "USER")
        //   - These are context-menu ("right click") application "commands" you can perform on users
        // 
        // The "run_event" also provides variant-agnostic helper functions, such as reply(...)
        // For more information on what the "run_event" can do, check the included discord header file
        static dpp::task<void> run_test(const bot::command::run_event& event)
        {
            // Send a reply to the user who issued the command
            event.reply("Hello world! :3");

            // As this is a coroutine, you must specifically write "co_return" instead of "return"
            // (though, you can omit the "co_return" if you already wrote it (or "co_await") somewhere
            co_return;
        }

        // Similarly to the above function, it gets called when the "/ping" command is being executed
        static dpp::task<void> run_ping(const bot::command::run_event& event)
        {
            // Here's an example on how to get the fixedphilip bot (cluster) from the "run_event"
            auto cluster = event.get_bot();
            if (!cluster)
            {
                // Realistically this should never happen, but just to be on the safe side...
                // ...oh and to make the IDE happy and not complain about a potential null pointer :)
                fixedphilip::log::error("run_ping: bot was null");
                co_return;
            }

            // If your bot/cluster pointer is valid, instead of calling fixedphilip::log::*()...
            // ...you should call the bot/cluster's log() instead, for more specific log prints
            cluster->log(dpp::ll_info, "Running the ping command...");

            // Here's an example on how to get the current module from the bot executing your command
            // If your module is shared between bots, you must protect your data with mutexes
            // (if it is static ie. single-instance, or dynamic and added to multiple bots)
            for (const auto& module : cluster->loaded_modules())
            {
                if (std::string("example") == module->name())
                {
                    // Access your module's non-static public (or private!) members here
                }
            }

            // Here's an example on how to get the user who issued the command
            dpp::user author;
            if (auto slash_command = event.get_slash_command())
            {
                author = slash_command->command.usr;
            }
            if (auto message_create = event.get_message_create())
            {
                author = message_create->msg.author;
            }
            // Since we know this is a "CHAT_INPUT" command, we only need to check for these 2 variants

            // Send a reply to the user who issued the command, printing their name and the bot's ping
            event.reply(std::format("Pong! Hey {}, my ping is: {} ms", 
                author.username, static_cast<int>(cluster->rest_ping * 1000)));
        }

        // This function is called when a fixedphilip bot is being created
        // Alternatively, it is also called when the module is being late-loaded using bot::add_module()
        // Modules are initialized in alphabetical order based on their name
        // 
        // Note that modules can additionally be disabled using "disabled_modules" from config.json
        // Since this is called during module iteration, avoid creating new (dynamic) modules here
        virtual bool init(bot& bot) override final
        {
            // Your module initialization code goes here...
            // Return false to prevent your module from being loaded (eg. if something goes wrong)
            // By default, this virtual function just returns true and doesn't do anything else
            // If you plan on doing the exact same thing, you don't need to override it
            return true;
        }

        // This function is called when a fixedphilip bot is requesting all loaded modules' commands
        // This usually happens a short while after init() - during the bot's initial on_ready event
        // For late-loaded commands using bot::add_module(), this function is called right after init()
        virtual std::vector<bot::command> commands(bot& bot) override final
        {
            bot::command test("test", "Test example command", bot.me.id, run_test);
            // test.add_option(...).add_option(...);

            bot::command ping("ping", "Get the bot's REST ping", bot.me.id, run_ping);
            // ping.add_option(...).add_option(...);

            // Return all the commands you've created to the initializer list here
            return { test, ping };
        }

        // This function is called when a fixedphilip bot is being destroyed
        // Alternatively, it is also called when it is being early-unloaded using bot::remove_module()
        // Modules are destroyed in reverse-alphabetical order based on their name
        virtual void destroy(bot& bot) override final
        {
            // By default, this virtual function doesn't do anything
            // If you don't need to run any code upon destruction, you don't need to override it
        }

        // Since this module is designed to be single-instance, we fill in the base parameters here
        // If you make a dynamically allocated module, you'd pass this info through your constructor
        // While it's not illegal for multiple modules to share the same name, it is not recommended
    public:
        // For consistency (and linked list order), this repository practices lowercase module names
        // You don't have to do this, but be wary that mixing cases will affect the alphabetical order
        example_module() : bot::module("example", "This is the example module's description") {}
    };

    // Modules are, by design, usually meant to be single-instance, but they don't have to be!
    // For dynamically allocated modules, you can use bot::add_module() and bot::remove_module()
    // 
    // When a module is being instantiated, it gets added to the internal linked list of modules
    // For each new fixedphilip bot, this list is iterated and each (static) module gets initialized
    // 
    // Dynamically allocated modules also get added to this linked list upon their instantiation...
    // ...but this will (and should!) practically always happen after it's already been iterated
    static example_module instance;
}
```
In the following image, you can see the example module in action:

<img width="361" height="470" alt="image" src="https://github.com/user-attachments/assets/3df93029-bc1a-4007-8bcc-52bc1fa43849" />
