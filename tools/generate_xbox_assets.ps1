param([Parameter(Mandatory = $true)][string]$OutputDirectory)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$root = Split-Path -Parent $PSScriptRoot
$sourcePath = Join-Path $root 'port\xboxseries\assets\AppLogo.png'
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$sourceImage = [Drawing.Image]::FromFile($sourcePath)
$source = [Drawing.Bitmap]::new($sourceImage)

try {
    function Get-ContentBounds([Drawing.Bitmap]$bitmap) {
        $left = $bitmap.Width
        $top = $bitmap.Height
        $right = -1
        $bottom = -1

        for ($y = 0; $y -lt $bitmap.Height; $y++) {
            for ($x = 0; $x -lt $bitmap.Width; $x++) {
                if (-not $bitmap.GetPixel($x, $y).A) {
                    continue
                }
                if ($x -lt $left) { $left = $x }
                if ($x -gt $right) { $right = $x }
                if ($y -lt $top) { $top = $y }
                if ($y -gt $bottom) { $bottom = $y }
            }
        }

        if ($right -lt $left) {
            return [Drawing.Rectangle]::new(0, 0, $bitmap.Width, $bitmap.Height)
        }
        return [Drawing.Rectangle]::new($left, $top, $right - $left + 1, $bottom - $top + 1)
    }

    function Write-SquareAsset([string]$name, [int]$size) {
        $image = [Drawing.Bitmap]::new($size, $size, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        $graphics = [Drawing.Graphics]::FromImage($image)
        try {
            $graphics.Clear([Drawing.Color]::Transparent)
            $graphics.InterpolationMode = 'HighQualityBicubic'
            $graphics.DrawImage(
                $source,
                [Drawing.Rectangle]::new(0, 0, $size, $size),
                $sourceBounds,
                [Drawing.GraphicsUnit]::Pixel
            )
            $image.Save((Join-Path $OutputDirectory $name), 'Png')
        }
        finally {
            $graphics.Dispose()
            $image.Dispose()
        }
    }

    $sourceBounds = Get-ContentBounds $source
    Write-SquareAsset 'StoreLogo.png' 50
    Write-SquareAsset 'Square44x44Logo.png' 44
    Write-SquareAsset 'Square150x150Logo.png' 150

    $splash = [Drawing.Bitmap]::new(620, 300)
    $graphics = [Drawing.Graphics]::FromImage($splash)
    try {
        $graphics.Clear([Drawing.Color]::FromArgb(255, 16, 24, 32))
        $graphics.InterpolationMode = 'HighQualityBicubic'
        $graphics.DrawImage($source, 170, 10, 280, 280)
        $splash.Save((Join-Path $OutputDirectory 'SplashScreen.png'), 'Png')
    }
    finally {
        $graphics.Dispose()
        $splash.Dispose()
    }
}
finally {
    $source.Dispose()
    $sourceImage.Dispose()
}
