param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$SourceRoot,
    [Parameter(Mandatory = $true)][string[]]$Files
)

$ErrorActionPreference = 'Stop'
$paths = @(
    $SourceRoot.TrimEnd('\', '/').Replace('/', '\'),
    $SourceRoot.TrimEnd('\', '/').Replace('\', '/'),
    'C:\Users\',
    'C:/Users/'
)
foreach ($name in $Files) {
    $bytes = [IO.File]::ReadAllBytes((Join-Path $BuildDirectory $name))
    $singleByte = [Text.Encoding]::Latin1.GetString($bytes)
    $wide = [Text.Encoding]::Unicode.GetString($bytes)
    foreach ($path in $paths) {
        if ($singleByte.Contains($path) -or $wide.Contains($path)) {
            throw "Local source path found in package input: $name"
        }
    }
}
