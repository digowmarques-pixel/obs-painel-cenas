; Instalador do Painel de Cenas para OBS Studio (Windows)
; Compilado pelo GitHub Actions com Inno Setup 6.
; Parâmetros: /DMyAppVersion=1.0.0 /DSourceDir=<pasta do plugin empacotado>

#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\release\RelWithDebInfo\obs-painel-cenas"
#endif

#define MyAppName "Painel de Cenas para OBS"
#define PluginId "obs-painel-cenas"

[Setup]
AppId={{CD747E03-21BA-4825-9AFD-643BD080135E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Painel de Cenas
DefaultDirName={commonappdata}\obs-studio\plugins\{#PluginId}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename={#PluginId}-{#MyAppVersion}-windows-x64-instalador
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
CloseApplications=no

[Languages]
Name: "ptbr"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Messages]
ptbr.WelcomeLabel2=Isto vai instalar o [name/ver] no seu OBS Studio.%n%nFeche o OBS antes de continuar.
en.WelcomeLabel2=This will install [name/ver] into OBS Studio.%n%nPlease close OBS before continuing.

[Files]
; Estrutura nova (OBS 33+): a DLL fica direto na pasta do plugin
Source: "{#SourceDir}\bin\64bit\{#PluginId}.dll"; DestDir: "{app}"; Flags: ignoreversion
; Estrutura antiga (OBS 28 a 32): a DLL fica em bin\64bit
Source: "{#SourceDir}\bin\64bit\{#PluginId}.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
; Traduções e dados
Source: "{#SourceDir}\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{app}"
