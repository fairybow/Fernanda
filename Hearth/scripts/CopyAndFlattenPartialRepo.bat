@echo off
setlocal

set SRC_DIR=%~dp0..\src
set COCO_INCLUDE_DIR=%~dp0..\submodules\Coco\Coco\include\Coco
set COCO_SRC_DIR=%~dp0..\submodules\Coco\Coco\src
set FOUNTAIN_H_INCLUDE_DIR=%~dp0..\submodules\fountain.h\fountain.h\include
set FOUNTAIN_H_SRC_DIR=%~dp0..\submodules\fountain.h\fountain.h\src
set DOCS_DIR=%~dp0..\docs
set PROJ_DIR=%~dp0..
set ROOT_DIR=%~dp0..\..
set OUT_DIR=%~dp0..\..\..\HearthSrc

if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%"
mkdir "%OUT_DIR%"

for /r "%COCO_INCLUDE_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

for /r "%COCO_SRC_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

for /r "%FOUNTAIN_H_INCLUDE_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

for /r "%FOUNTAIN_H_SRC_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

for /r "%SRC_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

for /r "%DOCS_DIR%" %%f in (*) do (
    copy "%%f" "%OUT_DIR%\" > nul
)

copy "%ROOT_DIR%\.github\workflows\release.yml" "%OUT_DIR%\" > nul
copy "%ROOT_DIR%\CHANGELOG.md" "%OUT_DIR%\" > nul
copy "%ROOT_DIR%\CONTRIBUTING.md" "%OUT_DIR%\" > nul
copy "%ROOT_DIR%\README.md" "%OUT_DIR%\" > nul
copy "%PROJ_DIR%\.clang-format" "%OUT_DIR%\" > nul
copy "%PROJ_DIR%\CMakeLists.txt" "%OUT_DIR%\" > nul

echo Copied to: %OUT_DIR%
pause
endlocal