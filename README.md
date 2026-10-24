<p align="center"><img src="docs/icon-160.png" width="120" alt=""></p>

<h1 align="center">Warthog</h1>

<p align="center"><b>Halo: Combat Evolved back on the original Xbox: ChupathingyCE's console line, playing online with the PC builds.</b></p>

<p align="center">
<a href="https://github.com/ChupathingyCE/chupathingyce">ChupathingyCE</a> ·
<a href="https://halo.milenko.org">Games online now</a> ·
<a href="https://discord.gg/chupathingy">Discord</a>
</p>

> Warthog follows ChupathingyCE: the shared game code comes from there, and this repository adds only the consoles' own code. You need your own copy of Halo; no SDK or game files are included.

> **Compatible with OpenCE build-244 (network version 32) for online play.** Players on that build join our games, and ours join games hosted on network versions 11 through 37. Browser builds can't join our games, nor we theirs.

Chupathingy is a free, open-source port of the Halo: Combat Evolved
decompilation. It runs natively on Windows, Mac (Apple silicon and Intel),
Linux (including Steam Deck) and Android, with no emulator, and it is built for stability and
community play: the original Xbox campaign and multiplayer maps, Halo PC and
Custom Edition maps, online games with the community's servers, and releases
you can count on. The console versions (original Xbox, Xbox 360, Wii U and
Switch) live in repositories of their own, with this one as their base: see
[Console ports](docs/building.md#console-ports) for where each one stands.

## Status

**On the original Xbox (in development)**
- Boots to the menus at a steady 30 fps on a development kit, with
  profiles
- Gets its address by DHCP, and the game list from the internet
- **ONLINE GAMES** in the Multiplayer menu: the game list on a screen of its
  own, joining a game by its invite through the PCs' own tunnel (joined a
  [D] dedicated server from a development kit on October 13)
