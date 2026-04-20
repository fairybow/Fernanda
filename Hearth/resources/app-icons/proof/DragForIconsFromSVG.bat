@echo off
setlocal

if "%~1"=="" (
    exit /b 1
)

set "STEM=%~n1"

magick -background none -density 1200 "%~1" -filter Lanczos -colorspace sRGB -depth 8 -define png:compression-level=0 -define png:compression-filter=0 ^
  ( +clone -resize 2048x2048 -write "../%STEM%-2048.png" +delete ) ^
  ( +clone -resize 1024x1024 -write "../%STEM%-1024.png" +delete ) ^
  ( +clone -resize 512x512 -write "../%STEM%-512.png" +delete ) ^
  ( +clone -resize 256x256 -write "../%STEM%-256.png" +delete ) ^
  ( +clone -resize 128x128 -write "../%STEM%-128.png" +delete ) ^
  ( +clone -resize 64x64 -write "../%STEM%-64.png" +delete ) ^
  ( +clone -resize 48x48 -write "../%STEM%-48.png" +delete ) ^
  ( +clone -resize 32x32 -write "../%STEM%-32.png" +delete ) ^
  ( +clone -resize 24x24 -write "../%STEM%-24.png" +delete ) ^
  ( +clone -resize 22x22 -write "../%STEM%-22.png" +delete ) ^
  -resize 16x16 "../%STEM%-16.png"

magick "../%STEM%-16.png" "../%STEM%-24.png" "../%STEM%-32.png" "../%STEM%-48.png" "../%STEM%-64.png" "../%STEM%-128.png" "../%STEM%-256.png" "../%STEM%.ico"

magick "../%STEM%-16.png" "../%STEM%-32.png" "../%STEM%-64.png" "../%STEM%-128.png" "../%STEM%-256.png" "../%STEM%-512.png" "../%STEM%-1024.png" "../%STEM%.icns"

endlocal
pause