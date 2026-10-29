# Renders the MSIX package's tile, Store logo, and scaled app icon from an SVG
# into .github/msix/Assets, where BuildMsix.ps1 takes them from. The package's
# fixed-size app icons aren't made here: they are the PNGs in resources/icons,
# which BuildMsix.ps1 copies in.
#
# Renders as DragForIconsFromSVG.bat does: each image from the vector at twice
# its size, then one downscale. Needs ImageMagick (DragForIconsFromSVG.bat
# installs it)

param(
    [string] $Svg = (Join-Path $PSScriptRoot 'Fernanda.svg')
)

$ErrorActionPreference = 'Stop'

# Each image's file name, its canvas, and the size of the icon centered on it.
# A tile keeps a margin around the icon, as Windows' own tiles do
$images = @(
    @('Square44x44Logo.scale-100.png', 44, 44),
    @('Square44x44Logo.scale-200.png', 88, 88),
    @('Square150x150Logo.scale-100.png', 150, 100),
    @('Square150x150Logo.scale-200.png', 300, 200),
    @('StoreLogo.scale-100.png', 50, 50),
    @('StoreLogo.scale-200.png', 100, 100)
)

$supersample = 2
$lanczosMin = 64

$magick = Get-Command 'magick' -ErrorAction SilentlyContinue
if (-not $magick) {
    throw 'ImageMagick not found. Install it, or run DragForIconsFromSVG.bat once, which installs it'
}
$magick = $magick.Source

$Svg = (Resolve-Path $Svg).Path
$outDir = Join-Path $PSScriptRoot '..\..\..\..\.github\msix\Assets'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$outDir = (Resolve-Path $outDir).Path

# The SVG's size at 96 dpi, the basis for each image's render density
$dims = (& $magick identify -format '%w %h' $Svg) -split ' '
$maxDim = [Math]::Max([int]$dims[0], [int]$dims[1])

foreach ($image in $images) {
    $name = $image[0]
    $canvas = $image[1]
    $icon = $image[2]

    $filter = if ($icon -ge $lanczosMin) { 'Lanczos' } else { 'Robidoux' }
    $density = [Math]::Ceiling(96 * $icon * $supersample / $maxDim)

    & $magick -background none -density $density $Svg `
        -filter $filter -resize "${icon}x${icon}" `
        -background none -gravity center -extent "${canvas}x${canvas}" `
        -depth 8 "PNG32:$(Join-Path $outDir $name)"
    if ($LASTEXITCODE -ne 0) {
        throw "ImageMagick exited with $LASTEXITCODE on $name"
    }

    Write-Host "  created $name (${icon}px icon on ${canvas}px, density $density, $filter)"
}
