[Setup]
AppName=Ptabtools
AppVerName=Ptabtools 0.4
AppPublisher=Jelmer Vernooij
AppPublisherURL=http://jelmer.vernstok.nl/oss/ptabtools/
AppSupportURL=http://jelmer.vernstok.nl/oss/ptabtools/
AppUpdatesURL=http://jelmer.vernstok.nl/oss/ptabtools/
DefaultDirName={pf}\ptabtools
DefaultGroupName=Ptabtools
DisableProgramGroupPage=yes
InfoBeforeFile=..\README

[Files]
Source: "..\COPYING"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptb2ascii.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptbinfo.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptb.lib"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptb2ly.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptb2xml.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Release\ptb.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "Deps\libglib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "Deps\intl.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "Deps\iconv.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "Deps\libglib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "Deps\libxml2.dll"; DestDir: "{app}"; Flags: ignoreversion;
Source: "..\ptb.h"; DestDir: "{app}"; Flags: ignoreversion
