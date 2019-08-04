Unit ShortcutInstallTask;

Interface

Uses
  InstallerSettings, AbstractInstallTask;

Type
  EShortcutType = (
    stDesktop,
    stStartMenu,
    stQuickLaunch
  );

  TShortcutInstallTask = Class (TAbstractInstallTask)
  Private
    FDescription : WideString;
    FShortcutType : EShortcutType;
    FTargetFileName : WideString;
    FLinkName : WideString;
    Function CreateShortcut:Cardinal;
    Function ShortcutTypeToString(AShortcutType:EShortcutType):WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; AType:EShortcutType; ATargetFile:WideString; ADescription:WideString; ACritical:Boolean = False); Reintroduce;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
    Function CounterTask:TAbstractInstallTask; Override;

    Property LinkName : WideString Read FLinkName;
  end;

Implementation

Uses
  Windows, SysUtils, Classes, ShellAPI, ShlObj, ComObj, ActiveX,
  DeleteInstallTask;


Constructor TShortcutInstallTask.Create(AInstallerSettings:TInstallerSettings; AType:EShortcutType; ATargetFile:WideString; ADescription:WideString; ACritical:Boolean = False);
begin
Inherited Create(AInstallerSettings, ACritical);
FTargetFileName := Format('%s\%s', [AInstallerSettings.ProgramDirectory, ATargetFile]);
FDescription := ADescription;
FShortcutType := AType;
end;


Function TShortcutInstallTask.CreateShortcut:Cardinal;
Var
  MyObject : IUnknown;
  MySLink : IShellLink;
  MyPFile : IPersistFile;
  Directory : WideString;
  LinkFileName : Widestring;
  foldersKey : HKEY;
  groupsKey : HKEY;
  regValue : Array [0..MAX_PATH] Of WideChar;
  regValueSize : Cardinal;
begin
Result := 0;
MyObject := CreateComObject(CLSID_ShellLink);
MySLink := MyObject As IShellLink;
MyPFile := MyObject As IPersistFile;
MySLink.SetPath(PWideChar(FTargetFileName));
MySLink.SetWorkingDirectory(PWideChar(ExtractFileDir(FTargetFileName)));
MySLink.SetIconLocation(PWideChar(FTargetFileName), 0);
MySLink.SetDescription(PWideChar(FDescription));
Result := RegOpenKeyExW(HKEY_CURRENT_USER, 'Software\MicroSoft\Windows\CurrentVersion\Explorer\Shell Folders', 0, KEY_QUERY_VALUE, foldersKey);
If Result = 0 Then
  begin
  LinkFileName := ExtractFileName(ChangeFileExt(FTargetFileName, '.lnk'));
  ZeroMemory(@regValue, SizeOf(regValue));
  regValueSize := SizeOf(regValue) - SizeOf(WideChar);
  Case FShortcutType Of
    stDesktop : Result := RegQueryValueExW(foldersKey, 'Desktop', Nil, Nil, @regValue, @regValueSize);
    stStartMenu : Result := RegQueryValueExW(foldersKey, 'Start Menu', Nil, Nil, @regValue, @regValueSize);
    stQuickLaunch : begin
      Result := RegOpenKeyExW(HKEY_CURRENT_USER, 'Software\MicroSoft\Windows\CurrentVersion\GrpConv\MapGroups', 0, KEY_QUERY_VALUE, groupsKey);
      If Result = 0 Then
        begin
        ZeroMemory(@regValue, SizeOf(regValue));
        regValueSize := SizeOf(regValue) - SizeOf(WideChar);
        Result := RegQueryValueExW(groupsKey, 'Quick Launch', Nil, Nil, @regValue, @regValueSize);
        RegCloseKey(groupsKey);
        end;
      end;
    Else Result := ERROR_NOT_SUPPORTED;
    end;

  If Result = 0 Then
    begin
    Directory := regValue;
    If FShortcutType = stStartMenu Then
      Directory := Directory + '\' + ExtractFileName(ExtractFileDir(FTargetFileName));

    FLinkName := Directory + '\' + LinkFileName;
    Result := MyPFile.Save(PWideChar(FLinkName), False);
    end;

  RegCloseKey(foldersKey);
  end;

MyObject._Release;
end;

Procedure TShortcutInstallTask.Execute;
begin
FErrorCode := CreateShortcut;
end;

Function TShortcutInstallTask.GetDescription:WideString;
begin
Result := Format('Create %s shortcut for "%s"', [ShortcutTypeToString(FShortcutType), FTargetFileName]);
end;

Function TShortcutInstallTask.ShortcutTypeToString(AShortcutType:EShortcutType):WideString;
begin
Result := '';
Case AShortcutType Of
  stDesktop: Result := 'Desktop';
  stStartMenu: Result := 'Start Menu';
  stQuickLaunch: Result := 'Quick Launch';
  end;
end;

Function TShortcutInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

Result := TDeleteFileInstallTask.Create(InstallerSettings, FLinkName);
end;



End.
