# Warthog UWP

Warthog UWP brings the [Warthog](https://github.com/ChupathingyCE/warthog) and
[ChupathingyCE](https://github.com/ChupathingyCE/chupathingyce) game code to
Xbox Series consoles in Developer Mode. This repository includes the
Xbox Series UWP port alongside the shared game source. The game version is
in [VERSION](VERSION); the UWP package version is in
[Package.appxmanifest](port/xboxseries/Package.appxmanifest).

The port uses SDL3, a 64-bit UWP host and a 32-bit game guest. It supports
campaign, online multiplayer, profile linking, Custom Edition maps, and
first-boot extraction from a legally owned Halo Xbox ISO or XISO. Game files
are not included.

## Install and storage

Install the APPX through Xbox Device Portal and include the x64 VCLibs
dependency if requested. On first launch, choose internal or external
storage and select an ISO or XISO to extract.

- Internal game data: `D:\DevelopmentFiles\Warthog`
- External game data: `Warthog` on the selected removable drive
- Xbox maps: `Warthog\maps`
- Custom Edition maps and their `bitmaps.map`, `sounds.map` and
  `loc.map` resources: `Warthog\maps_ce`

Multiplayer map downloads also use `maps_ce`. Existing maps in
`custom_maps` remain readable. Saves, settings, caches and logs stay with
the selected installation.

For build prerequisites, pinned dependencies and packaging commands, see
[the UWP port README](port/xboxseries/README.md). The original Xbox and
other console ports are separate projects maintained upstream.

## License and credits

Warthog and ChupathingyCE source is licensed under GPL-3.0-or-later; see
[LICENSE](LICENSE). Earlier CC0 code and third-party components retain their
respective licenses and notices. No Microsoft game files or console SDK
libraries are included. Halo is a Microsoft trademark; this is a fan
project, not endorsed by Microsoft, Bungie or 343 Industries.

The Halo CE decompilation originated with
[punpckhdq/halo](https://github.com/punpckhdq/halo) and
[bnunu/halo-1](https://github.com/bnunu/halo-1). This port builds on
[ChupathingyCE](https://github.com/ChupathingyCE/chupathingyce),
[Warthog](https://github.com/ChupathingyCE/warthog), OpenCE and their
contributors.

### Console SDK permission

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

This permission does not grant rights in Microsoft's libraries. Their
terms still apply.
