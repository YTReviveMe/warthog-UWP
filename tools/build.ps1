param(
    [string]$UwpDeps = 'C:\bsuwp\_deps\uwpdep-src',
    [string]$Sdl3Root,
    [int]$Jobs = 12
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$source = $root
if (-not $Sdl3Root) {
    $Sdl3Root = Join-Path $source 'build\uwp-deps\sdl3-tested-3.4.10'
}
if (-not (Test-Path -LiteralPath (Join-Path $source 'configure.py'))) {
    throw 'Warthog source was not found.'
}
if (-not (Test-Path -LiteralPath $UwpDeps)) {
    throw "UWP dependency directory was not found: $UwpDeps"
}
if (-not (Test-Path -LiteralPath (Join-Path $Sdl3Root 'bin\SDL3.dll'))) {
    throw "SDL3-UWP was not found: $Sdl3Root. Run tools/setup_uwp.ps1 first."
}

Push-Location $source
$previousHaloVersion = $env:HALO_VERSION
$env:HALO_VERSION = (Get-Content -LiteralPath (Join-Path $source 'VERSION') -Raw).Trim()
try {
    & py -3 configure.py --release --pgo=off
    if ($LASTEXITCODE) { throw 'Guest configuration failed.' }
    & ninja uwp_guest
    if ($LASTEXITCODE) { throw 'Guest build failed.' }
} finally {
    if ($null -eq $previousHaloVersion) {
        Remove-Item Env:HALO_VERSION -ErrorAction SilentlyContinue
    } else {
        $env:HALO_VERSION = $previousHaloVersion
    }
    Pop-Location
}

$msvc = Join-Path $PSScriptRoot 'with-msvc.ps1'
& $msvc cmake --fresh -S (Join-Path $source 'port\xboxseries') `
    -B (Join-Path $source 'build\xbox-uwp') -G Ninja `
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl `
    -DCMAKE_CXX_COMPILER=clang-cl "-DUWP_DEPS=$UwpDeps" "-DUWP_SDL3_ROOT=$Sdl3Root"
if ($LASTEXITCODE) { throw 'Host configuration failed.' }
& $msvc cmake --build (Join-Path $source 'build\xbox-uwp') -j $Jobs
if ($LASTEXITCODE) { throw 'Host build failed.' }
