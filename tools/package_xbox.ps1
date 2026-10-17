param(
    [string]$BuildDirectory,
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path -Parent $PSScriptRoot
if (-not $BuildDirectory) {
    $BuildDirectory = Join-Path $sourceRoot 'build\xbox-uwp'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $BuildDirectory 'Warthog-Xbox-x64.appx'
}

$required = @(
    'Warthog.exe',
    'OpenCEUWP.winmd',
    'SDL3.dll',
    'libuwp.dll',
    'opengl32.dll',
    'libgallium_wgl.dll',
    'dxil.dll',
    'z-1.dll',
    'halo_guest.elf'
)
foreach ($name in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $BuildDirectory $name))) {
        throw "Missing build output: $name"
    }
}
& (Join-Path $PSScriptRoot 'test_uwp_source_paths.ps1') `
    -BuildDirectory $BuildDirectory -SourceRoot $sourceRoot -Files $required

& (Join-Path $PSScriptRoot 'with-msvc.ps1') powershell -NoProfile -ExecutionPolicy Bypass -File `
    (Join-Path $PSScriptRoot 'test_uwp_runtime_dependencies.ps1') -Path $BuildDirectory
if ($LASTEXITCODE) { throw 'UWP runtime dependency validation failed.' }

$assets = Join-Path $BuildDirectory 'xbox-assets'
& powershell -NoProfile -ExecutionPolicy Bypass -File `
    (Join-Path $sourceRoot 'tools\generate_xbox_assets.ps1') -OutputDirectory $assets

$cache = Join-Path $BuildDirectory 'CMakeCache.txt'
$dependencyEntry = Select-String -LiteralPath $cache -Pattern '^UWP_DEPS:PATH=(.+)$' | Select-Object -First 1
if (-not $dependencyEntry) {
    throw 'UWP dependency path was not found in CMakeCache.txt.'
}
$dependencyLicenses = Join-Path $dependencyEntry.Matches[0].Groups[1].Value 'lic'
$dependencyNotices = @(
    'DXIL-LICENSE.txt', 'LIBUWP-LICENSE.txt', 'MESA-LICENSE.rst', 'ZLIB-LICENSE.txt'
)
$sourceNotices = [ordered]@{
    'LICENSE' = 'LICENSE.txt'
    'README.md' = 'Warthog-README.md'
    'LICENSES\CC0-1.0.txt' = 'CC0-1.0.txt'
    'port\assets\fonts\Newtown-LICENSE.txt' = 'Newtown-LICENSE.txt'
    'port\assets\fonts\Overpass-OFL.txt' = 'Overpass-OFL.txt'
    'port\linux\ui\fonts\OFL.txt' = 'NotoSans-OFL.txt'
    'port\linux\ui\fonts\KENNEY-CC0.txt' = 'Kenney-Input-Prompts-CC0.txt'
    'port\third_party\expat\COPYING' = 'expat-COPYING.txt'
    'port\third_party\extract-xiso\LICENSE.TXT' = 'extract-xiso-LICENSE.txt'
    'port\third_party\kcp\LICENSE' = 'kcp-LICENSE.txt'
    'port\third_party\mbedtls\LICENSE' = 'mbedtls-LICENSE.txt'
    'port\third_party\miniupnpc\LICENSE' = 'miniupnpc-LICENSE.txt'
    'port\third_party\monocypher\LICENCE.md' = 'monocypher-LICENCE.md'
    'port\third_party\qrcodegen\LICENSE' = 'qrcodegen-LICENSE.txt'
    'port\third_party\smaa\LICENSE' = 'smaa-LICENSE.txt'
    'port\third_party\stb\LICENSE' = 'stb-LICENSE.txt'
    'port\third_party\tomlc17\LICENSE' = 'tomlc17-LICENSE.txt'
    'port\xboxseries\third_party\imgui\LICENSE.txt' = 'imgui-LICENSE.txt'
}
foreach ($notice in $sourceNotices.Keys) {
    if (-not (Test-Path -LiteralPath (Join-Path $sourceRoot $notice))) {
        throw "Missing source notice: $notice"
    }
}
foreach ($notice in $dependencyNotices) {
    if (-not (Test-Path -LiteralPath (Join-Path $dependencyLicenses $notice))) {
        throw "Missing dependency notice: $notice"
    }
}
$sdlLicense = Join-Path $BuildDirectory 'sdl3-LICENSE.txt'
if (-not (Test-Path -LiteralPath $sdlLicense)) {
    throw 'Missing SDL3 license.'
}

$makeAppx = Get-ChildItem -LiteralPath (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin') `
    -Recurse -Filter MakeAppx.exe |
    Where-Object FullName -match '\\x64\\MakeAppx.exe$' |
    Sort-Object FullName -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $makeAppx) {
    throw 'MakeAppx.exe not found.'
}

$stage = Join-Path $BuildDirectory ('.stage-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $stage | Out-Null
    foreach ($name in $required) {
        Copy-Item -LiteralPath (Join-Path $BuildDirectory $name) -Destination $stage
    }

    Copy-Item -LiteralPath (Join-Path $sourceRoot 'port\xboxseries\Package.appxmanifest') `
        -Destination (Join-Path $stage 'AppxManifest.xml')
    New-Item -ItemType Directory -Path (Join-Path $stage 'Assets') | Out-Null
    Copy-Item -Path (Join-Path $assets '*.png') -Destination (Join-Path $stage 'Assets')
    $networkAssets = Join-Path $stage 'Warthog\network'
    New-Item -ItemType Directory -Path $networkAssets | Out-Null
    Copy-Item -LiteralPath (Join-Path $sourceRoot 'port\assets\network\brokers.txt') -Destination $networkAssets
    foreach ($notice in $sourceNotices.Keys) {
        Copy-Item -LiteralPath (Join-Path $sourceRoot $notice) `
            -Destination (Join-Path $stage $sourceNotices[$notice])
    }
    foreach ($notice in $dependencyNotices) {
        Copy-Item -LiteralPath (Join-Path $dependencyLicenses $notice) -Destination $stage
    }
    Copy-Item -LiteralPath $sdlLicense -Destination $stage

    if (Test-Path -LiteralPath $OutputPath) {
        Remove-Item -LiteralPath $OutputPath
    }
    & $makeAppx pack /d $stage /p $OutputPath /o
    if ($LASTEXITCODE) {
        throw "MakeAppx failed: $LASTEXITCODE"
    }
}
finally {
    if (Test-Path -LiteralPath $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
}

Write-Output "Created $OutputPath"
