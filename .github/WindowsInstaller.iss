; Fernanda Inno Setup Script (https://jrsoftware.org/isdl.php#stable)
; Required: Pass /DVariableName=x from command line

#ifndef AppVersion
  #error "AppVersion not defined. Pass /DAppVersion=x.x.x"
#endif
#ifndef AppVersionNumeric
  #error "AppVersionNumeric not defined. Pass /DAppVersionNumeric=x.x.x"
#endif
#ifndef InstallerName
  #error "InstallerName not defined. Pass /DInstallerName=name"
#endif
#ifndef ReadmePath
  #error "ReadmePath not defined. Pass /DReadmePath=path"
#endif
#ifndef LicensePath
  #error "LicensePath not defined. Pass /DLicensePath=path"
#endif
#ifndef PrivacyPath
  #error "PrivacyPath not defined. Pass /DPrivacyPath=path"
#endif
#ifndef OutputDir
  #error "OutputDir not defined. Pass /DOutputDir=path"
#endif

#define AppName "Fernanda"
#define AppExeName "Fernanda.exe"

[Setup]
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; NB: Never change or reuse. Windows identifies the installed app by this, so a
; reused ID makes the installer treat another app's install as its own
AppId={{252bd008-ddc1-4de4-8e79-4f9d55db5e00}
AppName={#AppName}
AppPublisher=fairybow
AppVerName={#AppName}
AppVersion={#AppVersion}
CloseApplications=yes
CloseApplicationsFilter={#AppExeName}
Compression=lzma2
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
; The privacy statement, shown before the tasks page, where its update check
; can be switched off
InfoBeforeFile={#PrivacyPath}
LicenseFile={#LicensePath}
MinVersion=10
OutputBaseFilename={#InstallerName}
OutputDir={#OutputDir}
SolidCompression=yes
UninstallDisplayIcon={app}\data\{#AppExeName}
VersionInfoProductVersion={#AppVersionNumeric}
WizardStyle=modern

[InstallDelete]
; Clears the previous version's deployment (stale Qt DLLs and plugins) on
; upgrade. Scoped to data\, which only this installer writes to
Type: filesandordirs; Name: "{app}\data\*"

[Tasks]
Name: "startmenu"; Description: "Create a &Start Menu folder"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce
Name: "desktopicon"; Description: "Create a &Desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce
; Checked unless a previous install unchecked it. Fernanda reads the choice
; from install.ini ([INI]) as its default (AppConfig)
Name: "updatecheck"; Description: "Check for &updates automatically"; GroupDescription: "Updates:"

[Files]
Source: "temp\*"; Excludes: "vc_redist.x64.exe"; DestDir: "{app}\data"; Flags: recursesubdirs ignoreversion
Source: "temp\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
Source: "{#ReadmePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#LicensePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#PrivacyPath}"; DestDir: "{app}"; Flags: ignoreversion

[INI]
; Written only when the update check is unchecked. [InstallDelete] clears it
; first, so checking the box again on a reinstall removes it
Filename: "{app}\data\install.ini"; Section: "Updates"; Key: "CheckAutomatically"; String: "false"; Tasks: not updatecheck

[Icons]
; Always installed
Name: "{app}\{#AppName}"; Filename: "{app}\data\{#AppExeName}"

; Optional
Name: "{group}\{#AppName}"; Filename: "{app}\data\{#AppExeName}"; Tasks: startmenu
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\data\{#AppExeName}"; Tasks: desktopicon

[UninstallDelete]
; install.ini isn't an installed file, so the uninstaller doesn't remove it on
; its own. These run last, so the folders it held are removed here too, if
; nothing else is left in them
Type: files; Name: "{app}\data\install.ini"
Type: dirifempty; Name: "{app}\data"
Type: dirifempty; Name: "{app}"

[Run]
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Runtime..."; Flags: waituntilterminated
