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

& (Join-Path $PSScriptRoot 'with-msvc.ps1') powershell -NoProfile -ExecutionPolicy Bypass -File `
    (Join-Path $PSScriptRoot 'test_uwp_runtime_dependencies.ps1') -Path $BuildDirectory
if ($LASTEXITCODE) { throw 'UWP runtime dependency validation failed.' }

$assets = Join-Path $BuildDirectory 'xbox-assets'
& powershell -NoProfile -ExecutionPolicy Bypass -File `
    (Join-Path $sourceRoot 'tools\generate_xbox_assets.ps1') -OutputDirectory $assets

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
    Copy-Item -LiteralPath (Join-Path $sourceRoot 'port\assets\fonts\Newtown-LICENSE.txt') `
        -Destination (Join-Path $stage 'Newtown-LICENSE.txt')
    Copy-Item -LiteralPath (Join-Path $sourceRoot 'port\third_party\extract-xiso\LICENSE.TXT') `
        -Destination (Join-Path $stage 'extract-xiso-LICENSE.txt')
    Copy-Item -LiteralPath (Join-Path $sourceRoot 'port\xboxseries\third_party\imgui\LICENSE.txt') `
        -Destination (Join-Path $stage 'imgui-LICENSE.txt')
    $sdlLicense = Join-Path $BuildDirectory 'sdl3-LICENSE.txt'
    if (Test-Path -LiteralPath $sdlLicense) {
        Copy-Item -LiteralPath $sdlLicense -Destination $stage
    }

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
