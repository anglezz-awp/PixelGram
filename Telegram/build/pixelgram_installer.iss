#define MyAppName "PixelGram Desktop"
#define MyAppShortName "PixelGram"
#define MyAppPublisher "anglezz"
#define MyAppURL "https://github.com/anglezz-awp/PixelGram"
#define MyAppExeName "PixelGram.exe"
#define MyAppVersion "7.4.4"
#define MyAppId "7B96E8A1-9F3A-42FB-96E2-B3A6556D6872"
#define ReleaseExe "G:\TBuild\tdesktop\out\Release\Telegram.exe"
#define IconFile "G:\TBuild\tdesktop\Telegram\Resources\art\icon256.ico"

[Setup]
AppId={{{#MyAppId}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion} (x64)
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={userappdata}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=C:\Users\anglezz\Desktop
OutputBaseFilename=PixelGram-v7.4.4-x64-setup
SetupIconFile={#IconFile}
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
DisableStartupPrompt=yes
PrivilegesRequired=lowest
VersionInfoVersion={#MyAppVersion}.0
CloseApplications=force
DisableDirPage=no
DisableProgramGroupPage=no
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "ru";      MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#ReleaseExe}"; DestDir: "{app}"; DestName: "PixelGram.exe"; Flags: ignoreversion
Source: "{#ReleaseExe}"; DestDir: "{app}"; DestName: "Telegram.exe"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppShortName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppShortName}}"; Filename: "{uninstallexe}"
Name: "{userdesktop}\{#MyAppShortName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppShortName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: files; Name: "{app}\log.txt"
Type: files; Name: "{app}\ws_proxy.log"
Type: filesandordirs; Name: "{app}\DebugLogs"
Type: filesandordirs; Name: "{app}\tdumps"
