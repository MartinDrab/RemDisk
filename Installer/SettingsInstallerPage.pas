Unit SettingsInstallerPage;

Interface

Uses
  AbstractInstallerPage;

Type
  TSettingsInstallerPage = Class (TAbstractInstallerPage)
    Protected
      Function GetProgramDirectory:WideString;
      Procedure SetProgramDirectory(AValue:WideString);
      Function GetDesktopShortcut:Boolean;
      Procedure SetDesktopShortcut(AValue:Boolean);
      Function GetStartMenu:Boolean;
      Procedure SetStartMenu(AValue:Boolean);
      Function GetAllUsers:Boolean;
      Procedure SetAllUsers(AValue:Boolean);
    Public
      Property ProgramDirectory : WideString Read GetProgramDirectory Write SetProgramDirectory;
      Property DesktopShortcut : Boolean Read GetDesktopShortcut Write SetDesktopShortcut;
      Property StartMenu : Boolean Read GetStartMenu Write SetStartMenu;
      Property AllUsers : Boolean Read GetAllUsers Write SetAllUsers;
    end;


Implementation

Function TSettingsInstallerPage.GetProgramDirectory:WideString;
begin
Result := InstallerSettings.ProgramDirectory;
end;

Procedure TSettingsInstallerPage.SetProgramDirectory(AValue:WideString);
begin
InstallerSettings.ProgramDirectory := AValue;
ReportChange;
end;

Function TSettingsInstallerPage.GetDesktopShortcut:Boolean;
begin
Result := InstallerSettings.DesktopShortcut;
end;

Procedure TSettingsInstallerPage.SetDesktopShortcut(AValue:Boolean);
begin
InstallerSettings.DesktopShortcut := AValue;
ReportChange;
end;

Function TSettingsInstallerPage.GetStartMenu:Boolean;
begin
Result := InstallerSettings.StartMenu;
end;

Procedure TSettingsInstallerPage.SetStartMenu(AValue:Boolean);
begin
InstallerSettings.StartMenu := AValue;
ReportChange;
end;

Function TSettingsInstallerPage.GetAllUsers:Boolean;
begin
Result := InstallerSettings.AllUsers;
end;

Procedure TSettingsInstallerPage.SetAllUsers(AValue:Boolean);
begin
InstallerSettings.AllUsers := AValue;
ReportChange;
end;


end.
