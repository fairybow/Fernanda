; Hearth Inno Setup Script (https://jrsoftware.org/isdl.php#stable)
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
#ifndef OutputDir
  #error "OutputDir not defined. Pass /DOutputDir=path"
#endif

[Setup]
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
AppId={{D82F0C66-E341-4953-BD96-372C196A7E9B}
AppName=Hearth
AppPublisher=fairybow
AppVerName=Hearth
AppVersion={#AppVersion}
ChangesAssociations=yes
CloseApplications=yes
CloseApplicationsFilter=Hearth.exe
Compression=lzma2
DefaultDirName={autopf}\Hearth
DefaultGroupName=Hearth
LicenseFile={#LicensePath}
MinVersion=10
OutputBaseFilename={#InstallerName}
OutputDir={#OutputDir}
SolidCompression=yes
UninstallDisplayIcon={app}\data\Hearth.exe
VersionInfoProductVersion={#AppVersionNumeric}
WizardStyle=modern

[InstallDelete]
Type: filesandordirs; Name: "{app}\*"

[Tasks]
Name: "startmenu"; Description: "Create a &Start Menu folder"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce
Name: "desktopicon"; Description: "Create a &Desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce

[Files]
Source: "temp\*"; DestDir: "{app}\data"; Flags: recursesubdirs ignoreversion
Source: "{#ReadmePath}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#LicensePath}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Always installed
Name: "{app}\Hearth"; Filename: "{app}\data\Hearth.exe"

; Optional
Name: "{group}\Hearth"; Filename: "{app}\data\Hearth.exe"; Tasks: startmenu
Name: "{group}\Uninstall Hearth"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\Hearth"; Filename: "{app}\data\Hearth.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\data\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Runtime..."; Flags: waituntilterminated skipifsilent

[Registry]
; Define the ProgID
Root: HKA; Subkey: "Software\Classes\Hearth.Notebook"; ValueType: string; ValueData: "Hearth Notebook"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Hearth.Notebook"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "Hearth Notebook"
Root: HKA; Subkey: "Software\Classes\Hearth.Notebook\DefaultIcon"; ValueType: string; ValueData: "{app}\data\Hearth.exe,1"
; ^ TODO: Change this to Notebook icon once it exists
Root: HKA; Subkey: "Software\Classes\Hearth.Notebook\shell\open\command"; ValueType: string; ValueData: """{app}\data\Hearth.exe"" ""%1"""

; Associate .hearthx extension with ProgID
Root: HKA; Subkey: "Software\Classes\.hearthx"; ValueType: string; ValueData: "Hearth.Notebook"; Flags: uninsdeletevalue