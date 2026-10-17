# Contributing to Warthog

Warthog is [ChupathingyCE](https://github.com/ChupathingyCE/chupathingyce)'s
console line: the original Xbox today, other consoles later
(docs/consoles.md). Fixes to the shared game code belong in ChupathingyCE
first; Warthog takes its main regularly (README, "Keeping in step with
ChupathingyCE").

## How changes flow

Code moves one way: ChupathingyCE -> Warthog. Warthog merges
ChupathingyCE, so a fix to the shared game, network or desktop code goes
to ChupathingyCE first and arrives here with the next merge. Pull
requests here are for console-specific changes only: `port/<console>/`, the
console build tools, and shared-code changes behind the console's define
(`HALO_XBOX_CONSOLE`) that leave the desktop builds unchanged.

Never commit, attach or link SDK files (Microsoft's, Sony's, Nintendo's:
headers, libraries, tools, d3d8ntpr.lib, xnet) or game data (maps, sounds,
movies, ui.map, disc images, keys). The SDK and the game are always the
user's own.

## Help wanted: Windows tooling

Milenko, the maintainer, doesn't use Windows, so Warthog builds on macOS
with OXDK and clang today (docs/building.md). We need someone to set up and
document the Windows side: a Visual Studio project or solution, or another
Windows build path; the official XDK and RDK tooling; XBDM and debugging on
a development kit; and a Windows CI job or build recipe. Coordinate in
#warthog-dev on the [Discord](https://discord.gg/chupathingy) before you
start, so the work isn't duplicated. The SDK rules below apply: describe
where the SDK's files go, never commit them.

## Getting involved

You don't have to write code to help.

- **Play and test.** New builds that need trying are in the Testing column of the [roadmap](https://github.com/orgs/ChupathingyCE/projects/1). Pick up the Testers role in the Discord to get pinged when one is ready.
- **Report bugs and share ideas** with the [bug report form](https://github.com/ChupathingyCE/chupathingyce/issues/new?template=bug.yml), or in #bug-reports and #ideas on the [Discord](https://discord.gg/chupathingy). New reports go into the roadmap's Inbox and get sorted each week.
- **Write code.** Issues labeled [good first issue](https://github.com/ChupathingyCE/chupathingyce/labels/good%20first%20issue) are small and self-contained, and each one says where to start. [help wanted](https://github.com/ChupathingyCE/chupathingyce/labels/help%20wanted) issues are a bit bigger. Ask in #dev on the Discord if you get stuck.
- **Host a server, make maps, or help run events.** Ask for the Server Hosts, Mappers or Game Night role in the Discord.

Contributors are credited in the release notes.

## Pull requests

- **Open them against `ChupathingyCE/warthog`'s `main`.** With the GitHub
  CLI, run `gh repo set-default ChupathingyCE/warthog` once in your clone.
- One change per pull request, with how you tested it: a devkit, a modified
  retail console or xemu, and which build.
- Console code stays in `port/<console>/`, and changes to shared code stay
  behind the console's define (`HALO_XBOX_CONSOLE`), so the desktop builds
  are unchanged.
- The game protocol stays the PC builds': the console adapts to their
  network version and layout, never the other way (docs/cross-play.md).
- Nothing logs an IP address whole unless it is a LAN one: go through
  `log_address`.
- Commit messages, pull requests and issues are public, and they're posted
  to the ChupathingyCE Discord.

## License

Warthog is licensed under the GNU GPL, version 3 or later (`GPL-3.0-or-later`, [LICENSE](LICENSE)), with the console SDK permission in the README's [License](README.md#license) section. By opening a pull request, you agree that your contribution is under the same license and permission. Code shared with ChupathingyCE goes there first, under the same terms.

- Changes to the files that stay CC0 (`docs/delta.md`, `port/linux/src/delta_wire.c`, `port/linux/src/delta_wire.h` and `port/linux/include/delta.h`) are under CC0 1.0, as those files are.
- Code or assets from somewhere else must be under a license that works with the GPL (MIT, BSD, zlib, Apache-2.0, CC0, SIL OFL for fonts, and so on), credited, with the license beside them (`port/third_party` for code). Say where they come from in the pull request.

## Using AI tools

You can use AI tools when you contribute. You're responsible for what you submit: understand every line, test it, and be able to explain it in review. Say in the pull request if a large part of it was generated. Pull requests that are mostly generated and untested will be closed.

## Versions

- **`0.MINOR.0`** is a major release, with a theme.
- **`0.MINOR.PATCH`** is a regular release, with features and fixes.
- **A letter after it** is a hotfix of that release: crash, security or network fixes only, no features. Letters start at `a`, in order: 0.7.5, then 0.7.5a, 0.7.5b.
- **1.0.0** comes once the game is feature-complete and stable on every platform.

Test builds are the `nightly` release and `-testing` tags, outside the numbering. The network version is separate from the release version: a hotfix changes it only when cross-play needs it.

## Bugs

Use the bug report form, or #warthog-support on the
[Discord](https://discord.gg/chupathingy). Attach `D:\debug.txt`; for a
join that fails, run once with an empty `D:\trace.txt` beside
`default.xbe` (it shows other hosts only as tags, never their Ethernet
addresses).

## SDK and game files

Warthog never provides SDK or game files. Don't commit, attach or link
Microsoft's (or any console maker's) SDK headers, libraries or tools, maps,
disc images or other copyrighted game data.
