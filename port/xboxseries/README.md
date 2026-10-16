# Warthog UWP for Xbox Developer Mode

Xbox Developer Mode port of the current Warthog tree. The original-Xbox
`ninja xbox` build remains independent; this port uses `ninja uwp_guest`.

Install Visual Studio 2022 with C++ UWP tools, Windows SDK 10.0.26100, Python, Ninja, CMake, Clang and LLD. The setup script builds pinned SDL3-UWP sources and fetches Dear ImGui 1.91.9b; provide x64 UWP builds of libuwp, Mesa and zlib.

```powershell
./tools/setup_uwp.ps1
./tools/build.ps1 -UwpDeps C:/path/to/uwp-deps
./tools/make_release.ps1 -OutputDirectory C:/path/to/packages
```

The Xbox-tested build uses SternXD SDL3-UWP 3.4.10 at commit `8fd8db768df139e9bb4c7a7869fb913a0e29ea89` with the patch in `port/xboxseries/sdl3/wasapi-winrt.patch`. `tools/setup_uwp.ps1` builds that revision and installs it to `build/uwp-deps/sdl3-tested-3.4.10`; `tools/build.ps1` selects that install by default. The setup script checks the source revision, patched file hashes and release archive checksum. Use `-Sdl3Root` to select another install explicitly. The guest uses SDL3 3.4.16 headers.

Install the APPX through Xbox Device Portal. Include the x64 VCLibs dependency if requested. The app installs as Warthog alongside OpenCE and ChupathingyCE.

SDL3 must link against the Visual C++ `lib/x64/store` import libraries, not the desktop libraries inherited from a developer shell. Setup and packaging run `tools/test_uwp_runtime_dependencies.ps1` to reject desktop/debug C++ runtime imports; packaging checks every top-level EXE and DLL. This is a build-time check, not a substitute for Xbox runtime testing.

Select internal or external storage on first launch and browse to a legally owned Halo Xbox ISO or XISO. Extraction is performed once and the installation is remembered.

Internal installations use `LocalState/Warthog`. External installations use the drive's `Warthog` folder. Saves, settings, map caches and logs stay with the selected installation. The shader cache uses the app's local cache storage.

Xbox maps belong in `Warthog\maps`. Put Custom Edition `.map` files and their `bitmaps.map`, `sounds.map` and `loc.map` resource files in `Warthog\custom_maps`.

No retail game data is included. Upstream licensing and third-party notices remain applicable.
