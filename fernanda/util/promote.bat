@echo off

:ask_release

set opt_1=
set /p opt_1=Releasing? (Y/N):
if not '%opt_1%'=='' set opt_1=%opt_1:~0,1%
if /i '%opt_1%'=='y' goto promote
if /i '%opt_1%'=='n' goto promote
if '%opt_1%'=='' goto promote
echo "%opt_1%" invalid
echo.
goto ask_release

:promote

echo Promoting .
powershell.exe -ExecutionPolicy ByPass "$shell = New-Object -ComObject 'Shell.Application'; $paths = Get-ChildItem -Path 'C:\Dev\fernanda' -Force -Exclude '.git'; ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

echo Promoting . .
powershell.exe -ExecutionPolicy ByPass "$shell = New-Object -ComObject 'Shell.Application'; $paths = (Get-Item -Path 'C:\Dev\fernanda-dev\x64' -Force), (Get-Item -Path 'C:\Dev\fernanda-dev\fernanda\x64' -Force), (Get-Item -Path 'C:\Dev\fernanda-dev\.vs' -Force); ForEach ($path in $paths) { $shell.NameSpace(0).ParseName($path.FullName).InvokeVerb('delete') }"

echo Promoting . . .
powershell.exe -ExecutionPolicy ByPass "Copy-Item -Path 'C:\Dev\fernanda-dev\*' -Destination 'C:\Dev\fernanda' -Exclude '.git' -Recurse"

echo Promoted!

if /i '%opt_1%'=='n' goto end
if '%opt_1%'=='' goto end

:release
echo Building release
start devenv.exe "C:\Dev\fernanda\fernanda.sln" /build Release x64

:: wait, then start NSIS and build installer

:end
pause
exit
