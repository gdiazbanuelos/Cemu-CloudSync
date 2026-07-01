# **Cemu-CloudSync**

[![Build check](https://github.com/gdiazbanuelos/Cemu-CloudSync/actions/workflows/build_check.yml/badge.svg)](https://github.com/gdiazbanuelos/Cemu-CloudSync/actions/workflows/build_check.yml)
[![Release current build](https://github.com/gdiazbanuelos/Cemu-CloudSync/actions/workflows/release_now.yml/badge.svg)](https://github.com/gdiazbanuelos/Cemu-CloudSync/actions/workflows/release_now.yml)
[![Latest release](https://img.shields.io/github/v/release/gdiazbanuelos/Cemu-CloudSync)](https://github.com/gdiazbanuelos/Cemu-CloudSync/releases)

This is a personal fork of [Cemu](https://github.com/cemu-project/Cemu), a Wii U emulator that is able to run most Wii U games and homebrew in a playable state.

This fork adds **CloudSync**: automatic save file syncing to Dropbox via [rclone](https://rclone.org/). When a game starts, saves are pulled from Dropbox if a newer copy exists remotely; after the save system initializes, local saves are pushed back up. Works on Windows and Linux (including AppImage builds).

Everything else below is unmodified from upstream Cemu.

Cemu is currently only available for 64-bit Windows, Linux & macOS devices.

### Links:
 - [Open Source Announcement](https://www.reddit.com/r/cemu/comments/wwa22c/cemu_20_announcement_linux_builds_opensource_and/)
 - [Official Website](https://cemu.info)
 - [Compatibility List/Wiki](https://wiki.cemu.info/wiki/Main_Page)
 - [Official Subreddit](https://reddit.com/r/Cemu)
 - [Official Discord](https://discord.gg/5psYsup)
 - [Official Matrix Server](https://matrix.to/#/#cemu:cemu.info)
 - [Setup Guide](https://cemu.cfw.guide)

#### Other relevant repositories:
 - [Cemu-Language](https://github.com/cemu-project/Cemu-Language)
 - [Cemu's Community Graphic Packs](https://github.com/cemu-project/cemu_graphic_packs)

## Download

You can download the latest build of this fork (with CloudSync) from this repo's [GitHub Releases](https://github.com/gdiazbanuelos/Cemu-CloudSync/releases).

For the official, unmodified Cemu, see the upstream [GitHub Releases](https://github.com/cemu-project/Cemu/releases/) or [flathub](https://flathub.org/apps/info.cemu.Cemu) for Linux.

On Windows, Cemu is available both as an installer and in a portable format, where no installation is required besides extracting it in a safe place.

The native macOS build is currently purely experimental and should not be considered stable or ready for issue-free gameplay. There are also known issues with degraded performance due to the use of MoltenVK and Rosetta for ARM Macs. We appreciate your patience while we improve Cemu for macOS.

Pre-2.0 releases can be found on Cemu's [changelog page](https://cemu.info/changelog.html).

## CloudSync Setup

CloudSync needs an [rclone](https://rclone.org/) remote named exactly **`Dropbox`** (case-sensitive) configured on your system before saves will sync.

### Windows

1. Download the Windows zip from [rclone.org/downloads](https://rclone.org/downloads/)
2. Extract it somewhere permanent, e.g. `C:\rclone\`
3. Add that folder to your `PATH`: Windows Search → "Environment Variables" → **Edit the system environment variables** → **Environment Variables** → under "System variables" select `Path` → **Edit** → **New** → add `C:\rclone` → OK everywhere
4. Open a **new** PowerShell/Command Prompt window (to pick up the updated `PATH`) and run:
   ```
   rclone config
   ```
5. Follow the wizard:
   - `n` → New remote
   - Name: `Dropbox`
   - Storage type: pick the number for `dropbox`
   - Leave `client_id`/`client_secret` blank (press Enter) unless you've set up your own Dropbox app
   - Edit advanced config: `n`
   - Use auto config: `y` — this opens your browser to log into Dropbox and authorize rclone
   - Confirm and `q` to quit config
6. Test it:
   ```
   rclone lsd Dropbox:
   ```
   Should list your Dropbox's top-level folders with no error.

### Linux / Steam Deck

Steam Deck's rootfs is read-only by default, so don't use `sudo pacman -S` — install rclone to your user's home directory instead.

**Steam Deck (Desktop Mode → open Konsole terminal):**
```bash
mkdir -p ~/.local/bin
curl -O https://downloads.rclone.org/rclone-current-linux-amd64.zip
unzip rclone-current-linux-amd64.zip
cp rclone-*-linux-amd64/rclone ~/.local/bin/
chmod +x ~/.local/bin/rclone
rm -r rclone-*-linux-amd64 rclone-current-linux-amd64.zip
```

**Regular Linux distro (with sudo):**
```bash
sudo apt install rclone      # Debian/Ubuntu
# or
curl https://rclone.org/install.sh | sudo bash
```

Then on either, configure the Dropbox remote (same steps as Windows):
```bash
~/.local/bin/rclone config   # or just `rclone config` if it's on your PATH
```
- `n` → New remote
- Name: `Dropbox`
- Storage type: `dropbox`
- Leave client_id/secret blank
- Advanced config: `n`
- Auto config: `y` (Desktop Mode has a browser, so this should work directly)

Test:
```bash
~/.local/bin/rclone lsd Dropbox:
```

## Build Instructions

To compile Cemu yourself on Windows, Linux or macOS, view [BUILD.md](/BUILD.md).

## Issues

Issues with the emulator should be filed using [GitHub Issues](https://github.com/cemu-project/Cemu/issues).  
The old bug tracker can be found at [bugs.cemu.info](https://bugs.cemu.info) and still contains relevant issues and feature suggestions.

## Contributing

Pull requests are very welcome. For easier coordination you can visit the developer discussion channel on [Discord](https://discord.gg/5psYsup) or alternatively the [Matrix Server](https://matrix.to/#/#cemu:cemu.info).
Before submitting a pull request, please read and follow our code style guidelines listed in [CODING_STYLE.md](/CODING_STYLE.md).

If coding isn't your thing, testing games and making detailed bug reports or updating the (usually outdated) compatibility wiki is also appreciated!

Questions about Cemu's software architecture can also be answered on Discord (or through the Matrix bridge).

#### AI generated contributions:

We ask that all code submitted is written and understood by a human. You can use AI for planning, designing, reviewing and for asking questions about the codebase, but the code itself needs to be written by you. As a small exception you can use intellisense-style AI code autocompletion for pure boilerplate code as long as it's only a small part of your submission. To further clarify, when we ask for "human written" that excludes letting an AI write the code and then paraphrasing it. In other words, we are asking for human effort.

Why this policy exists:

We have relatively low reviewing capacity and requiring human-written code increases the quality and trustworthyness of submitted pull requests. There are also general concerns with AI usage in emulation:
- LLMs tend to make up solutions that work on the surface but are generally not accurate in the emulation sense
- There is evidence that LLMs have been trained on leaked proprietary SDKs and we cannot verify the origin of the knowledge. This is especially a problem for core emulation logic

Please keep these points in mind when contributing to Cemu. Contributions that do not follow this policy may be rejected.

## License
Cemu is licensed under [Mozilla Public License 2.0](/LICENSE.txt). Exempt from this are all files in the dependencies directory for which the licenses of the original code apply as well as some individual files in the src folder, as specified in those file headers respectively.
