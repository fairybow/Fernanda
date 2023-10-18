!include "${NSISDIR}\Contrib\Modern UI\System.nsh"
!include "include\FileAssociation.nsh"
!include FileFunc.nsh
!include Fonts.nsh
!include LogicLib.nsh
!include MUI2.nsh
!include nsDialogs.nsh
!include WinMessages.nsh

RequestExecutionLevel admin

; ----- Definitions -----
!define Q_VERSION "6.5.1"
!define Q_COMPILER "msvc2019_64"

!define APP "Fernanda"
!define PUB "fairybow"

!define DATA "$INSTDIR\data"
!define PLATFORMS "${DATA}\platforms"
!define STYLES "${DATA}\styles"
!define TLS "${DATA}\tls"
!define F_DIR "C:\Dev\Fernanda"
!define Q_DIR "C:\Qt\${Q_VERSION}\${Q_COMPILER}"
!define Q_BIN "${Q_DIR}\bin"
!define Q_RESOURCES "${Q_DIR}\resources"
!define Q_PLUGINS "${Q_DIR}\plugins"

!define F_ICON "${F_DIR}\Fernanda\resources\Fernanda.ico"
!define F_EXE "${DATA}\Fernanda.exe"
!define UN_F_EXE "$INSTDIR\Uninstall.exe"

!define F_FONTS "${F_DIR}\Fernanda\external"

!define MUI_PAGE_WELCOME
!define MUI_PAGE_LICENSE
!define MUI_PAGE_DIRECTORY
!define MUI_PAGE_INSTFILES
!define MUI_PAGE_FINISH
!define MUI_ABORTWARNING
!define MUI_UNINSTALLER
!define MUI_UNCONFIRMPAGE
!define MUI_ICON "${F_ICON}"
!define MUI_UNICON "${F_ICON}"

!define F_REG "SOFTWARE\${APP}"
!define UN_F_REG "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"

; ----- Options -----
Var Dialog
Var DesktopCheckbox
Var StartCheckbox
Var HasDesktopCheckbox
Var HasStartCheckbox

Function OptionsPage
	!insertmacro MUI_HEADER_TEXT "Options" "Choose from some additional options."
	nsDialogs::Create 1018
	Pop $Dialog
	${NSD_CreateLabel} 0 0 100% 13u "Select from the following options"
	Pop $0
	${NSD_CreateCheckbox} 0 26du 100% 10u "Create a desktop shortcut"
	Pop $DesktopCheckbox
	${NSD_SetState} $DesktopCheckbox ${BST_CHECKED}
	${NSD_CreateCheckbox} 0 39u 100% 10u "Create Start menu items"
	Pop $StartCheckbox
	${NSD_SetState} $StartCheckbox ${BST_CHECKED}
	nsDialogs::Show
FunctionEnd

Function OptionsPageLeave
	${NSD_GetState} $DesktopCheckbox $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $HasDesktopCheckbox true
	${Else}
		StrCpy $HasDesktopCheckbox false
	${EndIf}

	${NSD_GetState} $StartCheckbox $0
	${If} $0 == ${BST_CHECKED}
		StrCpy $HasStartCheckbox true
	${Else}
		StrCpy $HasStartCheckbox false
	${EndIf}
FunctionEnd

; ----- Misc Functions -----
Function CheckExists
	${If} ${FileExists} "$INSTDIR\*"
		MessageBox MB_YESNO `"$INSTDIR" already exists. Update?` IDYES YES
		Abort
	YES:
		RMDir /r "$INSTDIR"
	${EndIf}
FunctionEnd

; ----- Insert pages -----
;!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${F_DIR}\LICENSE"
Page custom OptionsPage OptionsPageLeave
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; ----- Installation -----
Name "${APP}"
Outfile "Fernanda-setup-Windows-x64.exe"
InstallDir "$PROGRAMFILES64\${APP}"
DirText "Choose a directory"

