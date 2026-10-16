<p align="center"><img src="docs/icon-160.png" width="120" alt=""></p>

<h1 align="center">Warthog</h1>

<p align="center"><b>Halo: Combat Evolved back on the original Xbox: ChupathingyCE's console line, playing online with the PC builds.</b></p>

<p align="center">
<a href="https://github.com/ChupathingyCE/chupathingyce">ChupathingyCE</a> ·
<a href="https://halo.milenko.org">Games online now</a> ·
<a href="https://discord.gg/4BUm2FwuCB">Discord</a>
</p>

> Warthog follows ChupathingyCE: the shared game code comes from there, and this repository adds only the consoles' own code. You need your own copy of Halo; no SDK or game files are included.

> **Built on ChupathingyCE (network version 28, for online play with OpenCE's build-201).** Warthog joins games hosted on network versions 11 through 30, by ChupathingyCE and other builds alike, with up to 16 players. There's no release yet: Warthog is in development, and for now it runs on development kits and modified consoles.

Warthog is the Halo: Combat Evolved decompilation built back for the console
it was written for, on the same code as
[ChupathingyCE](https://github.com/ChupathingyCE/chupathingyce) (the
Windows, Mac, Linux and Android builds). The goal: Halo as it shipped on the
Xbox, with its memory, caches and limits, that also plays online with the
PC builds. Expect rough edges, and please report them.

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

[docs/consoles.md](docs/consoles.md) has where each stands.

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

Warthog never provides game files: we don't provide maps, disc images or
other copyrighted game data, and we don't link to them.

## Playing online

| You want to | Do this |
| --- | --- |
| Join a game online | **Multiplayer → ONLINE GAMES**, pick a game, press **A**. Needs `bypass_security.txt` (below), which the packaging script writes. |
| Join a game on your network | **Multiplayer → System Link**, as on the disc. PC hosts on your network show up too. |
| Join one host directly | Put its LAN address (`192.168.1.20`) or its invite link in `D:\join.txt`; System Link joins the address, ONLINE GAMES' **Y** the invite. |
| See your stats | Not yet on the console. |

What to know:

- **16 players.** The console plays games of up to 16 players, as the Xbox
  did. It leaves a game that grows past that, as it leaves a full one. With
  Delta (below), hosts will know the console's limit and keep room for it.
- **No network co-op.** OpenCE's network co-op isn't built for the
  original Xbox for now. Co-op games are left out of the console's lists,
  and a game that turns co-op is left with a message. Split screen and the
  campaign on the console itself play as they always did.
- **Network versions.** The console joins hosts of network versions 11
  through 30. A host of another version is refused with the reason.
- **Your address stays private.** The game's log never shows a public IP
  address, only a tag.

## Settings on the console

The console has no settings file yet. Its switches are small text files in
the game's folder (`D:\`, beside `default.xbe`): an empty file turns a
switch on.

| File | What it does |
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

- **ChupathingyCE to Warthog, one way.** Warthog merges
  ChupathingyCE: changes flow from ChupathingyCE into Warthog, never back
  through it. Fixes to the shared game code go to ChupathingyCE first (and
  from there to OpenCE). Warthog keeps only what the console needs on top.
- **Playing together.** The game protocol is the PC builds', byte for byte:
  the console adapts to their network version and their 128-slot game
  settings, never the other way ([docs/cross-play.md](docs/cross-play.md)).
- **Delta.** ChupathingyCE's network family ([docs/delta.md](docs/delta.md)):
  everything our machines and services say beyond OpenCE's protocol. Warthog
  is to be a full Delta machine: Delta Peer with an `xbox` platform key (its
  16-player and no-co-op caveats, its memory class), the signed legacy
  table, a signed game list so the console needs no TLS, and later stats
  and profile links.

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

## Help wanted: Windows tooling

Milenko, Warthog's maintainer, doesn't use Windows, so Warthog builds on
macOS with OXDK and clang today, and the Windows side is open. If you know
the Xbox's Windows tools, we'd love your help to set up and document:

- a Windows build path: a Visual Studio project or solution, or the same
  clang and OXDK build run on Windows;
- the official XDK and RDK tooling: what installs where, and how the build
  finds it;
- debugging through XBDM on a development kit;
- a Windows CI job or a written build recipe that others can follow.

Say hello in #warthog-dev on the [Discord](https://discord.gg/4BUm2FwuCB)
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

## Credits

- The decompilation: [punpckhdq/halo](https://github.com/punpckhdq/halo) and
  [bnunu/halo-1](https://github.com/bnunu/halo-1), of the Xbox build 2342.
- The port: [ChupathingyCE](https://github.com/ChupathingyCE/chupathingyce),
  and OpenCE and its contributors, up to October 9, 2026.
- Warthog: [Milenko](https://github.com/MrMilenko) and contributors. The
  toolchain: [OXDK](https://github.com/MrMilenko/OXDK).
- Libraries: KCP and Monocypher (internet play), zlib, and the
  ChupathingyCE libraries the shared code uses. Their licenses are beside
  them in `port/third_party`.

### Contributors

- [thelinkin3000](https://github.com/thelinkin3000): the Nintendo Switch
  port, being brought into Warthog.
- CrunchBite (XLink Kai's developer): XLink Kai's addressing for consoles
  (docs/xlink.md), and the idea of checking the console's memory and saying
  so on screen instead of crashing.
- vsod99: the xemu development loop and the memory probe behind the
  cross-play plan.

Halo is a trademark of Microsoft. Warthog is a fan project, not made or
endorsed by Microsoft, Bungie or 343 Industries, and includes none of the
game's content. The code is released under [CC0](LICENSE.md).
