@echo off

:start
set fer_repo=
set fer_dir=
set fer_ver=

:dev
set opt_1=
set /p opt_1=Build from dev repo? (Y/N):
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
set qt_ver="6.4.2"
set qt_compiler="msvc2019_64"
set vs_fernanda="%SystemDrive%\Dev\%fer_repo%"

set qt="%SystemDrive%\Qt\%qt_ver%\%qt_compiler%"
set qwin="%qt%\plugins\platforms\qwindows.dll"
set qwinstyle="%qt%\plugins\styles\qwindowsvistastyle.dll"
set qschannelbackend="%qt%\plugins\tls\qschannelbackend.dll"
set qweb_icudtl="%qt%\resources\icudtl.dat"
set qt_core="%qt%\bin\Qt6Core.dll"
set qt_gui="%qt%\bin\Qt6Gui.dll"
set qt_network="%qt%\bin\Qt6Network.dll"
set qt_opengl="%qt%\bin\Qt6OpenGL.dll"
set qt_positioning="%qt%\bin\Qt6Positioning.dll"
set qt_printsupport="%qt%\bin\Qt6PrintSupport.dll"
set qt_qml="%qt%\bin\Qt6Qml.dll"
set qt_qmlmodels="%qt%\bin\Qt6QmlModels.dll"
set qt_quick="%qt%\bin\Qt6Quick.dll"
set qt_quickwidgets="%qt%\bin\Qt6QuickWidgets.dll"
set qt_webchannel="%qt%\bin\Qt6WebChannel.dll"
set qt_wecore="%qt%\bin\Qt6WebEngineCore.dll"
set qt_wewidgets="%qt%\bin\Qt6WebEngineWidgets.dll"
set qt_widgets="%qt%\bin\Qt6Widgets.dll"
set qt_xml="%qt%\bin\Qt6Xml.dll"
set qweb_r1="%qt%\resources\qtwebengine_devtools_resources.pak"
set qweb_r2="%qt%\resources\qtwebengine_resources.pak"
set qweb_r3="%qt%\resources\qtwebengine_resources_100p.pak"
set qweb_r4="%qt%\resources\qtwebengine_resources_200p.pak"
set qweb_exe="%qt%\resources\QtWebEngineProcess.exe"

set vs_fernanda_exe="%vs_fernanda%\x64\Release\fernanda.exe"
set vs_readme="%vs_fernanda%\README.md"
set vs_license="%vs_fernanda%\LICENSE"

set data="%fer_dir%\data"
set platforms="%data%\platforms"
set styles="%data%\styles"
set tls="%data%\tls"

md %platforms%
md %styles%
md %tls%

echo repo: %fer_repo%> "%fer_dir%\build.txt"
echo version: %fer_ver%>> "%fer_dir%\build.txt"
echo qt version: %qt_ver%>> "%fer_dir%\build.txt"

echo f|xcopy /y /f "%vs_fernanda_exe%" "%data%\fernanda.exe"
echo f|xcopy /y /f "%vs_readme%" "%fer_dir%\README.md"
echo f|xcopy /y /f "%vs_license%" "%fer_dir%\LICENSE"
echo f|xcopy /y /f "%qwin%" "%platforms%\qwindows.dll"
echo f|xcopy /y /f "%qwinstyle%" "%styles%\qwindowsvistastyle.dll"
echo f|xcopy /y /f "%qschannelbackend%" "%tls%\qschannelbackend.dll"
echo f|xcopy /y /f "qweb_icudtl" "%data%\icudtl.dat"
echo f|xcopy /y /f "%qt_core%" "%data%\Qt6Core.dll"
echo f|xcopy /y /f "%qt_gui%" "%data%\Qt6Gui.dll"
echo f|xcopy /y /f "%qt_network%" "%data%\Qt6Network.dll"
echo f|xcopy /y /f "qt_opengl" "%data%\Qt6OpenGL.dll"
echo f|xcopy /y /f "qt_positioning" "%data%\Qt6Positioning.dll"
echo f|xcopy /y /f "%qt_printsupport%" "%data%\Qt6PrintSupport.dll"
echo f|xcopy /y /f "qt_qml" "%data%\Qt6Qml.dll"
echo f|xcopy /y /f "qt_qmlmodels" "%data%\Qt6QmlModels.dll"
echo f|xcopy /y /f "qt_quick" "%data%\Qt6Quick.dll"
echo f|xcopy /y /f "qt_quickwidgets" "%data%\Qt6QuickWidgets.dll"
echo f|xcopy /y /f "qt_webchannel" "%data%\Qt6WebChannel.dll"
echo f|xcopy /y /f "qt_wecore" "%data%\Qt6WebEngineCore.dll"
echo f|xcopy /y /f "qt_wewidgets" "%data%\Qt6WebEngineWidgets.dll"
echo f|xcopy /y /f "%qt_widgets%" "%data%\Qt6Widgets.dll"
echo f|xcopy /y /f "%qt_xml%" "%data%\Qt6Xml.dll"
echo f|xcopy /y /f "qweb_r1" "%data%\qtwebengine_devtools_resources.pak"
echo f|xcopy /y /f "qweb_r2" "%data%\qtwebengine_resources.pak"
echo f|xcopy /y /f "qweb_r3" "%data%\qtwebengine_resources_100p.pak"
echo f|xcopy /y /f "qweb_r4" "%data%\qtwebengine_resources_200p.pak"
echo f|xcopy /y /f "qweb_exe" "%data%\QtWebEngineProcess.exe"

set fer_dir=%fer_dir:"=%
set data=%data:"=%
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%fer_dir%\fernanda.lnk');$s.TargetPath='%data%\fernanda.exe';$s.Save()"
powershell "$s=(New-Object -COM WScript.Shell).CreateShortcut('%fer_dir%\fernanda (dev).lnk');$s.TargetPath='%data%\fernanda.exe';$s.Arguments='-dev';$s.Save()"

pause
exit
