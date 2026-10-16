param([string]$Sdl3Root, [string]$SdlSource, [int]$Jobs = 12)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$revision = '8fd8db768df139e9bb4c7a7869fb913a0e29ea89'
$archiveHash = '64D6BC72BAE394522832B696B6BE9BEFDF429575E6768AD9244FF5EFF9E4F158'
$wasapiHashes = @{
    'src/audio/wasapi/SDL_wasapi.c' = '218453A44E3AB09F158E60AC0B291BDA4499FD5860CE0245AEBABCCEEEE39B2D'
    'src/audio/wasapi/SDL_wasapi_winrt.cpp' = '7BDC74EB21B0AD8C6CEFB9A458AD7328E9F0486E32F6DBECBF10CE1FA46779DD'
}
if (-not $Sdl3Root) { $Sdl3Root = Join-Path $root 'build\uwp-deps\sdl3-tested-3.4.10' }
if (-not $SdlSource) { $SdlSource = Join-Path $root 'build\uwp-deps\sdl3-src-3.4.10' }

if (-not (Test-Path -LiteralPath $SdlSource)) {
    & git -c core.autocrlf=false clone https://github.com/SternXD/SDL3-uwp.git $SdlSource
    if ($LASTEXITCODE) { throw 'SDL3 source clone failed.' }
    & git -C $SdlSource checkout --detach $revision
    if ($LASTEXITCODE) { throw 'SDL3 source checkout failed.' }
}
if ((& git -C $SdlSource rev-parse HEAD) -ne $revision) {
    throw "SDL3 source must be at pinned revision $revision."
}
$patch = Join-Path $root 'port\xboxseries\sdl3\wasapi-winrt.patch'
$alreadyPatched = $true
foreach ($file in $wasapiHashes.Keys) {
    if ((Get-FileHash -LiteralPath (Join-Path $SdlSource $file) -Algorithm SHA256).Hash -ne $wasapiHashes[$file]) {
        $alreadyPatched = $false
    }
}
if (-not $alreadyPatched) {
    & git -C $SdlSource apply --check $patch
    if ($LASTEXITCODE) { throw 'SDL3 source does not match the pinned WASAPI patch.' }
    & git -C $SdlSource apply $patch
    if ($LASTEXITCODE) { throw 'SDL3 WASAPI patch failed.' }
}
foreach ($file in $wasapiHashes.Keys) {
    $actual = (Get-FileHash -LiteralPath (Join-Path $SdlSource $file) -Algorithm SHA256).Hash
    if ($actual -ne $wasapiHashes[$file]) { throw "SDL3 source differs from the tested patch: $file" }
}
$changed = @(& git -C $SdlSource status --porcelain | ForEach-Object { $_.Substring(3).Replace('\', '/') } | Sort-Object)
$expected = @($wasapiHashes.Keys | Sort-Object)
if (($changed -join '|') -ne ($expected -join '|')) { throw 'SDL3 source has unrelated changes.' }

$msvc = Join-Path $PSScriptRoot 'with-msvc.ps1'
$project = Join-Path $SdlSource 'VisualC-WinRT\SDL-UWP.vcxproj'
$previousCl = $env:CL
$previousLink = $env:LINK
try {
    $env:CL = "$previousCl /d1trimfile:`"$SdlSource`""
    $env:LINK = "$previousLink /PDBALTPATH:%_PDB%"
    & $msvc MSBuild.exe $project /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143 /m:$Jobs /v:minimal
    if ($LASTEXITCODE) { throw 'SDL3 build failed.' }
} finally {
    $env:CL = $previousCl
    $env:LINK = $previousLink
}
$built = Join-Path $SdlSource 'VisualC-WinRT\x64\Release\SDL-UWP'
& $msvc powershell -NoProfile -ExecutionPolicy Bypass -File `
    (Join-Path $PSScriptRoot 'test_uwp_runtime_dependencies.ps1') -Path (Join-Path $built 'SDL3.dll')
if ($LASTEXITCODE) { throw 'SDL3 runtime dependency check failed.' }

$deps = Join-Path $root 'build\uwp-deps'
$archive = Join-Path $deps 'SDL3-UWP-3.4.10-release.zip'
if (-not (Test-Path -LiteralPath $archive)) {
    New-Item -ItemType Directory -Path $deps -Force | Out-Null
    Invoke-WebRequest 'https://github.com/XboxEmulationHub/SDL3-uwp/releases/download/release-uwp-3.4.10/SDL3-UWP.zip' -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $archiveHash) {
    throw 'SDL3 release archive checksum does not match.'
}
$unpacked = Join-Path $deps 'sdl3-release-unpacked-3.4.10'
Expand-Archive -LiteralPath $archive -DestinationPath $unpacked -Force
$release = Join-Path $unpacked 'winrt'
foreach ($path in @('include\SDL3\SDL.h', 'cmake\SDL3Config.cmake', 'licenses\SDL3\LICENSE.txt')) {
    if (-not (Test-Path -LiteralPath (Join-Path $release $path))) { throw "SDL3 release is missing $path" }
}
New-Item -ItemType Directory -Path $Sdl3Root -Force | Out-Null
Get-ChildItem -LiteralPath $release | Copy-Item -Destination $Sdl3Root -Recurse -Force
Copy-Item -LiteralPath (Join-Path $built 'SDL3.dll') -Destination (Join-Path $Sdl3Root 'bin\SDL3.dll') -Force
Copy-Item -LiteralPath (Join-Path $built 'SDL3.lib') -Destination (Join-Path $Sdl3Root 'lib\SDL3.lib') -Force
Write-Output "SDL3-UWP 3.4.10: $Sdl3Root"

$imgui = Join-Path $root 'port\xboxseries\third_party\imgui'
if (-not (Test-Path -LiteralPath (Join-Path $imgui 'imgui.cpp'))) {
    New-Item -ItemType Directory -Path (Split-Path -Parent $imgui) -Force | Out-Null
    & git clone --depth 1 --branch v1.91.9b https://github.com/ocornut/imgui.git $imgui
    if ($LASTEXITCODE) { throw 'Dear ImGui clone failed.' }
}