- **System Link** with PC builds: joins a PC host's game, a [D] dedicated
  server's too, at the console's 16 slots (tested with a desktop build at the
  console's limits; not yet on a console)
- Reads the console's memory (64, 128 or 256 MB) at the start, and can use
  more on a bigger one
- Music and sound: the menu music, the campaign's music and effects
- Not yet: hosting PC players, Bink movies, reverb, network co-op

**Consoles**

| Console | Status |
| --- | --- |
| Original Xbox | In development: this page |
| Xbox 360 | Next, with the Wii U alongside: both are big-endian, so the byte-order groundwork is shared. Scaffolding only so far |
| Wii U | Alongside the Xbox 360. Scaffolding only so far |
| Switch | thelinkin3000's port, being brought in |

- A port of the Halo: Combat Evolved decompilation for Windows, Mac, Linux
  and Android, run by its community.
- You bring your own legally owned copy of the game. We never host, share or
  distribute Microsoft's files, and the game never downloads them.
- No browser version, and no websites or domains that look official.
- A fan project, not made or endorsed by Microsoft, Bungie or 343 Industries.

## Download

Get the latest release from the [Releases page](https://github.com/ChupathingyCE/chupathingyce/releases/latest):

| Platform | Download | Notes |
| --- | --- | --- |
| Windows (64-bit) | `chupathingyce-windows64-release.zip` | Windows 10 or later, 64-bit. |
| Windows (32-bit, older PCs) | `chupathingyce-windows-release.zip` | Windows 10 or later. |
| Linux (64-bit) | `chupathingyce-linux64-release.zip` | glibc 2.31 or later (Debian 11, Ubuntu 20.04). Brings its own SDL3. See [port/linux/README.md](port/linux/README.md). |
| Linux (32-bit, older systems) | `chupathingyce-linux-release.zip` | glibc 2.31 or later (32-bit). Brings its own SDL3. See [port/linux/README.md](port/linux/README.md). |
| Android | `chupathingyce-android-release.zip` | Android 9 or later, 64-bit. See [port/android/README.md](port/android/README.md). |
| Mac | `chupathingyce-macos-release.zip` | macOS 13 or later, Apple silicon or Intel. |

The game checks for new releases when it starts and asks before updating. On Linux there is also an AppImage, on the test builds for now.
The Windows and Linux builds are for x86 and x64 PCs; there are no ARM builds of
the game for them yet.

We don't pay for code signing yet, so the first start needs one extra step:

- **Windows** may warn about an unknown publisher: choose **More info → Run anyway**.
- **Mac**: move ChupathingyCE to Applications and open it. If macOS won't open
  it, go to **System Settings → Privacy & Security**, and choose **Open
  Anyway** next to ChupathingyCE. You only do this once.

## You need your own copy of Halo

Warthog doesn't include the game's maps, sounds or art. You need your own
Xbox disc of Halo: Combat Evolved. Warthog plays the maps of the North
American release (NTSC, 01.10.12.2276), as every ChupathingyCE build and
dedicated server does; the PAL disc's maps work too.

1. Copy the `maps` folder from your own disc (about 2 GB) to your computer.
2. Lay the game out for the console's hard disk: `tools/xbox_package.py`
   puts `default.xbe` beside your maps in one folder (see "Build").
3. Copy that folder to the console, for example to `E:\Games\Warthog\`, and
   start `default.xbe`.

The same screen accepts your Halo PC retail CD image (`.iso`), which fills
`maps_pc`, and the Halo Custom Edition installer (`halocesetup_en_1.00.exe`),
which fills `maps_ce` (not on Android yet, where **Import maps** takes
loose `.map` files or a `.zip` of them). Or choose **Make the folders** to get empty `maps`,
`maps_ce`, `maps_pc` and `maps_md` folders and copy the files in yourself.

On Android, copy the disc image to your phone first; the maps, settings and
saves go in `/sdcard/Android/data/dev.horrible.chupathingyce/files`. On a
Mac, the maps, settings and saves go in
`~/Library/Application Support/ChupathingyCE`.
On Linux, the maps and settings (`config.toml`) go next to the `halo`
executable, and the saves in `~/.local/share/chupathingyce` (or
`$XDG_DATA_HOME/chupathingyce`). On Windows, the saves go in
`%APPDATA%\ChupathingyCE`. The first start copies your profiles, saved games
and game types from the folder earlier versions used (`halo-linux`, on Linux
and on a Mac, or `%APPDATA%\halo` on Windows), which is left as it was. The 64-bit and
32-bit builds use the same places, so switching from one to the other keeps
your saves and settings.

**Common mistake:** the `maps` folder must hold the Xbox maps from your disc
image, not Halo PC's. Halo PC maps go in folders of their own beside it:
`maps_ce`, `maps_md` and `maps_pc` (see below). On a Mac, Linux and
Windows, if `maps` holds Halo PC maps or has no `ui.map`, the game says so
and quits.

## Playing online

| You want to | Do this |
| --- | --- |
| Join a game | **Multiplayer → Online Games**, pick a game, press **A**. Or press **Join** on [stats.chupathingy.org](https://stats.chupathingy.org). |
| Host a game | **Multiplayer → Online Games → Y (Create Game)**, or host from System Link as usual. Your game is listed online by itself, on stats.chupathingy.org and in OpenCE's in-game Server Browser (`public_lobby`/`host_public` under `[network]` in `config.toml` turn that off). |
| Invite a friend | When you host, the game copies an invite link (`halo://join/…`). Send it; opening it joins your game. |
| See your stats | Your service record is on [stats.chupathingy.org](https://stats.chupathingy.org), found by your name. Games you join count even when the host doesn't run Chupathingy: when an online game you joined ends, the game sends stats.chupathingy.org the scoreboard as your game saw it (your own line in full; everyone else's name, team, place, score, kills and deaths) with your player ID. Your copy also confirms your line in the host's report, which puts the game on your profile. To turn both off for games you join, set `report_joined_games = false` under `[network]` in `config.toml`. |
| Stop sharing your hosted games' stats | Set `report_events = false` under `[network]` in `config.toml`. It's on by default: the games you host send their Delta Stats (kills with positions, accuracy, medals, objectives) to stats.chupathingy.org for its match pages and heatmaps. Never anyone's address or hardware ID; OpenCE players are only their name, team and that game's score, kills and deaths. This stops Delta Stats only: a listed game's carnage report is still sent. To keep your hosted games listed but send neither, set `report_hosted_games = false`; `list_hosted_games = false` stops the listing too. All of these are also on the Privacy screen in Settings (Gearbox menus). |
| Make an account | On [stats.chupathingy.org/profile](https://stats.chupathingy.org/profile), or press **Start** in Online Games to make one for the player you already are. |
| Link the game without a browser (Steam Deck, Game Mode) | In Online Games, press **RB** (or **C** on the keyboard) for Link Profile. On your phone or computer, go to [stats.chupathingy.org/connect](https://stats.chupathingy.org/connect), enter the code the game shows (or scan its QR code), then press **A** in the game to confirm. |
| Use the PC menus | Set `menus = "pc"` under `[display]` in `config.toml`. **Multiplayer → Join Game → Server Browser** lists the public games of every compatible build and the games of stats.chupathingy.org, each once. |
| List a game from a build that can't list itself | Sign in on the site, open **Host a Game**, and paste your invite link. |

What to know:

<p align="center">
<img src="docs/screenshots/site-games.jpg" width="49%" alt="stats.chupathingy.org: games and recent games">
<img src="docs/screenshots/site-medals.jpg" width="49%" alt="stats.chupathingy.org: medals">
</p>

## Settings on the console

The console has no settings file yet. Its switches are small text files in
the game's folder (`D:\`, beside `default.xbe`): an empty file turns a
switch on.

**Stats, on [stats.chupathingy.org](https://stats.chupathingy.org)**
- A carnage report for every finished game, with medals
- Service records and leaderboards, with confirmed players (your games count toward you, whatever name you use)
- Accounts, made on the site or from the game, with an encrypted backup of your player identity
- Listing a game hosted from a build that can't list itself, by its invite link

**The game**
- The campaign, split screen and System Link, running natively (no emulator)
- LAN parties the old way: System Link on your own network and split screen need no internet and no account
- High-res HUD and text, and widescreen menus
- Controller prompts for Xbox, PlayStation and Nintendo pads, and the keyboard
- Grifball and Bomb games, with a court to play them on
- Online co-op: host the campaign on the Internet for friends ([how](#online-co-op))
- Surround sound (5.1 and 7.1), Doppler shifts and sound cones as on the Xbox, and an optional headphone 3D mode, all set on one Audio Setup screen
- The intro, attract and cutscene movies, on every desktop build and Android
- Community Xbox map packs, such as Halo CE+, beside the retail maps ([Xbox map packs](#xbox-map-packs))
- Text chat in games with other Chupathingy players, and a follow camera while you wait to respawn
- Updates itself on Windows, Linux and Android: it checks for new releases when it starts, asks first, and only trusts the files a signed list of update files names (on Mac it opens the download)

**Platforms**

| | Windows | Mac | Linux | Android |
| --- | --- | --- | --- | --- |
| The game | Yes | Yes, Apple silicon and Intel | Yes | Yes |
| Online Games, hosting, stats | Yes | Yes | Yes | Yes |
| Dedicated server | The game, for a test | The game, for a test | Yes: x86, x64 and arm64, and Docker | |
| Updates itself | Yes | Opens the download page | Yes; not the AppImage | Yes |
| Halo PC (Custom Edition) maps | Yes | Yes | Yes | Yes, see below |
| HaloMD maps | Yes | Yes | Yes | Yes, see below |
| Server Browser in the PC menus | Yes | Yes | Yes | Yes |
| Halo PC CD and CE installer import at first start | Yes | Yes | Yes | Not yet |
| Discord presence and Join | Yes | Yes | Yes | No |
| Choice of audio output, surround and Audio Setup | Yes | Yes | Yes | No (the system's) |
| Crash reports | Yes | Yes | Yes | Not yet |
| Intro, attract and cutscene movies (Bink, with FFmpeg) | Yes | Yes | Yes | Yes |

Some defaults differ by platform: anti-aliasing is MSAA 2x on 64-bit
computers and off on 32-bit ones and Android, and the Mac starts in a window
instead of fullscreen. The movies are the ones on your disc's `bink` folder,
which the first-start import copies; a movie that is missing is skipped. On a Steam Deck in Game Mode, the game
shows no system dialogs (they can crash there): the main menu's version line
says when a new version is out, and you update from Desktop Mode.

## Builds

- **Releases** are the stable builds, on the [Releases page](https://github.com/ChupathingyCE/chupathingyce/releases/latest). The game updates itself from them.
- **Nightly** is the [`nightly` pre-release](https://github.com/ChupathingyCE/chupathingyce/releases/tag/nightly): main, built every day at 04:00 UTC when it has changed. For early birds; it doesn't update itself.
- **Testing** builds are hand-rolled builds of a branch, published as pre-releases named "(test builds)". The current one is the [Grifball test](https://chupathingy.org/grifball).
- **Linux AppImage** is on the test builds for now.
- **What's coming** is the [0.8.0 milestone](https://github.com/ChupathingyCE/chupathingyce/milestone/5) and its unreleased [release notes](docs/release-notes/0.8.0.md).

## Online co-op

The campaign can be hosted for friends over the Internet:

1. Multiplayer > **CREATE GAME** > **INTERNET**.
2. On the map list, switch the chooser in the top row from MULTIPLAYER to
   **SINGLEPLAYER**, then pick a level and a difficulty.
3. Continue to **Server Setup** (friendly fire, extra enemies, player
   collisions and listing are set there) and start the game.

Friends join it like any other game. The CO-OP CAMPAIGN entry of the
Multiplayer menu is the Xbox's split screen co-op on one computer, not an
online game. The original Xbox can't host or join online co-op.

## Halo PC maps

Every Chupathingy build also plays Halo PC multiplayer maps: Windows
(32-bit and 64-bit), Mac, Linux (32-bit and 64-bit), Android and the
dedicated server. Each kind has a folder of its own beside the game's
`maps` folder:

| Folder | What goes in it | Played online as |
| --- | --- | --- |
| `maps_ce` | Halo PC Custom Edition maps, with Custom Edition's `bitmaps.map`, `sounds.map` and `loc.map` | `<name>@ce` |
| `maps_md` | HaloMD maps (below) | `<name>@md` |
| `maps_pc` | the maps of your own Halo PC (retail) disc | `<name>@pc` |

Where the game's `maps` folder is:

| Platform | The folder holding `maps`, `maps_ce`, `maps_md`, `maps_pc` and the map packs' `maps_<pack>` |
| --- | --- |
| `bypass_security.txt` | Starts the network in its open mode, which the internet and the PC builds need. The packaging script writes it; `--secure` leaves it out for console-only System Link. |
| `game_list.txt` | One line, a host or address (`:port` optional): where to get the game list if `warthog.milenko.org` doesn't resolve. |
| `join.txt` | One line: a host's LAN address, or an invite (`halo://join/…`). |
| `trace.txt` | Detailed network traces in `debug.txt`, for reporting a join that fails. Other hosts appear only as tags (`enet#` and six hex digits), never by their network cards' addresses; with it, `debug.txt` also gives XLink Kai's address for the console. |
| `large_caches.txt` | On a 128 MB console, a bigger texture cache (44 MB; 64 MB on a 256 MB console). On a stock 64 MB console it shows a message and keeps the Xbox's own. |
| `brokers.txt` | The signalling servers internet play uses, one `host:port` a line, if the built-in ones can't be reached. |
| `xlink.txt` | Planned: XLink Kai addressing ([docs/xlink.md](docs/xlink.md)). Today `debug.txt` says whether the console's address is already Kai's (yes or no; Kai's address itself with `trace.txt`). |

The game writes its log to `D:\debug.txt`; attach it to bug reports.

## Warthog, ChupathingyCE and Delta

Until 0.8.0, `debug.txt` was beside `maps` (and on Windows `halo.log` beside
`halo.exe`). The game leaves an old one where it is.

Copy the `.map` files from your own Halo PC install. On Android, Import
maps (hold the app's icon, or the touch controls' Options in the game)
picks `.map` files or `.zip` archives with Android's own file picker and
puts each in its folder. Include `bitmaps.map`,
`sounds.map` and `loc.map` from Custom Edition in `maps_ce`: every Halo PC
map reads them. Halo PC's own `ui.map` there adds its map names and
pictures. The maps appear in the multiplayer map list after the Xbox maps,
marked [CE], [MD] or [PC]. In Online Games, a game on a Halo PC map is
badged, and it can be joined only by players who have that map: the game
says which file is missing and where it goes. The Xbox maps from your disc
image are still needed. Chupathingy doesn't come with any of these files.

**HD HUDs on Custom Edition maps.** Custom Edition maps can draw their HUD in
high resolution. For the HD HUD on every map that uses Custom Edition's shared
HUD, use the `bitmaps.map`, `sounds.map` and `loc.map` from Aerocatia's
[halopc-restored](https://github.com/Aerocatia/halopc-restored/releases/tag/r220)
release r220 (the "custom edition full shared" package) in `maps_ce` instead of
the stock ones; keep a copy of yours. Maps with their own HUD use theirs.

**Playing with OpenCE players.** OpenCE plays Custom Edition maps from
a `custom_maps` folder. A game on a Custom Edition map is named the same way
for both, so their players with the map join our games and we join theirs.
HaloMD and Halo PC retail games are Chupathingy's only: a player on
OpenCE is told the map is missing. Between Chupathingy players,
the map's identity (its name and a hash of its file) is checked, so a
player with a different file of the same name is told so.

**Downloading a missing map.** When a game you join is on a Custom Edition
map you don't have, the game offers to download it from the community map
repositories (the public map repositories Halo PC players have used for
years, as Chimera downloads from them: maps.halonet.net, then its mirrors
maps2 and maps1, over HTTPS): "Download <name>.map from the community map
repository?", with Yes, No and Don't ask again. The download shows its
progress and can be cancelled. Before the map goes in `maps_ce`, the game
checks it: no more than 200 MB, downloaded and unpacked; only `<name>.map`
taken from the archive, never a path from it; its size and CRC-32 as the
archive states them; a Custom Edition multiplayer map whose header names the
map asked for; and the host's version of it (the checksum in its header,
which the host advertises with its game). It never replaces a file that is
there. Then the game joins, and the map's own checks run as for any map. A
downloaded map is listed in `maps_ce/downloaded_maps.txt`: its scripts are
held to a downloaded map's tighter rules. `network.map_downloads` in
`config.toml` (or Network Settings in the PC menus) says `"ask"` (the
default), `"always"` or `"never"`. Desktop and Android builds download maps;
the dedicated server doesn't. HaloMD and Halo PC retail maps aren't
downloaded, and neither are Halo PC's own maps and files (Blood Gulch, Putput,
the campaign's levels, `ui`, `bitmaps` and the rest, also under a hashed
name), though the repositories have copies: they're Microsoft's and
Gearbox's, so the game asks you to copy them from your own Halo PC or Custom
Edition install instead ("Install this map yourself").

**When the host changes map.** If a game you're in moves on to a Custom
Edition map you don't have (a dedicated server's playlist going to its next
map, say), the game leaves it, offers the map the same way over the main
menu (or downloads it at once, with `"always"`), and then joins the same host
again, in its next lobby or the game in progress. The game's protocol gives a
machine about a minute to load a map before the host drops it, too short for
most downloads, so the player leaves and comes back rather than holding the
game up. If the map isn't downloaded (you say no, cancel it, or it fails),
the main menu says the map is missing, as it does with `"never"`.

**Maps named with a hash.** Hosts on OpenCE name a shared Custom
Edition map `<name>_<8 hex digits>` (a game on `Sneak_af352483`, say), the
digits the start of a hash of the whole map file. Chupathingy plays such a
game from your `<name>.map` (`sneak.map`) when its hash matches, and the
downloader fetches `<name>.map` for it, checking the hash too. If your
`<name>.map` is another version, the download is kept beside it as
`<name>_<digits>.map`.

**Older folders.** Earlier Chupathingy versions kept Custom Edition maps
in `maps/ce/` and HaloMD maps in `md_maps/`, and OpenCE keeps its Custom
Edition maps in `custom_maps/`. The game still plays the maps in all three.
At its first start it offers to move `maps/ce/` to `maps_ce` and `md_maps/`
to `maps_md` (each folder is moved whole: nothing is copied or deleted, and
a folder that can't be moved stays where it is). `custom_maps/` is OpenCE's
and stays where it is. `game.move_old_map_folders` in `config.toml` says
`"ask"`, `"yes"` or `"no"`; Android moves them without asking. A map file
named `<name>@ce.map` (`@md`, `@pc`), as OpenCE names them, is
found in its folder or in `maps/` itself.

**Common mistake:** don't point `maps` itself at a Halo PC maps folder, or copy
Halo PC maps straight into it. The game can't start with Halo PC's `ui.map`
in place of the Xbox one, and Halo PC maps in `maps/` itself don't play. Keep
the Xbox maps in `maps/` and the Halo PC maps in `maps_ce`, `maps_md` or
`maps_pc`.

On Android, two limits apply. The maps need a fixed 23 MB of the app's
memory, which Android's Java runtime takes on some devices with a very large
Java heap (more than about 700 MB, such as some gaming handhelds). There
the Halo PC maps are not listed, and the log says `cannot reserve Custom
Edition maps' tag cache`. And the phone's texture cache is smaller than a
computer's, so on the heaviest maps (Foundation) some surfaces can show the
wrong textures in their busiest views. See [port/android/README.md](port/android/README.md).

### HaloMD maps

Chupathingy also plays HaloMD's multiplayer maps (the Mac Halo community's
maps, made for Halo PC 1.0), in every build. Bring your own: download
the maps you want from HaloMD's mod list, and put their `.map` files in
`maps_md`, beside the game's `maps` folder.

They need Custom Edition's `bitmaps.map`, `sounds.map` and `loc.map` in
`maps_ce` too: a HaloMD map keeps Halo's own textures and sounds in those
shared files, and the game reads them from Custom Edition's copies.

The maps appear in the multiplayer map list after the Halo PC maps, marked
[MD], with their names from HaloMD's list. Online, a HaloMD map is played as
`<name>@md` (`bgplus_5@md`), badged HALOMD in Online Games, and joined only by
players who have the same map file.

HaloMD's plug-ins aren't part of Chupathingy. A few maps were made for one:
the visible-object and bigger-BSP limits they raised are already raised here,
and the widescreen view is the game's own, but the maps made for gameplay
plug-ins (3rd Person, Rocket Surfing, Spartan) play without them, as plain
Halo.

## Xbox map packs

Chupathingy also plays community packs of Xbox maps, such as Halo CE+ X
(Original Xbox) by VKMT, on every build, the original Xbox's too. Such a pack
is a set of Xbox-format maps, usually saved under the Xbox's own maps' file
names (CE+'s Abyss is its `bloodgulch.map`), so each pack gets a folder of
its own beside `maps`, named for the pack, and its maps never replace or get
confused with the retail ones:

| Folder | Pack | Played online as |
| --- | --- | --- |
| `maps_cex` (also `maps_ceplus`, `maps_ce+`) | Halo CE+ | `<name>@cex` (`bloodgulch@cex`) |
| `maps_<name>` | any other pack | `<file>@<name>` |

To install a pack, copy the contents of the pack's `maps` folder, its
`maplist.txt` included, into its folder: for Halo CE+, into `maps_cex`. Leave
your retail maps in `maps`. Chupathingy doesn't host or ship pack files: get
the pack from its release page. A pack isn't its folder's name: a map the
library knows is its pack's in any `maps_*` folder, so players whose folders
are called differently still play together.

The pack's maps appear in the multiplayer map lists after the others, marked
with the pack's tag ([CE+]) and named by the library of known packs
(`port/assets/maps`), by the pack's `maplist.txt`, or by their file names.
Online they show as "Abyss" with a CE+ badge, and a player joins only with
the same version of the pack's map: one without it is told which pack and
folder it needs ("This game uses Halo CE+'s Abyss (bloodgulch.map in
maps_cex)"). OpenCE and older Chupathingy builds can't join these games.

The pack's own `ui.map` isn't used: the game's menus stay the game's. To play
with the pack's own menus instead, set `game.pack_menus = "cex"` in
`config.toml` and restart. A pack folder needs a name of 1 to 8 letters and
digits, other than `ce`, `md`, `pc`, `de`, `fr`, `es` and `it`.

## Settings (config.toml)

Most of what you can change lives in the game's menus, but everything the
port adds is in one file, `config.toml`. The game writes it the first time it
starts, with every setting listed, commented out at its default, and a line
or two saying what each one does. To change one, remove the `#` in front of it
and edit the value; the game reads the file when it starts, so restart it
after a change. A line that starts with `#` does nothing. If a change seems
to do nothing, look in `logs/debug.txt` for lines starting `config.toml` or
`settings:`: they name a misspelled setting, one in the wrong `[section]`, a
line still commented out, a line the game skipped because of a mistake (a
value missing its quotes, `menus = pc` for `menus = "pc"`, or `False` for
`false`; the other lines still count), a word a setting does not know, or a
`config.toml` that is not the one the game reads (such as `config.toml.txt`
on Windows).

| Platform | config.toml is |
| --- | --- |
| Windows | next to `halo.exe` |
| Mac | `~/Library/Application Support/ChupathingyCE/config.toml` |
| Linux | next to the `halo` executable |
| Android | in the game's data folder, `/sdcard/Android/data/dev.horrible.chupathingyce/files` |

A few people look for most:

| Setting | What it does |
| --- | --- |
| `display.mode` | `"fullscreen"`, `"borderless"` or `"windowed"`. F11 switches between a window and the whole screen. When it is set, `display.fullscreen` is ignored; when it is empty, `display.fullscreen = true` is borderless and `false` a window. |
| `display.window_scale` | The window's size, as a multiple of 640x480. |
| `display.vsync`, `display.max_fps` | Vertical sync, and a frame rate cap with V-Sync off (0 for twice the display's refresh rate, -1 for none). |
| `display.interpolation` | `false` keeps the game's own 30 frames a second; Video Setup's Frame Rate at 30 (Xbox) sets it. |
| `display.direct_camera` | Instant aim: the view turns the frame the mouse, stick or touch moves it. On by default; config.toml only. |
| `display.menus` | `"chupathingy"` (the default: the Xbox's look with Halo PC's settings, profiles and more), `"xbox"` (as close to the console as can be) or `"gearbox"` (Halo PC's menus with their Server Browser; `"pc"` still works). |
| `display.high_res_text` | `true` draws the game's text (menus and HUD) with sharp fonts at your resolution; `false` uses the maps' original bitmap fonts. The Server Browser and the moderation overlays use the system's fonts either way. For a blocky look overall, also set `display.resolution_scaling = "original"`. |
| `display.high_res_font`, `display.high_res_font_large` | Your own font file for the sharp text and Online Games, in place of the built-in fonts: put it in `data/fonts` beside `maps` and give its name (`high_res_font = "MyFont.ttf"`), or a full path. `_large` sets one for the large text only. Empty uses the built-in fonts; a file that can't be read is noted in `logs/debug.txt` and the built-in font is used. The font is yours: Chupathingy ships none but its own free ones, and never sends yours anywhere. |
| `display.field_of_view` | How wide the first-person view is, in degrees: 70 (the Xbox's) by default, 50 to 120. |
| `display.far_clip_distance` | How far the world is drawn: 1024 (the Xbox's) by default, up to 10000, for large maps that cut off in the distance. Drawing only. |
| `display.match_timer` | `true` shows a timed multiplayer game's time left at the top of the screen (off by default). |
| `display.player_names` | Names over players' heads: `"all"`, `"allies"`, `"enemies"` or `"none"`. |
| `game.unlock_all_levels` | `true` gives new profiles every campaign level. By default a new profile starts at The Pillar of Autumn, as on the Xbox. Profiles made by earlier versions had every level open; `profile_reset_campaign` in the console starts one over. |
| `audio.volume`, `audio.music_volume`, `audio.effects_volume` | Volumes, from 0.0 to 1.0. |
| `audio.device` | The output the sound plays on, by name (Audio Setup lists them); `"default"` follows the system's default output. |
| `audio.channels` | `"auto"` follows your output (5.1 on a 6-channel device, 7.1 on 8, else stereo); `"stereo"`, `"5.1"` or `"7.1"` set it. Desktop only; read at start. |
| `audio.hrtf` | Headphone 3D sound: `"off"` (the default), `"on"`, or `"auto"` (on when the output's name says headphones). Stereo output only. |
| `audio.doppler` | Moving sounds shift in pitch as on the Xbox (on by default). |
| `input.mouse_sensitivity`, `input.invert_mouse` | Mouse aim. |
| `input.keyboard_prompts` | Button prompts follow the device you used last: keys after the keyboard or mouse, as Halo PC did, and buttons after a controller. This setting is no longer in the menus; `false` keeps the controller's buttons everywhere. |
| `controls.*` | Every key: `controls.jump = "Space"`, or two at once, such as `"F, Mouse 4"`. |
| `network.browser_url` | The game list Online Games shows (stats.chupathingy.org). |
| `network.copy_to_clipboard` | `false` (the default) leaves the clipboard alone; `true` puts the invite link of a game you host on it. The link is also in the log and under INVITE LINK in the Gearbox menus' Server Settings. |
| `network.host_public` | Whether games you host are listed for everyone (true) or only joinable by invite (false). |
| `update.auto` | Whether the game updates itself. |
| `paths.data`, `paths.saves` | Where the maps and the saves are, if not the usual place. |
| `paths.logs`, `paths.cache` | Where the logs and the cache go, if not the usual place (below). |

Settings you leave at their default follow each new version's default, so
leaving the file alone is always safe. Every setting can also be given as an
environment variable for one run (the file lists each one's name), which
overrides the file.

## Run a server

The Chupathingy Dedicated Server hosts a playlist of games around the clock,
with no player of its own, and lists them on stats.chupathingy.org and in the
Server Browser. It is its own download for Linux on x86, x64 and arm64
(Oracle Cloud's free tier and Raspberry Pis included): one file, no libraries
to install, and no port forwarding. See [server/README.md](server/README.md).

## Cross-play and Delta

- **Our own releases.** Chupathingy has its own code, releases and version
  numbers, and doesn't change under you every few hours.
- **Playing together.** Chupathingy players and OpenCE players
  join each other's games when their network versions match. Chupathingy
  follows OpenCE's versions: a cross-play test
  runs against each new one, and once it passes, a signed table tells
  existing Chupathingy builds which network versions to announce and join,
  without a new download. A change to how multiplayer plays still needs a
  release. The line at the top of this page says which versions this
  release plays with.
- **Delta** is Chupathingy's network family ([docs/delta.md](docs/delta.md)):
  the game list, stats, profile links and dedicated server admin tools on
  [stats.chupathingy.org](https://stats.chupathingy.org), and a channel between
  Chupathingy machines for features of our own (how builds play together:
  [docs/delta-compatibility.md](docs/delta-compatibility.md)). Games with OpenCE players
  use the shared game protocol, so everyone plays together.

## Building it yourself

Warthog builds on macOS today, with [OXDK](https://github.com/MrMilenko/OXDK),
clang and lld-link (Linux should work the same way, untested). Nothing from
Microsoft's SDKs is in this repository, and none of it may be committed:
you supply your own.

- Python 3, [ninja](https://ninja-build.org/) and LLVM (clang and
  lld-link). On macOS: `brew install llvm lld ninja`.
- [OXDK](https://github.com/MrMilenko/OXDK) in `~/OXDK` (or `--oxdk DIR`,
  or `OXDK_DIR`), with its `cxbe` built, and the 5933 Xbox SDK's libraries
  and headers in OXDK's `xbox/xdk/lib` and `xbox/xdk/include`, as OXDK's
  README says.
- An Aug 2001 Direct3D library, `d3d8ntpr.lib` (the `aug01` i386 build),
  given to `configure.py --xbox-d3d8`.

```sh
python3 configure.py --xbox-d3d8 /path/to/d3d8ntpr.lib
ninja xbox                       # build/xbox/default.xbe
port/xbox/tests/run.sh --xbox    # the host tests
python3 tools/xbox_package.py --maps /path/to/your/maps --out ~/Downloads/Warthog-xbox
```

`xbox_package.py` lays out `default.xbe` and your own `maps/` as one folder
for the console's hard disk, with an empty `bypass_security.txt` (`--secure`
leaves it out). Copy it to the console (FTP on a modified console), for
example to `E:\Games\Warthog\`, and start `default.xbe`.
[docs/building.md](docs/building.md) has every step, the versions tested
and the options. `xbox360`, `wiiu` and `switch` build the other consoles'
stubs where their toolchains are installed
([docs/consoles.md](docs/consoles.md)).

### Xbox Developer Mode (UWP)

The UWP port runs the 32-bit Warthog game as a guest inside a 64-bit
AppContainer host. It keeps the original `ninja xbox` target separate and
uses `ninja uwp_guest` plus a CMake-built host. On first launch it lets the
player choose internal or removable storage, select a legally owned Halo
Xbox ISO/XISO, and extracts the required maps once. See
[port/xboxseries/README.md](port/xboxseries/README.md) for prerequisites, building, packaging and
storage locations.

The console ports (original Xbox, Xbox 360, Switch, Wii U) are built from their own
repositories, with this one as a submodule: "Console ports" in
[docs/building.md](docs/building.md#console-ports).

The version being made is in `VERSION`. Releases are built and published by
the project's release workflow; the builds on this repository's Actions page
are for checking changes.

Milenko, Warthog's maintainer, doesn't use Windows, so Warthog builds on
macOS with OXDK and clang today, and the Windows side is open. If you know
the Xbox's Windows tools, we'd love your help to set up and document:

- a Windows build path: a Visual Studio project or solution, or the same
  clang and OXDK build run on Windows;
- the official XDK and RDK tooling: what installs where, and how the build
  finds it;
- debugging through XBDM on a development kit;
- a Windows CI job or a written build recipe that others can follow.

Say hello in #warthog-dev on the [Discord](https://discord.gg/chupathingy)
so we can coordinate, and see [CONTRIBUTING.md](CONTRIBUTING.md).

## Testing

**Development kit or modified console.** Copy the folder to the hard disk
(FTP, or the development kit's tools) and launch `default.xbe`. The game
logs `halo:` lines to the debug monitor (XBDM) and writes `D:\debug.txt`.
It is a debug build: an assertion stops it and shows the error on screen
and in `debug.txt`. A watchdog reports a main loop that stalls;
`D:\trace.txt` turns on the bring-up traces.

**xemu.** vsod99's fork of OpenCE (`vsod99/halo-ce-universal`, branch
`xbox-backport`, CC0) has a development loop, `tools/xbox_dev.py`: it packs
an ISO, boots it in xemu at 128 MB with NAT and streams the serial log. Its
ISO and xemu steps work for any XBE. xemu needs the user's own MCPX boot
ROM, flash image and hard disk image.

**On the host.** `port/xbox/tests/run.sh` builds the console's parsers
(the game list, the settings record's fold, `join.txt`, the memory class,
XLink's addressing) on the computer under AddressSanitizer, with a fuzzer;
`--xbox` adds the Xbox build's own checks.

## Cross-play

The console builds 16 players on 16 machines and the Xbox's 2,048 objects;
the PCs build 128, 128 and 8,192, and both send their game settings whole
(13,120 bytes at 128 slots, unchanged through network version 28). As a client
of a PC host, the console folds the PCs' 128-slot game settings record and
Slayer's state into its own 16 slots as they arrive, checks the record's
layout against its own at compile time, and leaves a game that grows past
its slots. A desktop build with the console's limits
(`configure.py --console-limits`) plays whole games against a PC dedicated
server on one computer that way.

Hosting PC players, and the PCs' object indices past 2,048, need a bigger
game state than the Xbox's (its pools fill all but 948 bytes of 0x305000
bytes): the route is the game state as ordinary virtual memory on a 128 MB
console. The field-by-field analysis and the plan are in
[docs/cross-play.md](docs/cross-play.md).

Internet games go through the PCs' own tunnel (`port/linux/src/p2p.c`,
built for the console as a joiner), which needs XNet in its open mode
(`D:\bypass_security.txt`). Memory detection and the console's switches are in
`port/xbox/README.md`; XLink Kai is in
[docs/xlink.md](docs/xlink.md).

## Keeping in step with ChupathingyCE

Warthog merges ChupathingyCE (a merge, never a rebase: Warthog's main is
public and is never rewritten): conflicts are resolved in favor of
ChupathingyCE's code with the console's switches kept, and `ninja xbox`,
the desktop builds and the host tests must pass. Each console switch
Warthog keeps in a shared file is work at every merge.

## License

Warthog is free software: you can redistribute it and modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License or (at your option) any later
version (`GPL-3.0-or-later`), with the console SDK permission below. It comes
with no warranty. The full text is in [LICENSE](LICENSE).

- **From this change onward.** Warthog follows ChupathingyCE, which is GPL
  from release 0.8.0. The source at every commit before the change was
  released under [CC0 1.0](LICENSES/CC0-1.0.txt) and stays that way.
- **What Warthog builds on.** The Halo CE decompilation
  ([punpckhdq/halo](https://github.com/punpckhdq/halo) and
  [bnunu/halo-1](https://github.com/bnunu/halo-1)) and OpenCE are released
  under CC0 1.0, which lets anyone build on them under any terms. Their
  repositories stay CC0.
- **Third-party code, fonts and art** keep their own licenses, which are
  beside them (`port/third_party`, `port/assets/fonts`,
  `port/linux/ui/fonts`, `port/assets/prompts`, `port/assets/hud`,
  `port/assets/hrtf`), and the release downloads
  carry their notices.
- **Halo itself is not covered.** The license covers the work of
  Chupathingy's authors. It gives no rights in Halo, its maps or anything
  else Microsoft owns.
- **Source.** Each release's source is its tag in this repository.

### Console SDK permission

You may build Warthog yourself with a console maker's SDK (see "Building it
yourself"), and share what you build:

> **Additional permission under GNU GPL version 3 section 7**
>
> If you modify this Program, or any covered work, by linking or combining
> it with the libraries of a Microsoft Xbox or Xbox 360 software development
> kit (such as the Xbox Development Kit and the Xbox 360 SDK, including
> their `xnet.lib` and Direct3D libraries), or a modified version of those
> libraries, containing parts covered by the terms of those kits' licenses,
> the licensors of this Program grant you additional permission to convey
> the resulting work. Corresponding Source for a non-source form of such a
> combination shall include the source code of the covered work, and need
> not include the source code of those libraries.

The permission only lifts the GPL's own objection. It grants no rights in
Microsoft's libraries, whose own terms still apply, and you may remove it
from your copies (section 7). Warthog's own published builds are made only
with free, legally available tools and libraries; none contains code from
Microsoft's SDKs.

## Support

Chupathingy is free and always will be. Support through
[Ko-fi](https://ko-fi.com/milenko) goes to running the site and the servers. If
there's a specific developer or port you'd like to support, ask them: they may
not be part of our community.

## Credits

- The decompilation: [punpckhdq/halo](https://github.com/punpckhdq/halo) and
  [bnunu/halo-1](https://github.com/bnunu/halo-1), of the Xbox build 2342.
- The port Chupathingy began as a fork of: OpenCE and its contributors, up to
  October 9, 2026 (see [Where it came from](#where-it-came-from)).
- Later fixes taken from OpenCE (CC0), with their authors kept in the
  history: startupfoundry, xshxdex98, hkmodd, eltanschauung, justinfilip,
  bertoaldo, MrBruh, addisonbair, thelinkin3000, bmanturner and chestahh (by
  their GitHub handles). The wire format tool (`tools/network_format.py`) is
  addisonbair's and MrBruh's.
- Chupathingy: [Milenko](https://github.com/MrMilenko) and contributors. The
  icon is MrBruh's helmet, with tusks.
- Fonts: Cyborg Interface Gothic, the high-res text's font, by fizzy, made
  to match Halo's interface font from Overpass and glyphs of their own
  (SIL OFL); [Overpass](https://github.com/RedHatOfficial/Overpass) (SIL OFL);
  [Noto Sans](https://fonts.google.com/noto) (SIL OFL) and
  [Kenney's Input Prompts](https://kenney.nl/assets/input-prompts) (CC0); the
  scoreboard's platform logos are one-color marks from
  [Simple Icons](https://simpleicons.org) (CC0) and Wikimedia Commons (public
  domain), plus our own Xbox 360 ring and Wii U GamePad; the names and logos
  are their owners' trademarks, used only to identify platforms.
- Controller button icons by snapchipper, CC0 (`port/assets/prompts`).
- Headphone 3D sound (`audio.hrtf`): head-related impulse responses from the
  [SADIE II database](https://www.york.ac.uk/sadie-project/database.html)
  (KEMAR, subject D2), University of York, Apache License 2.0
  (`port/assets/hrtf`).
- High-res HUD art on the Xbox maps: Aerocatia's and Jessie's HUD art from
  [Halo PC Restored](https://github.com/Aerocatia/halopc-restored) (release
  r220), used with Aerocatia's permission on the condition that both are
  credited; `port/assets/hud/NOTICE.md` lists which files are theirs.
- HD HUDs on Custom Edition maps (MCC's "0.5 HUD scale" and Chimera's
  "force HUD use high resolution scale" flags): ugoboom, tested with
  Aerocatia's [halopc-restored](https://github.com/Aerocatia/halopc-restored)
  resource maps.
- Grifball courts: Bomb plays on SlappyThePirate's "Grifball Beta" court
  (`grifball.map`), which is open to use with credit; the map is not
  included.
- HaloMD map names: from [MacGamingMods](https://macgamingmods.com)' public
  HaloMD mod list, so the menus can show each map's own name.
- Missing Custom Edition maps: downloaded from the community map
  repositories, as Chimera uses them (maps.halonet.net, the HAC2 map
  repository, and its mirrors). Chupathingy's downloader is its own code;
  Chimera showed where the repositories are and how they answer.
- Libraries: SDL3, stb, Mbed TLS, miniupnpc, KCP, tomlc17, musl's maths,
  Expat, Monocypher, zlib, SMAA, and Project Nayuki's QR Code
  generator. Their licenses are
  beside them in `port/third_party`.

### Contributors

- [thelinkin3000](https://github.com/thelinkin3000): the Nintendo Switch
  port, being brought into Warthog.
- CrunchBite (XLink Kai's developer): XLink Kai's addressing for consoles
  (docs/xlink.md), and the idea of checking the console's memory and saying
  so on screen instead of crashing.
- vsod99: the xemu development loop and the memory probe behind the
  cross-play plan.

- [Tyberious](https://github.com/Tyberious) (Jeff Clark): Xbox ADPCM decoding
  without the 344 Hz buzz, vertex constant serials that never wrap, pose
  snapping for frame interpolation, and the clip-space position kept on the
  desktop (no more first-person vertex spikes).
- [thelinkin3000](https://github.com/thelinkin3000): the motion sensor for
  three and four local players (a split screen crash), and split screen
  dividers where the views meet on a wide screen.
- [xshxdex98](https://github.com/xshxdex98) ([DamnationCE](https://github.com/xshxdex98/DamnationCE)):
  the pause menu's QUIT by mouse or keyboard, and lens flares out of view no
  longer tested.
- [zimm3rmann](https://github.com/zimm3rmann): visibility test slot 0 kept
  apart from the scratch query (lens flares sharing a result).
- [nsafran1217](https://github.com/nsafran1217) (Nathan Safran): no hang
  where the visibility results buffer cannot be mapped.
- [saulob](https://github.com/saulob): the first menu frame's colors.
- [natsu-anon](https://github.com/natsu-anon): the cursor hidden during play.
- [lantos1618](https://github.com/lantos1618): no ghosting in the zoom effect
  at high resolutions; on macOS, Command-W does not quit and Command-Q asks
  twice.
- [pfista](https://github.com/pfista) (Michael Pfister): projectile trails
  ready before a weapon's first shot.
- [JoshRob297](https://github.com/JoshRob297): hosts that run their own games
  (the dedicated servers) no longer refuse every join after one arrived as a
  game ended.
- [HiIAmMoot](https://github.com/HiIAmMoot) (Mootjuh): the Android menus by
  touch, and `debug.solo_game` (a multiplayer game started alone).
- [oatkrs](https://github.com/oatkrs) (Utkarsh): crisp windowed rendering and no
  audio cut-outs on macOS, and a corrupted script thread dropped instead of
  halting the game.

### Findings and testing

- [bnunu](https://github.com/bnunu): Custom Edition map
  findings from the custom-edition-maps branch, credited in the commits they
  led to, and the larger texture cache for Halo PC maps, which that branch had
  first.
- Sabriel and ugoboom: the Halo PC map reports and regression lists behind
  most of the Custom Edition fixes.
- Sabriel, roundzabout and Mega: Grifball playtesting and the bugs and
  tweaks it turned up.
- MrBruh (build-145's Custom
  Edition loader, from DamnationCE's work by
  [xshxdex98](https://github.com/xshxdex98)): maps that need OpenSauce refused
  with a clear reason, the missing-map message when joining, and the
  `custom_maps\<name>` naming our hosts and clients share with OpenCE.

Halo is a trademark of Microsoft. Chupathingy is a fan project, not made or
endorsed by Microsoft, Bungie or 343 Industries, and includes none of the
game's content. The code is released under the GNU GPL, version 3 or later
(see [License](#license)).
