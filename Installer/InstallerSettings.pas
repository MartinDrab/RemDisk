Unit InstallerSettings;

Interface

Uses
  Classes;

Type
  EInstallerAction = (
    iaUndefined,
    iaInstall,
    iaRepair,
    iaRemove,
    iaMax
  );

  TInstallerSettings = Class
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

Constructor TInstallerSettings.Create;
begin
Inherited Create;
Action := iaInstall;
ProgramDirectory := 'C:\Program Files\RemDisk';
DesktopShortcut := True;
StartMenu := True;
AllUsers := True;
FileList := TStringList.Create;
Description := 'RemDisk Virtual Disk Manager';
StartMenuDir := 'RemDisk';
FileList.Add('RemDisk.exe');
FileList.Add('RemDisk.dll');
FileList.Add('RemDisk.sys');
FileList.Add('RemDisk.inf');
FileList.Add('RemDisk.cat');
FileList.Add('RemBus.sys');
FileList.Add('RemBus.inf');
FileList.Add('RemBus.cat');
end;

Destructor TInstallerSettings.Destroy;
begin
FileList.Free;
Inherited Destroy;
end;



End.

