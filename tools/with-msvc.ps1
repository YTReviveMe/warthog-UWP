param(
    [Parameter(Mandatory = $true, Position = 0)][string]$Program,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$Arguments
)

$ErrorActionPreference = 'Stop'
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $installation) { throw 'Visual Studio C++ tools were not found.' }

# Re-entering repeatedly in one build script grows PATH/LIB until cmd.exe's
# command-line limit is exceeded. Reuse an initialized x64 environment.
if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VSCMD_ARG_HOST_ARCH -ne 'x64' -or
    -not (Test-Path -LiteralPath "$env:VCToolsInstallDir/bin/Hostx64/x64/link.exe")) {
    Import-Module (Join-Path $installation 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation `
        -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}
& $Program @Arguments
exit $LASTEXITCODE
