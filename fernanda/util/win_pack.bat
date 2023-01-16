@echo off

:start
set fer_repo=
set fer_dir=
set fer_ver=

:dev
set opt_1=
set /p opt_1=build from dev repo? (y/n):
if not '%opt_1%'=='' set opt_1=%opt_1:~0,1%
if /i '%opt_1%'=='y' goto yes
if /i '%opt_1%'=='n' goto no
if '%opt_1%'=='' goto no
echo "%opt_1%" invalid
echo.
goto dev

:no
set fer_repo="fernanda"
set /p opt_2=version:
set fer_ver="%opt_2%"
goto execute

:yes
set fer_repo="fernanda-dev"
set fer_ver="dev"
goto execute

:execute
set fer_dir="fernanda-%fer_ver%-x64-windows"
set qt_ver="6.4.1"
set qt_compiler="msvc2019_64"
set vs_fernanda="%SystemDrive%\Dev\%fer_repo%"

set qt="%SystemDrive%\Qt\%qt_ver%\%qt_compiler%"
set qwin="%qt%\plugins\platforms\qwindows.dll"
set qwinstyle="%qt%\plugins\styles\qwindowsvistastyle.dll"
set qt_core="%qt%\bin\Qt6Core.dll"
set qt_gui="%qt%\bin\Qt6Gui.dll"
set qt_widgets="%qt%\bin\Qt6Widgets.dll"
set qt_xml="%qt%\bin\Qt6Xml.dll"

set vs_fernanda_exe="%vs_fernanda%\x64\Release\fernanda.exe"
set vs_readme="%vs_fernanda%\README.md"
set vs_license="%vs_fernanda%\LICENSE"

set data="%fer_dir%\data"
set platforms="%data%\platforms"
set styles="%data%\styles"

md %platforms%
md %styles%

::
echo Do not use this version of me for serious writing!> "%fer_dir%\For testing.txt"
::

echo repo: %fer_repo%> "%fer_dir%\build.txt"
echo version: %fer_ver%>> "%fer_dir%\build.txt"
echo qt version: %qt_ver%>> "%fer_dir%\build.txt"

echo f|xcopy /v /y /f "%vs_fernanda_exe%" "%data%\fernanda.exe"
echo f|xcopy /v /y /f "%vs_readme%" "%fer_dir%\README.md"
echo f|xcopy /v /y /f "%vs_license%" "%fer_dir%\LICENSE"
echo f|xcopy /v /y /f "%qwin%" "%platforms%\qwindows.dll"
echo f|xcopy /v /y /f "%qwinstyle%" "%styles%\qwindowsvistastyle.dll"
echo f|xcopy /v /y /f "%qt_core%" "%data%\Qt6Core.dll"
echo f|xcopy /v /y /f "%qt_gui%" "%data%\Qt6Gui.dll"
echo f|xcopy /v /y /f "%qt_widgets%" "%data%\Qt6Widgets.dll"
echo f|xcopy /v /y /f "%qt_xml%" "%data%\Qt6Xml.dll"

set fer_dir=%fer_dir:"=%
set data=%data:"=%
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%fer_dir%\fernanda.lnk');$s.TargetPath='%data%\fernanda.exe';$s.Save()"
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%fer_dir%\fernanda (dev).lnk');$s.TargetPath='%data%\fernanda.exe';$s.Arguments='-dev';$s.Save()"

pause
exit
