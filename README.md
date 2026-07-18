# fixedphilip
<img src="https://raw.githubusercontent.com/brokenphilip/fixedphilip/refs/heads/main/assets/logo_128.png" align="right">

fixedphilip is a general-purpose Discord bot written in C++20, utilizing the [D++](https://dpp.dev/) library.

fixedphilip is designed to support both modern slash-commands (guild and user installs), as well as old-style (chat prefix) commands (provided the "Message Content" privileged intent is enabled).

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
> Currently, the bot is designed for **personal use only**:
> - I will not be providing a guild/user invite link for my own hosted instance to the general public, and
> - Feature requests will likely not be considered, unless I personally have a use for them.

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
Upon your first launch, it will create a default `config.json`. You must edit this file in a text editor and provide your own [bot token](https://dpp.dev/creating-a-bot-application.html). After the required modifications have been made, you can run fixedphilip as normal.

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
### Creating commands
Adding new features to fixedphilip is done by creating commands. Each command's source file is located under `src/commands/*.cpp`, and is usually named after the command. This ensures each command is its own separate compile unit that can be freely disabled at any time.
```cpp
#include <fixedphilip/command.h>

namespace fixedphilip::commands::example
{
    // This callback is run when the command is being created/initialized, right before it gets registered
    // Return true if initialization was successful and register the command, false otherwise
    dpp::task<bool> init(dpp::slashcommand& command, fixedphilip::discord::bot& bot)
    {
        // You may modify the slash command reference here (to add options to it, or set its flags)
        co_return true;
    }

    // This callback is run when the command is being executed, either via slash-command or by old-style (chat prefix) command
    // Use the "bot" reference to access the dpp::cluster responsible for the command, and other bot-specific info
    dpp::task<void> run(const fixedphilip::command::run_event& event, fixedphilip::discord::bot& bot)
    {
        // "event" is a variant which may contain a dpp::message_create_t or a dpp::slashcommand_t
        // ...but it also contains helper variant-agnostic functions such as "reply(...)"
        event.reply("Hello from example command! :3");
        co_return;
    }
}

// This macro allocates your command (fixedphilip::commands::example) in static memory, and adds it to its own internal linked list
// This linked list is sorted alphabetically (based on name) upon each addition - commands are always initialized in this order
FIXEDPHILIP_COMMAND(example, "This is an example command's description");
```