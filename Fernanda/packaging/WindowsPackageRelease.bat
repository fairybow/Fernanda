@echo off
setlocal

REM ============================================================================
REM Fernanda Release Packaging Script (Windows)
REM ============================================================================

set INSTALLER_NAME=FernandaInstaller

set QT_DIR=C:\Qt\6.10.1\msvc2022_64
set QT_WINDEPLOY=%QT_DIR%\bin\windeployqt6.exe
REM This is for windeployqt6 to bundle VC++ Redist itself:
set VCINSTALLDIR=C:\Program Files\Microsoft Visual Studio\18\Community\VC\
REM Must use 8.3 short path for Program Files (x86) because of parentheses
set ISS_COMPILER=C:\PROGRA~2\Inno Setup 6\ISCC.exe
set ISS=.\WindowsInstaller.iss
set RELEASE_DIR=..\..\x64\Release

set RELEASE_EXE=%RELEASE_DIR%\Fernanda.exe
set RELEASE_VERSION_TXT=%RELEASE_DIR%\Version.txt
set README=..\..\README.md
set LICENSE=..\..\LICENSE
set ADDITIONAL_TERMS=..\..\ADDITIONAL_TERMS

set TEMP_DIR=.\temp
set OUTPUT_DIR=.\output\Windows x64

set STEPS=6

REM ============================================================================

echo.
echo ========================================
echo Fernanda Inno Installer Build
echo ========================================
echo.

echo [1/%STEPS%] Validating...

if not exist "%QT_WINDEPLOY%" (
    echo ERROR: windeployqt not found at %QT_WINDEPLOY%
    goto :error
)

if not exist "%ISS_COMPILER%" (
    echo ERROR: ISS compiler not found at %ISS_COMPILER%
    goto :error
)

if not exist "%ISS%" (
    echo ERROR: ISS script not found at %ISS%
    goto :error
)

if not exist "%RELEASE_EXE%" (
    echo ERROR: Release build not found at %RELEASE_EXE%
    goto :error
)

if not exist "%README%" (
    echo ERROR: README.md not found at %README%
    goto :error
)

if not exist "%LICENSE%" (
    echo ERROR: LICENSE not found at %LICENSE%
    goto :error
)

if not exist "%ADDITIONAL_TERMS%" (
    echo ERROR: ADDITIONAL_TERMS terms not found at %ADDITIONAL_TERMS%
    goto :error
)

if not exist "%RELEASE_VERSION_TXT%" (
    echo ERROR: Version.txt not found at %RELEASE_VERSION_TXT%
    echo        Did the pre-build step run?
    goto :error
)

set /p APP_VERSION=<"%RELEASE_VERSION_TXT%"
for /f "tokens=1 delims=-" %%a in ("%APP_VERSION%") do set APP_VERSION_NUMERIC=%%a
echo            Version: %APP_VERSION%

echo [2/%STEPS%] Cleaning temp directory...
if exist "%TEMP_DIR%" rmdir /s /q "%TEMP_DIR%"
mkdir "%TEMP_DIR%"

echo [3/%STEPS%] Copying Fernanda.exe...
copy "%RELEASE_EXE%" "%TEMP_DIR%\" > nul

REM TODO: If/when we add image viewing, revise this!
echo [4/%STEPS%] Running windeployqt...
"%QT_WINDEPLOY%" ^
    --release ^
    --no-translations ^
    --no-system-d3d-compiler ^
    --no-opengl-sw ^
    --skip-plugin-types generic,networkinformation ^
    --exclude-plugins qgif,qjpeg ^
    "%TEMP_DIR%\Fernanda.exe"

REM final contents of "temp" folder should look like this:
REM temp/
REM |-- README.md/LICENSE/ADDITIONAL_TERMS
REM |-- Fernanda.lnk (shortcut to exe)
REM +-- data/
REM     |-- [Qt DLLs, etc.]
REM     +-- Fernanda.exe
REM Inno will unpack to {app} (Program Files/Fernanda/), add additional files ({app}\README.md, etc.), and create a shortcut, {app}\Fernanda.lnk (points to {app}\data\Fernanda.exe)

echo [5/%STEPS%] Building installer...
if exist "%OUTPUT_DIR%" rmdir /s /q "%OUTPUT_DIR%"
mkdir "%OUTPUT_DIR%"

"%ISS_COMPILER%" ^
    /DAppVersion=%APP_VERSION% ^
    /DAppVersionNumeric=%APP_VERSION_NUMERIC% ^
    /DInstallerName=%INSTALLER_NAME% ^
    /DReadmePath="%README%" ^
    /DLicensePath="%LICENSE%" ^
    /DAdditionalTermsPath="%ADDITIONAL_TERMS%" ^
    /DOutputDir="%OUTPUT_DIR%" ^
    "%ISS%"
if errorlevel 1 goto :error

copy "%RELEASE_VERSION_TXT%" "%OUTPUT_DIR%\" > nul

echo.
echo ========================================
echo Build complete!
echo ========================================
echo Output: %OUTPUT_DIR%\%INSTALLER_NAME%.exe
echo         %OUTPUT_DIR%\Version.txt
echo.
goto :end

:error
echo.
echo Build failed!
echo.

:end
pause
endlocal
