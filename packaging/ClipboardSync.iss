; Build with:
;   iscc /DMyAppVersion=1.2.3 /DSourceDir=<deployed app folder> packaging\ClipboardSync.iss
;
; Written for Inno Setup 6.7 (on every GitHub Windows runner); 7 compiles it too.

#ifndef MyAppVersion
  #define MyAppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\build\deploy"
#endif

#define MyAppName "Clipboard Sync"
#define MyAppExeName "ClipboardSync.exe"
#define MyAppUrl "https://github.com/skoulik/ClipboardSync"

[Setup]
AppId={{008662E1-A21D-4682-86C4-637A4A5FBAF3}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Sergei Kulik
AppPublisherURL={#MyAppUrl}
AppSupportURL={#MyAppUrl}/issues
AppUpdatesURL={#MyAppUrl}/releases
VersionInfoVersion={#MyAppVersion}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
; Per-user by default, no UAC prompt; the first page offers the machine-wide install, which
; alone can add the firewall rule.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; The app's autostart entry is per-user (HKCU) in either mode.
UsedUserAreasWarning=no
OutputDir=..\build\installer
OutputBaseFilename=ClipboardSync-{#MyAppVersion}-windows-x64
SetupIconFile=..\icon\ClipboardSync.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
MinVersion=10.0
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
Compression=lzma2/ultra64
SolidCompression=yes

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Registry]
; Written by the app's "Automatically start at login"; removed here so no entry survives
; pointing at a deleted exe.
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: none; ValueName: "{#MyAppName}"; Flags: dontcreatekey uninsdeletevalue

[Run]
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall add rule name=""{#MyAppName}"" dir=in action=allow protocol=UDP program=""{app}\{#MyAppExeName}"" enable=yes"; Flags: runhidden; Check: IsAdminInstallMode; StatusMsg: "Adding the firewall rule..."
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\taskkill.exe"; Parameters: "/f /im {#MyAppExeName}"; Flags: runhidden; RunOnceId: "StopApp"
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""{#MyAppName}"" program=""{app}\{#MyAppExeName}"""; Flags: runhidden; Check: IsAdminInstallMode; RunOnceId: "DelFirewallRule"
