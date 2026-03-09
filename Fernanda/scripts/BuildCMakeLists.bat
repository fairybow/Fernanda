@echo off
pushd "%~dp0.."
powershell -ExecutionPolicy Bypass -File "%~dp0BuildCMakeLists.ps1"
popd
pause
