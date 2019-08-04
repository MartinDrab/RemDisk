Unit InstallerSettings;

Interface

Uses
  Classes, Windows;

Type
  EInstallerAction = (
    iaUndefined,
    iaInstall,
    iaRepair,
    iaRemove,
    iaMax
  );

  TInstallerSettings = Class
  Private
    FhModule : THandle;
    Function RSCReadString(AName:WideString; Var AValue:WideString):Boolean;
    Function RSCReadStringList(AName:WideString; AList:TStrings):Boolean;
  Public
    Action : EInstallerAction;
    ProgramDirectory : WideString;
    AllUsers : Boolean;
    DesktopShortcut : Boolean;
    StartMenu : Boolean;
    QuickLaunch : Boolean;
    FileList : TStringList;
    Description : WideString;
    StartMenuDir : WideString;

    Constructor Create;
    Destructor Destroy; Override;
  end;

Implementation

Uses
  SysUtils;

Constructor TInstallerSettings.Create;
begin
Inherited Create;
FhModule := GetModuleHandleW(Nil);
Action := iaInstall;
ProgramDirectory := 'C:\Program Files\RemDisk';
DesktopShortcut := True;
StartMenu := True;
AllUsers := True;
FileList := TStringList.Create;
Description := 'RemDisk Virtual Disk Manager';
RSCReadString('DESCRIPTION', Description);
StartMenuDir := 'RemDisk';
RSCReadString('APPNAME', StartMenuDir);
FileList.Add('RemDisk.exe');
FileList.Add('RemDisk.dll');
FileList.Add('RemDisk.sys');
FileList.Add('RemDisk.inf');
FileList.Add('RemDisk.cat');
FileList.Add('RemBus.sys');
FileList.Add('RemBus.inf');
FileList.Add('RemBus.cat');
RSCReadStringList('FILES', FileList);
end;

Destructor TInstallerSettings.Destroy;
begin
FileList.Free;
Inherited Destroy;
end;

Function TInstallerSettings.RSCReadString(AName:WideString; Var AValue:WideString):Boolean;
Var
  rs : TResourceStream;
begin
Try
  rs := TResourceStream.Create(FhModule, AName, RT_RCDATA);
  Try
    rs.Position := 0;
    AValue := WideCharToString(rs.Memory);
  Finally
    rs.Free;
    end;
Except
  Result := False;
  end;
end;


Function TInstallerSettings.RSCReadStringList(AName:WideString; AList:TStrings):Boolean;
Var
  rs : TResourceStream;
  ws : PWideChar;
begin
Try
  rs := TResourceStream.Create(FhModule, AName, RT_RCDATA);
  Try
    AList.Clear;
    rs.Position := 0;
    ws := rs.Memory;
    While (Assigned(ws)) And (ws^ <> #0) Do
      begin
      AList.Add(WideCharToString(ws));
      ws := (ws + StrLen(ws) + 1);
      end;
  Finally
    rs.Free;
    end;
Except
  Result := False;
  end;
end;


End.