Section "Install"

	Call CheckExists

	; ----- Copy application files -----
	DetailPrint "Copying application files..."

	SetOutPath "$INSTDIR"
	File "${F_DIR}\LICENSE"
	WriteINIStr "$INSTDIR\Fernanda - GitHub.url" "InternetShortcut" "URL" "https://github.com/fairybow/Fernanda"

	SetOutPath "${DATA}"
	File "${F_DIR}\x64\Release\Fernanda.exe"
	File "${Q_RESOURCES}\icudtl.dat"
	File "${Q_BIN}\Qt6Core.dll"
	File "${Q_BIN}\Qt6Gui.dll"
	File "${Q_BIN}\Qt6Network.dll"
	File "${Q_BIN}\Qt6OpenGL.dll"
	File "${Q_BIN}\Qt6Positioning.dll"
	File "${Q_BIN}\Qt6PrintSupport.dll"
	File "${Q_BIN}\Qt6Qml.dll"
	File "${Q_BIN}\Qt6QmlModels.dll"
	File "${Q_BIN}\Qt6Quick.dll"
	File "${Q_BIN}\Qt6QuickWidgets.dll"
	File "${Q_BIN}\Qt6WebChannel.dll"
	File "${Q_BIN}\Qt6WebEngineCore.dll"
	File "${Q_BIN}\Qt6WebEngineWidgets.dll"
	File "${Q_BIN}\Qt6Widgets.dll"
	File "${Q_BIN}\Qt6Xml.dll"
	File "${Q_RESOURCES}\qtwebengine_devtools_resources.pak"
	File "${Q_RESOURCES}\qtwebengine_resources.pak"
	File "${Q_RESOURCES}\qtwebengine_resources_100p.pak"
	File "${Q_RESOURCES}\qtwebengine_resources_200p.pak"
	File "${Q_BIN}\QtWebEngineProcess.exe"

	SetOutPath "${PLATFORMS}"
	File "${Q_PLUGINS}\platforms\qwindows.dll"

	SetOutPath "${STYLES}"
	File "${Q_PLUGINS}\styles\qwindowsvistastyle.dll"

	SetOutPath "${TLS}"
	File "${Q_PLUGINS}\tls\qschannelbackend.dll"

	CreateShortCut "$INSTDIR\Fernanda.lnk" "${F_EXE}"
	CreateShortCut "$INSTDIR\Fernanda (dev).lnk" "${F_EXE}" "-dev"

	; ----- Install fonts -----
	DetailPrint "Installing fonts..."
	StrCpy $FONT_DIR $FONTS
	!insertmacro InstallTTFFont "${F_FONTS}\mononoki\mononoki-Regular.ttf" "mononoki-Regular"
	!insertmacro InstallTTFFont "${F_FONTS}\mononoki\mononoki-Italic.ttf" "mononoki-Italic"
	!insertmacro InstallTTFFont "${F_FONTS}\mononoki\mononoki-Bold.ttf" "mononoki-Bold"
	!insertmacro InstallTTFFont "${F_FONTS}\mononoki\mononoki-BoldItalic.ttf" "mononoki-BoldItalic"
	SendMessage ${HWND_BROADCAST} ${WM_FONTCHANGE} 0 0 /TIMEOUT=5000

	; ----- Register file extension -----
	DetailPrint "Registering file extension..."
	${registerExtension} "${F_EXE}" ".story" "Fernanda Story File"

	; ----- Create desktop shortcut -----
	${If} $HasDesktopCheckbox == true
		DetailPrint "Creating desktop shortcut..."
		CreateShortCut "$DESKTOP\${APP}.lnk" "${F_EXE}" ""
	${EndIf}

	; ----- Create Start menu items -----
	${If} $HasStartCheckbox == true
		DetailPrint "Creating Start menu items..."
		CreateDirectory "$SMPROGRAMS\${APP}"
		CreateShortCut "$SMPROGRAMS\${APP}\Uninstall.lnk" "${UN_F_EXE}" "" "${UN_F_EXE}" 0
		CreateShortCut "$SMPROGRAMS\${APP}\${APP}.lnk" "${F_EXE}" "" "${F_EXE}" 0
	${EndIf}

	; ----- Create uninstaller -----
	DetailPrint "Creating uninstaller..."
	WriteRegStr HKLM "${UN_F_REG}" "DisplayIcon" "${F_ICON}"
	WriteRegStr HKLM "${UN_F_REG}" "DisplayName" "${APP}"
	WriteRegStr HKLM "${UN_F_REG}" "UninstallString" "${UN_F_EXE}"
	WriteRegStr HKLM "${UN_F_REG}" "Publisher" "${PUB}"
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "${UN_F_REG}" "EstimatedSize" "$0"
	WriteUninstaller "${UN_F_EXE}"

SectionEnd

; ----- Uninstallation -----
Section "Uninstall"

	${unregisterExtension} ".story" "Fernanda Story File"

	RMDir /r "$INSTDIR\*.*"
	RMDir "$INSTDIR"

	Delete "$DESKTOP\${APP}.lnk"
	Delete "$SMPROGRAMS\${APP}\*.*"
	RmDir "$SMPROGRAMS\${APP}"

	DeleteRegKey HKEY_LOCAL_MACHINE "${F_REG}"
	DeleteRegKey HKEY_LOCAL_MACHINE "${UN_F_REG}"

SectionEnd
