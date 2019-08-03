Unit InstallerSettings;

Interface

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

    Constructor Create;
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
end;

End.
