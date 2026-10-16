param([Parameter(Mandatory = $true)][string]$Path)

$ErrorActionPreference = 'Stop'
$item = Get-Item -LiteralPath $Path
$binaries = if ($item.PSIsContainer) {
    @(Get-ChildItem -LiteralPath $item.FullName -File | Where-Object Extension -in @('.exe', '.dll'))
} else { @($item) }
if (-not $binaries.Count) { throw "No binaries found at $Path" }
foreach ($binary in $binaries) {
    $imports = (& dumpbin /nologo /imports $binary.FullName) -join "`n"
    if ($LASTEXITCODE) { throw "Cannot inspect $($binary.Name)" }
    $dependencies = @([regex]::Matches($imports, '(?im)^\s+([a-z0-9_.-]+\.dll)\s*$') |
        ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique)
    if (-not $dependencies.Count) { throw "No DLL imports parsed for $($binary.Name)" }
    $runtimes = @($dependencies | Where-Object { $_ -match '^(msvcp|msvcr|vcruntime|vccorlib)\d' })
    foreach ($runtime in $runtimes) {
        if ($runtime -notmatch '_app\.dll$' -or $runtime -match 'd_app\.dll$') {
            throw "$($binary.Name) imports non-release-UWP runtime $runtime"
        }
    }
    if ($binary.Name -ieq 'SDL3.dll') {
        foreach ($required in @('vccorlib140_app.dll', 'msvcp140_app.dll', 'vcruntime140_app.dll')) {
            if ($dependencies -notcontains $required) { throw "SDL3.dll is missing expected UWP runtime $required" }
        }
        if ($imports -notmatch '\bLoadPackagedLibrary\b') { throw 'SDL3.dll is missing its packaged-library loader.' }
    }
    Write-Output "PASS: $($binary.Name) runtime imports ($($runtimes -join ', '))"
}
