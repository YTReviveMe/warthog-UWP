param(
    [Parameter(Mandatory = $true)][string]$PackagePath,
    [string]$CertificatePath
)

$ErrorActionPreference = 'Stop'
$package = Get-Item -LiteralPath $PackagePath
if (-not $CertificatePath) {
    $CertificatePath = Join-Path $package.DirectoryName 'Warthog-UWP.cer'
}

$machineStore = $true
$store = 'TrustedPeople'
$certificate = Get-ChildItem Cert:\LocalMachine\TrustedPeople |
    Where-Object {
        $_.Subject -eq 'CN=ReviveMe' -and
        $_.HasPrivateKey -and
        $_.NotAfter -gt (Get-Date).AddDays(30)
    } |
    Sort-Object NotAfter -Descending |
    Select-Object -First 1

if (-not $certificate) {
    $machineStore = $false
    $store = 'My'
    $certificate = Get-ChildItem Cert:\CurrentUser\My |
        Where-Object {
            $_.Subject -eq 'CN=ReviveMe' -and
            $_.HasPrivateKey -and
            $_.NotAfter -gt (Get-Date).AddDays(30)
        } |
        Sort-Object NotAfter -Descending |
        Select-Object -First 1
}

if (-not $certificate) {
    $certificate = New-SelfSignedCertificate -Type CodeSigningCert `
        -Subject 'CN=ReviveMe' -FriendlyName 'Warthog UWP Dev Mode' `
        -CertStoreLocation Cert:\CurrentUser\My -KeyExportPolicy Exportable `
        -HashAlgorithm SHA256 -NotAfter (Get-Date).AddYears(2)
}

$signTool = Get-ChildItem -LiteralPath (Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin') `
    -Recurse -Filter SignTool.exe |
    Where-Object FullName -match '\\x64\\SignTool.exe$' |
    Sort-Object FullName -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $signTool) {
    throw 'SignTool.exe not found.'
}

$arguments = @('sign', '/fd', 'SHA256', '/sha1', $certificate.Thumbprint, '/s', $store, '/v', $package.FullName)
if ($machineStore) {
    $arguments = @(
        'sign', '/fd', 'SHA256', '/sha1', $certificate.Thumbprint,
        '/s', $store, '/sm', '/v', $package.FullName
    )
}

& $signTool @arguments
if ($LASTEXITCODE) {
    throw "SignTool failed: $LASTEXITCODE"
}

Export-Certificate -Cert $certificate -FilePath $CertificatePath -Force | Out-Null
Write-Output "Signed $($package.FullName)"
Write-Output "Certificate $CertificatePath"
