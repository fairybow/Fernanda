@echo off

:ask_release

set releasing=
set /p releasing=Releasing? (Y/N):
if not '%releasing%'=='' set releasing=%releasing:~0,1%
if /i '%releasing%'=='y' goto promote
if /i '%releasing%'=='n' goto promote
if '%releasing%'=='' goto promote
echo "%releasing%" invalid
echo.
goto ask_release

:: close VS!

:promote

echo Promoting .
powershell.exe -ExecutionPolicy ByPass ^
"$shell = New-Object -ComObject 'Shell.Application'; $paths = Get-ChildItem -Path 'C:\Dev\fernanda' -Force -Exclude '.git'; ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

echo Promoting . .
powershell.exe -ExecutionPolicy ByPass ^
"$shell = New-Object -ComObject 'Shell.Application'; $paths = (Get-Item -Path 'C:\Dev\fernanda-dev\x64' -Force), (Get-Item -Path 'C:\Dev\fernanda-dev\fernanda\x64' -Force), (Get-Item -Path 'C:\Dev\fernanda-dev\.vs' -Force); ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

echo Promoting . . .
powershell.exe -ExecutionPolicy ByPass ^
"Copy-Item -Path 'C:\Dev\fernanda-dev\*' -Destination 'C:\Dev\fernanda' -Exclude '.git' -Recurse"

echo Promoted!

if /i '%releasing%'=='n' goto end
if '%releasing%'=='' goto end

:build
echo Building Visual Studio release . . .
start /w devenv.exe "C:\Dev\fernanda\fernanda.sln" /build Release x64

echo Building NSIS installer . . .
start /w "C:\Program Files (x86)\NSIS\makensis.exe" "C:\Dev\fernanda\fernanda\util\Installer.nsi"

echo f|xcopy /v /y /f "C:\Dev\fernanda\fernanda\util\Fernanda-setup-x64-Windows.exe" "%OneDrive%\Desktop\Fernanda-setup-x64-Windows.exe"
del "C:\Dev\fernanda\fernanda\util\Fernanda-setup-x64-Windows.exe"

start https://github.com/fairybow/fernanda

:end
pause
exit
