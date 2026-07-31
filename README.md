# fixedphilip
<img src="https://raw.githubusercontent.com/brokenphilip/fixedphilip/refs/heads/main/assets/logo_128.png" align="right">

> *"when is brokenphilip getting fixed?!" -several concerned internet residents*

fixedphilip is a general-purpose Discord bot written in C++20, utilizing the [D++](https://dpp.dev/) library. The bot aims to provide a convenient set of features for everyday use, primarily ones not found in (or executed better than) other Discord bots. It is designed to support both modern slash-commands (for guild/server and user installs), as well as optional old-style (chat prefix) commands (provided the "Message Content" privileged intent is enabled).

Some of fixedphilip's most notable features include:
- Detailed `/status` command with uptime, statistics, machine resource usage* and other info
- Conversion between some measurable units (temperature, speed, others*) and all currencies using `/convert`
- Information and notifications/subscriptions (on a municipality, settlement or street level) regarding planned power outages in Serbia using `/eds`*
- Advanced `/remind`er management*
- Modular command system, making it easier for developers to create and manage fixedphilip features

> [!NOTE]
> Asterisk (*) indicates a planned feature.
>
> The `/eds` command is only available through my own hosted instance, as it is closed-source.

> [!IMPORTANT]
> Currently, the bot is intended for **personal use only**:
> - I will not be providing a guild/user invite link for my own hosted instance to the general public, and
> - Feature requests will likely not be considered, unless I could benefit from them myself.

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

Aside from the `token`, the `config.json` also contains the following keys (remove a key to reset it to its defaults):
- `prefix` - the default/global chat prefix for old-style commands
  - If you wish to disable old-style commands, set the prefix to a blank string ("")
- `presence_activity` - the activity text shown in the bot's presence (member list and profile)
  - If you wish to disable the bot's presence altogether, set this to a blank string ("")
  - Accepts prefixes "Playing ...", "Streaming ...", "Listening to ...", "Watching ..." and "Competing in ..."
  - Also accepts tokens, which automatically get replaced when presence gets updated:
    - The `%prefix%` token is replaced with the default/global chat prefix for old-style commands
    - The `%version%` token is replaced with the currently running fixedphilip version
- `presence_status` - the status icon/color shown in the bot's presence (member list and profile)
  - Accepts "offline", "online", "dnd", "idle" and "invisible"
- `presence_update_rate_mins` - how often the bot's presence should update (0 means the status only gets set once on startup)

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
While there is no official documentation, most header file functions and data structures are either self-explanatory or documented via comments.
### Creating modules
Adding new features to fixedphilip is done by creating modules. Each module's source file is located under `src/modules/*.cpp`, and is usually named after the module. This ensures each command is its own separated compilation unit that can be freely disabled at any time.


### Creating commands
Adding new features to fixedphilip is done by creating commands. Each command's source file is located under `src/commands/*.cpp`, and is usually named after the command. This ensures each command is its own separate compile unit that can be freely disabled at any time.
```cpp
// This include file contains all the necessary data structures to create a fixedphilip module
#include <fixedphilip/discord.h>

// Can be anything really, so long as it's ideally not in the global namespace
namespace fixedphilip
{
    // Each module should inherit from the base class, as it is pretty useless on its own
	// You can name it whatever - this repository, for consistency, names it "<module_name>_module"
    class example_module : public fixedphilip::discord::bot::module
    {
        // This function is called when the "/test" command is being executed by a (non-bot) user
        // Depending on the type of your command, "run_event" is a variant that can be one of either:
        // - "dpp::slashcommand_t" or "dpp::message_create_t" if your command is "dpp::ctxm_chat_input" (CHAT_INPUT)
		//   - These are the default chat-based slash commands, which also work with old-style (chat prefix) commands
		//   - Slash command provide the former, old-style commands provide the latter (unless they're disabled)
        // - "dpp::message_context_menu_t" if your command is "dpp::ctxm_message" (MESSAGE)
		//   - These are context-menu ("right click") application "commands" you can perform on messages
        // - "dpp::user_context_menu_t" if your command is "dpp::ctxm_user" (USER)
		//   - These are context-menu ("right click") application "commands" you can perform on users
		// The "run_event" also provides variant-agnostic helper functions, such as reply(...)
        // For more information on what the "run_event" can do, check the included discord header file
        static dpp::task<void> run_test(const fixedphilip::discord::bot::command::run_event& event)
        {
		    event.reply("Hello world! :3");

			// As this is a coroutine, if you're not co_await-ing any functions, you must specifically write "co_return"
            co_return;
		}

        // This function is called when a fixedphilip::discord::bot is being created
        // Alternatively, this function is also called when it is being late-loaded using bot::add_module()
		// Note that modules can be disabled using "disabled_modules" from config.json
        // Modules are initialized in alphabetical order based on their name
        virtual bool init(fixedphilip::discord::bot& bot) override final
		{
            // Your module initialization goes here...
            // Return false if something goes wrong, to prevent your module from being loaded
			// By default, this virtual function just returns true - if you're doing the same, you don't need to override it
            return true;
        }

        // This function is called when a fixedphilip::discord::bot is requesting all loaded modules' commands
		// This usually happens a short while after init() - more specifically, in the bot's initial on_ready event
        // For late-loaded commands using bot::add_module(), this function is called right after init()
        virtual std::vector<fixedphilip::discord::bot::command> commands(fixedphilip::discord::bot& bot) override final
        {
            fixedphilip::discord::bot::command test("test", "Test example command", bot.me.id, run_test);
			// test.add_option(...).add_option(...);

			// If your module requires multiple commands, you'd add them to the initializer list return here
            return { test };
        }

        // This function is called when a fixedphilip::discord::bot is being destroyed
        // Alternatively, this function is also called when it is being early-unloaded using bot::remove_module()
        // Modules are destroyed in reverse-alphabetical order based on their name
		virtual void destroy(fixedphilip::discord::bot& bot) override final
		{
            // By default, this virtual function does nothing - if you don't need it, you don't need to override it
		}

        // Since this module is designed to be single-instance, we fill in the base constructor parameters here
        // If you make a dynamically allocated module, you'd usually pass this info through your constructor
		// While it's not illegal for multiple modules to share the same name, it is not recommended
    public:
	    // For consistency and linked list order, this repository practices lowercase module names
		// You don't have to do this, but be wary that mixing cases will affect the alphabetical order
		example_module() : fixedphilip::discord::bot::module("example", "This is the example module's description") {}
    };
    // Modules are, by design, meant to be single-instance, but they don't have to be!
	// If you plan on making dynamically allocated modules, you can use bot::add_module() and bot::remove_module()
    // When a module is being instantiated, it gets added to the internal alphabetically-sorted linked list of modules
    // This linked list is iterated for each fixedphilip bot during construction, where each module gets initialized
    // Dynamically allocated modules also get added to this internal linked list, but it is basically useless
    static example_module instance;
}
```
