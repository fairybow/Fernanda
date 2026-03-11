@echo off
setlocal

set SRC_DIR=%~dp0..\src
set OUT_DIR=%~dp0..\..\..\FernandaSrc

if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%"
mkdir "%OUT_DIR%"

for /r "%SRC_DIR%" %%f in (*.h *.cpp) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

echo Copied to: %OUT_DIR%
pause
endlocal