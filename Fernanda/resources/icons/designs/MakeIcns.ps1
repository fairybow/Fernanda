# Packs the PNGs that DragForIconsFromSVG.bat renders into a macOS .icns, which
# ImageMagick can't write. An .icns is a header (the bytes "icns" and the
# file's length) followed by one chunk per image: a four-character type, the
# chunk's length, and the image. macOS 10.7 and later read PNG images in these
# chunks, so each PNG goes in unchanged. Both lengths are big-endian and count
# their own 8-byte header

param(
    [Parameter(Mandatory)] [string] $Dir,
    [Parameter(Mandatory)] [string] $Stem
)

$ErrorActionPreference = 'Stop'

# Each chunk's type and the pixel size of the PNG it holds. An @2x type is a
# smaller icon at double resolution, so it takes the PNG twice its size
$chunks = @(
    @('icp4', 16),
    @('icp5', 32),
    @('ic11', 32),    # 16@2x
    @('ic12', 64),    # 32@2x
    @('ic07', 128),
    @('ic13', 256),   # 128@2x
    @('ic08', 256),
    @('ic14', 512),   # 256@2x
    @('ic09', 512),
    @('ic10', 1024)   # 512@2x
)

function Write-BigEndian([System.IO.Stream] $Stream, [uint32] $Value) {
    $bytes = [BitConverter]::GetBytes($Value)
    if ([BitConverter]::IsLittleEndian) {
        [Array]::Reverse($bytes)
    }
    $Stream.Write($bytes, 0, 4)
}

$Dir = (Resolve-Path $Dir).Path
$body = New-Object System.IO.MemoryStream

foreach ($chunk in $chunks) {
    $png = [IO.File]::ReadAllBytes((Join-Path $Dir "$Stem-$($chunk[1]).png"))
    $type = [Text.Encoding]::ASCII.GetBytes($chunk[0])
    $body.Write($type, 0, 4)
    Write-BigEndian $body ([uint32]($png.Length + 8))
    $body.Write($png, 0, $png.Length)
}

$file = [IO.File]::Create((Join-Path $Dir "$Stem.icns"))

try {
    $magic = [Text.Encoding]::ASCII.GetBytes('icns')
    $file.Write($magic, 0, 4)
    Write-BigEndian $file ([uint32]($body.Length + 8))
    $body.WriteTo($file)
} finally {
    $file.Dispose()
}

Write-Host "  created $Stem.icns ($($chunks.Count) images)"
