# fixedphilip
<img src="https://cdn.discordapp.com/app-icons/449970784585121792/e1f2f0407a77ddd696202c7ec3720e1b.png?size=128" align="right">

fixedphilip is a general-purpose Discord bot written in C++20, utilizing the [D++](https://dpp.dev/) library.

> [!NOTE]
> This repository is currently an **early work-in-progress**, any and all information is subject to change without notice and may be missing or incorrect.

> [!IMPORTANT]
> Currently, the bot is designed for **personal use only**:
> - I will not be providing a guild/user invite link for my own hosted instance, and
> - Feature requests will likely not be considered for the forseeable future.
<br clear="right"/>

## Running
Upon your first launch, it will create a `config.json` with the following contents, and then exit:
```json
{ "token": "your_bot_token_here" }
```
You must edit this file in a text editor by providing your own [bot token](https://dpp.dev/creating-a-bot-application.html). After the required modifications have been made, you can run fixedphilip as normal.

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

# install dpp library
mkdir ~/fixedphilip
cd ~/fixedphilip
wget -O dpp.deb https://dl.dpp.dev/latest/linux-rpi-arm64
sudo dpkg -i dpp.deb
```

## Building
### ...to Windows using Visual Studio
To build fixedphilip for Windows, a reasonably modern version of VS must be installed.

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

Proceed to run fixedphilip as explained above.

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

# install ARM64 dpp library
mkdir ~/fixedphilip
cd ~/fixedphilip
wget -O dpp.deb https://dl.dpp.dev/latest/linux-rpi-arm64
sudo dpkg -i dpp.deb
```
Start VS, and in the menu bar, navigate to **File -> Open -> Project/Solution**, and select `CMakeLists.txt` to open fixedphilip as a VS project.

Ensure "WSL: Debian" (or your distro of choice) and your desired build configuration (Debug or Release) are selected in the Toolbar, then in the VS menu bar, navigate to **Build -> Build All**.

After building, `rsync` fixedphilip to your target system, to a location (folder) of your choosing.

Proceed to run fixedphilip as explained above.
