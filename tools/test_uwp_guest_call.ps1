param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$output = Join-Path $root 'build\tests\guest-call'
New-Item -ItemType Directory -Path $output -Force | Out-Null
Push-Location $output
try {
    & ml64 /nologo /c /Foguest_call.obj (Join-Path $root 'port\xboxseries\host\guest_call.asm')
    if ($LASTEXITCODE) { throw 'Guest bridge assembly failed.' }
    & ml64 /nologo /c /Foprobe.obj (Join-Path $root 'port\xboxseries\tests\guest_call_probe.asm')
    if ($LASTEXITCODE) { throw 'Probe assembly failed.' }
    & cl /nologo /O2 /W4 (Join-Path $root 'port\xboxseries\tests\guest_call_test.c') guest_call.obj probe.obj /Fe:guest_call_test.exe
    if ($LASTEXITCODE) { throw 'Test compilation failed.' }
    & .\guest_call_test.exe
    if ($LASTEXITCODE) { throw 'Guest calling-convention test failed.' }
} finally {
    Pop-Location
}
