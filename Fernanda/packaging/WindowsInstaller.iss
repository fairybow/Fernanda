; Fernanda Inno Setup Script (https://jrsoftware.org/isdl.php#stable)
; Required: Pass /DVariableName=x from command line

; TODO: Shutdown (or advise) before updating
; TODO: Clearing previous install before "updating" (replacing)?

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
#ifndef AdditionalTermsPath
  #error "AdditionalTermsPath not defined. Pass /DAdditionalTermsPath=path"
#endif
#ifndef OutputDir
  #error "OutputDir not defined. Pass /DOutputDir=path"
#endif

[Setup]
AppId={{D82F0C66-E341-4953-BD96-372C196A7E9B}
AppName=Fernanda
AppVerName=Fernanda
AppVersion={#AppVersion}
VersionInfoProductVersion={#AppVersionNumeric}
AppPublisher=fairybow
DefaultDirName={autopf}\Fernanda
DefaultGroupName=Fernanda
LicenseFile={#LicensePath}
OutputDir={#OutputDir}
OutputBaseFilename={#InstallerName}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
ChangesAssociations=yes
UninstallDisplayIcon={app}\data\Fernanda.exe
MinVersion=10

[Tasks]
Name: "startmenu"; Description: "Create a &Start Menu folder"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: checkedonce

[Files]
Source: "temp\*"; DestDir: "{app}\data"; Flags: recursesubdirs
Source: "{#ReadmePath}"; DestDir: "{app}\docs"
Source: "{#LicensePath}"; DestDir: "{app}\docs"
Source: "{#AdditionalTermsPath}"; DestDir: "{app}\docs"

[Icons]
; Always installed
Name: "{app}\Fernanda"; Filename: "{app}\data\Fernanda.exe"

; Optional
Name: "{group}\Fernanda"; Filename: "{app}\data\Fernanda.exe"; Tasks: startmenu
Name: "{group}\Uninstall Fernanda"; Filename: "{uninstallexe}"; Tasks: startmenu
Name: "{autodesktop}\Fernanda"; Filename: "{app}\data\Fernanda.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\data\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Installing Visual C++ Runtime..."; Flags: waituntilterminated skipifsilent

[Registry]
; Define the ProgID
Root: HKA; Subkey: "Software\Classes\Fernanda.Notebook"; ValueType: string; ValueData: "Fernanda Notebook"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Fernanda.Notebook"; ValueType: string; ValueName: "FriendlyTypeName"; ValueData: "Fernanda Notebook"
Root: HKA; Subkey: "Software\Classes\Fernanda.Notebook\DefaultIcon"; ValueType: string; ValueData: "{app}\data\Fernanda.exe,0"
; ^ TODO: Change this to Notebook icon once it exists
Root: HKA; Subkey: "Software\Classes\Fernanda.Notebook\shell\open\command"; ValueType: string; ValueData: """{app}\data\Fernanda.exe"" ""%1"""

; Associate .fnx extension with ProgID
Root: HKA; Subkey: "Software\Classes\.fnx"; ValueType: string; ValueData: "Fernanda.Notebook"; Flags: uninsdeletevalue
