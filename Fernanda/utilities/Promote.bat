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

:: ask VS to close

:promote

echo Promoting .
powershell.exe -ExecutionPolicy ByPass ^
"$shell = New-Object -ComObject 'Shell.Application'; $paths = Get-ChildItem -Path 'C:\Dev\Fernanda' -Force -Exclude '.git', '.github'; ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

echo Promoting . .
powershell.exe -ExecutionPolicy ByPass ^
"$shell = New-Object -ComObject 'Shell.Application'; $paths = (Get-Item -Path 'C:\Dev\Fernanda-dev\x64' -Force), (Get-Item -Path 'C:\Dev\Fernanda-dev\Fernanda\x64' -Force), (Get-Item -Path 'C:\Dev\Fernanda-dev\.vs' -Force); ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

:: clean up temps
powershell.exe -ExecutionPolicy ByPass ^
"$shell = New-Object -ComObject 'Shell.Application'; $paths = Get-ChildItem -Path 'C:\Dev\Fernanda-dev' -Recurse -Force -Include '*.*~'; ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

:: copy from -dev to main
echo Promoting . . .
powershell.exe -ExecutionPolicy ByPass ^
"Copy-Item -Path 'C:\Dev\Fernanda-dev\*' -Destination 'C:\Dev\Fernanda' -Exclude '.git' -Recurse"

echo Promoted!

if /i '%releasing%'=='n' goto end
if '%releasing%'=='' goto end

:build
echo Building Visual Studio release . . .
start /w devenv.exe "C:\Dev\Fernanda\Fernanda.sln" /build Release x64

echo Building NSIS installer . . .
start /w "C:\Program Files (x86)\NSIS\makensis.exe" "C:\Dev\Fernanda\Fernanda\utilities\Installer.nsi"

echo f|xcopy /v /y /f "C:\Dev\Fernanda\Fernanda\utilities\Fernanda-setup-Windows-x64.exe" "%OneDrive%\Desktop\Fernanda-setup-Windows-x64.exe"
del "C:\Dev\Fernanda\Fernanda\utilities\Fernanda-setup-Windows-x64.exe"

start https://github.com/fairybow/Fernanda

:end
pause
exit
