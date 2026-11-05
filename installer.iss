#define MyAppName "Автосервис"
#define MyAppExe "AutoService.exe"
#define MyAppVersion "1.0"

[Setup]
AppId={{B6B8C1A2-4F5D-4E8B-9C3A-7D2E1F0A9B11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\AutoService
DefaultGroupName={#MyAppName}
SetupIconFile=resources\app.ico
UninstallDisplayIcon={app}\{#MyAppExe}
OutputDir=installer_out
OutputBaseFilename=AutoService-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"; GroupDescription: "Дополнительно:"

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"
Name: "{group}\Удалить {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "Запустить {#MyAppName}"; Flags: nowait postinstall skipifsilent
