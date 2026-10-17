@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    echo Usage: drag an SVG file onto this script.
    pause
    exit /b 1
)

set "OUT_DIR=out"
set "STEM=%~n1"
set "SIZES=2048 1024 512 256 128 64 48 32 24 22 16"
set "LANCZOS_MIN=64"
REM Supersample factor: render each size at SS x target, then one gentle downscale.
REM   SS=1  = true native render (crispest edges, slight aliasing possible)
REM   SS=2  = render at 2x then downscale once (good balance) -- default
REM   SS=3+ = smoother / softer
set "SS=2"

REM --- Ensure ImageMagick is present (winget-install if missing) ---
set "MAGICK=magick"
where magick >nul 2>nul
if errorlevel 1 (
    where winget >nul 2>nul
    if errorlevel 1 (
        echo ImageMagick is not installed and winget was not found.
        echo Install winget ^(App Installer^) or ImageMagick manually, then re-run.
        pause
        exit /b 1
    )
    echo ImageMagick not found. Installing via winget...
    winget install --id ImageMagick.ImageMagick -e --accept-package-agreements --accept-source-agreements
    set "MAGICK="
    for /f "delims=" %%G in ('where magick 2^>nul') do set "MAGICK=%%G"
    if not defined MAGICK (
        for /f "delims=" %%G in ('dir /b /s "%ProgramFiles%\ImageMagick*\magick.exe" 2^>nul') do set "MAGICK=%%G"
    )
    if not defined MAGICK (
        echo ImageMagick was installed but could not be located in this session.
        echo Close this window and run the script again.
        pause
        exit /b 1
    )
)

if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%"
mkdir "%OUT_DIR%"

REM --- Read the SVG's intrinsic pixel size (at 96 dpi) so density math generalizes ---
set "IW=" & set "IH="
for /f "tokens=1,2" %%A in ('""!MAGICK!" identify -format "%%w %%h" "%~1""') do (
    set "IW=%%A" & set "IH=%%B"
)
if not defined IW (
    echo Could not read SVG dimensions. Is this a valid SVG?
    pause
    exit /b 1
)
set "MAXDIM=!IW!"
if !IH! GTR !IW! set "MAXDIM=!IH!"
echo SVG intrinsic size: !IW!x!IH!  ^(basis for per-size density^)

REM --- Render EACH size directly from the vector at SS x target, then snap to exact size ---
for %%S in (%SIZES%) do (
    set "FILT=Robidoux"
    if %%S GEQ %LANCZOS_MIN% set "FILT=Lanczos"
    REM density = ceil( 96 * S * SS / maxdim )  -> renders about (S*SS) px from the vector
    set /a "DENS=(96*%%S*%SS% + MAXDIM-1)/MAXDIM"
    "!MAGICK!" -background none -density !DENS! "%~1" ^
        -filter !FILT! -resize %%Sx%%S -background none -gravity center -extent %%Sx%%S ^
        -depth 8 "PNG32:%OUT_DIR%\%STEM%-%%S.png"
    echo   created %%Sx%%S ^(density !DENS!, !FILT!^)
)

echo Building %STEM%.ico...
"!MAGICK!" "%OUT_DIR%\%STEM%-16.png" "%OUT_DIR%\%STEM%-24.png" "%OUT_DIR%\%STEM%-32.png" "%OUT_DIR%\%STEM%-48.png" "%OUT_DIR%\%STEM%-64.png" "%OUT_DIR%\%STEM%-128.png" "%OUT_DIR%\%STEM%-256.png" "%OUT_DIR%\%STEM%.ico"

echo Building %STEM%.icns...
"!MAGICK!" "%OUT_DIR%\%STEM%-16.png" "%OUT_DIR%\%STEM%-32.png" "%OUT_DIR%\%STEM%-64.png" "%OUT_DIR%\%STEM%-128.png" "%OUT_DIR%\%STEM%-256.png" "%OUT_DIR%\%STEM%-512.png" "%OUT_DIR%\%STEM%-1024.png" "%OUT_DIR%\%STEM%.icns"

echo.
echo Done. Output in "%OUT_DIR%".
endlocal
pause