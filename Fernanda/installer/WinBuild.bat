@echo off
setlocal EnableDelayedExpansion

REM ============================================================================
REM Fernanda Installer Build Script (Windows)
REM ============================================================================
REM Run this from the installer directory: Fernanda\installer\
REM ============================================================================

REM --- Configuration (adjust these paths for your system) ---
set QT_DIR=C:\Qt\6.10.1\msvc2022_64
set QT_IFW_DIR=C:\Qt\Tools\QtInstallerFramework\4.10
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\
REM set VC_REDIST=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Redist\MSVC\14.50.35710\vc_redist.x64.exe

REM --- Derived paths ---
set WINDEPLOYQT=%QT_DIR%\bin\windeployqt6.exe
set BINARYCREATOR=%QT_IFW_DIR%\bin\binarycreator.exe

REM --- Project paths (relative to installer directory) ---
set SOURCE_EXE=..\..\x64\Release\Fernanda.exe
set DATA_DIR=packages\com.fairybow.fernanda\data
set CONFIG_DIR=config
set PACKAGES_DIR=packages
set OUTPUT_DIR=output

REM ============================================================================
REM Validation
REM ============================================================================

echo.
echo ========================================
echo Fernanda Installer Build
echo ========================================
echo.

if not exist "%SOURCE_EXE%" (
    echo ERROR: Release build not found at %SOURCE_EXE%
    echo Please build Fernanda in Release configuration first.
    goto :error
)

if not exist "%WINDEPLOYQT%" (
    echo ERROR: windeployqt not found at %WINDEPLOYQT%
    echo Please check QT_DIR setting.
    goto :error
)

if not exist "%BINARYCREATOR%" (
    echo ERROR: binarycreator not found at %BINARYCREATOR%
    echo Please check QT_IFW_DIR setting.
    goto :error
)

REM ============================================================================
REM Prepare data directory
REM ============================================================================

echo [1/4] Cleaning data directory...
if exist "%DATA_DIR%" rmdir /s /q "%DATA_DIR%"
mkdir "%DATA_DIR%"

REM ============================================================================
REM Copy executable
REM ============================================================================

echo [2/4] Copying Fernanda.exe...
copy "%SOURCE_EXE%" "%DATA_DIR%\" > nul
if errorlevel 1 (
    echo ERROR: Failed to copy executable.
    goto :error
)

REM ============================================================================
REM Run windeployqt
REM ============================================================================

echo [3/4] Running windeployqt...
"%WINDEPLOYQT%" ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    "%DATA_DIR%\Fernanda.exe"

if errorlevel 1 (
    echo ERROR: windeployqt failed.
    goto :error
)

REM ============================================================================
REM Copy VC++ Redistributable
REM ============================================================================

REM echo [4/5] Copying VC++ Redistributable...
REM if exist "%VC_REDIST%" (
REM     copy "%VC_REDIST%" "%DATA_DIR%\" > nul
REM     echo        - vc_redist.x64.exe
REM ) else (
REM     echo WARNING: VC++ Redistributable not found at %VC_REDIST%
REM     echo          Users may need to install it separately.
REM )

REM ============================================================================
REM Build installer
REM ============================================================================

echo [4/4] Building installer...

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

"%BINARYCREATOR%" ^
    --offline-only ^
    -c "%CONFIG_DIR%\config.xml" ^
    -p "%PACKAGES_DIR%" ^
    "%OUTPUT_DIR%\FernandaInstaller.exe"

if errorlevel 1 (
    echo ERROR: binarycreator failed.
    goto :error
)

REM ============================================================================
REM Done
REM ============================================================================

echo.
echo ========================================
echo Build complete!
echo ========================================
echo Output: %OUTPUT_DIR%\FernandaInstaller.exe
echo.

goto :end

:error
echo.
echo Build failed!
echo.

:end
pause
endlocal
