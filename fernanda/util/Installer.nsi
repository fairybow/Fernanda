!include "MUI.nsh"
!include "${NSISDIR}\Contrib\Modern UI\System.nsh"
!include "..\include\nsis\FileAssociation.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

; Checkbox options (todo)
; - add desktop shortcuts
; - add start menu items
; - open on close

; ---------- Qt ----------
!define Q_VERSION "6.4.2"
!define Q_COMPILER "msvc2019_64"

; ---------- App ----------
!define APP "Fernanda"
!define PUB "@fairybow"
Name "${APP}"
Outfile "Fernanda-setup-x64-Windows.exe"

; ---------- Directories ----------
InstallDir "$PROGRAMFILES64\${APP}"
DirText "Choose a directory"
!define DATA "$INSTDIR\data"
!define PLATFORMS "${DATA}\platforms"
!define STYLES "${DATA}\styles"
!define TLS "${DATA}\tls"
!define F_DIR "C:\Dev\fernanda"
!define Q_DIR "C:\Qt\${Q_VERSION}\${Q_COMPILER}"
!define Q_BIN "${Q_DIR}\bin"
!define Q_RESOURCES "${Q_DIR}\resources"
!define Q_PLUGINS "${Q_DIR}\plugins"

; ---------- Files ----------
!define F_ICON "${F_DIR}\fernanda\res\icons\Fernanda.ico"
!define F_EXE "${DATA}\Fernanda.exe"
!define UN_F_EXE "$INSTDIR\Uninstall.exe"

; ---------- Registry ----------
!define F_REG "SOFTWARE\${APP}"
!define UN_F_REG "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"

; ---------- MUI ----------
!define MUI_WELCOMEPAGE
!define MUI_LICENSEPAGE
!define MUI_DIRECTORYPAGE
!define MUI_ABORTWARNING
!define MUI_UNINSTALLER
!define MUI_UNCONFIRMPAGE
!define MUI_FINISHPAGE
!define MUI_ICON "${F_ICON}"
!define MUI_UNICON "${F_ICON}"
!insertmacro MUI_LANGUAGE "English"

; ---------- Functions ----------
Function CheckExists
	${If} ${FileExists} "$INSTDIR\*"
		MessageBox MB_YESNO `"$INSTDIR" already exists. Update?` IDYES YES
		Abort
	YES:
		RMDir /r "$INSTDIR"
	${EndIf}
FunctionEnd

; ---------- Installation ----------
Section "Install"

	Call CheckExists

	; Write docs
	SetOutPath "$INSTDIR"
	File "${F_DIR}\LICENSE"
	WriteINIStr "$INSTDIR\Fernanda - GitHub.url" "InternetShortcut" "URL" "https://github.com/fairybow/fernanda"

	; Write data
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

	; Write platforms and styles
	SetOutPath "${PLATFORMS}"
	File "${Q_PLUGINS}\platforms\qwindows.dll"
	SetOutPath "${STYLES}"
	File "${Q_PLUGINS}\styles\qwindowsvistastyle.dll"
	SetOutPath "${TLS}"
	File "${Q_PLUGINS}\tls\qschannelbackend.dll"

	; Write shortcuts
	CreateShortCut "$INSTDIR\Fernanda.lnk" "${F_EXE}"
	CreateShortCut "$INSTDIR\Fernanda (dev).lnk" "${F_EXE}" "-dev"

	; Register file extension
	${registerExtension} "${F_EXE}" ".story" "Fernanda Story File"

	; Create desktop shortcut
	CreateShortCut "$DESKTOP\${APP}.lnk" "${F_EXE}" ""
 
	; Create start menu items
	CreateDirectory "$SMPROGRAMS\${APP}"
	CreateShortCut "$SMPROGRAMS\${APP}\Uninstall.lnk" "${UN_F_EXE}" "" "${UN_F_EXE}" 0
	CreateShortCut "$SMPROGRAMS\${APP}\${APP}.lnk" "${F_EXE}" "" "${F_EXE}" 0
 
	; Register uninstall info
	WriteRegStr HKLM "${UN_F_REG}" "DisplayIcon" "${F_ICON}"
	WriteRegStr HKLM "${UN_F_REG}" "DisplayName" "${APP}"
	WriteRegStr HKLM "${UN_F_REG}" "UninstallString" "${UN_F_EXE}"
	WriteRegStr HKLM "${UN_F_REG}" "Publisher" "${PUB}"
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	IntFmt $0 "0x%08X" $0
	WriteRegDWORD HKLM "${UN_F_REG}" "EstimatedSize" "$0"
 
	WriteUninstaller "${UN_F_EXE}"

SectionEnd

; ---------- Uninstallation ----------
Section "Uninstall"

	${unregisterExtension} ".story" "Fernanda Story File"

	; Remove application from the firewall exception list
	; SimpleFC::RemoveApplication "${F_EXE}"
	; Pop $0

	RMDir /r "$INSTDIR\*.*"
	RMDir "$INSTDIR"

	Delete "$DESKTOP\${APP}.lnk"
	Delete "$SMPROGRAMS\${APP}\*.*"
	RmDir  "$SMPROGRAMS\${APP}"

	DeleteRegKey HKEY_LOCAL_MACHINE "${F_REG}"
	DeleteRegKey HKEY_LOCAL_MACHINE "${UN_F_REG}"
 
SectionEnd
