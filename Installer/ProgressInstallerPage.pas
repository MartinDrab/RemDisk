Unit ProgressInstallerPage;

Interface

Uses
  Classes, Generics.Collections, SyncObjs,
  InstallerSettings, AbstractInstallerPage,
  AbstractInstallTask, InstallTaskThread;

Type
  TProgressInstallerPage = Class (TAbstractInstallerPage)
  Private
    FTaskThread : TInstallTaskThread;
  Protected
    Function GetNextButton:TInstallerPageSwitchingButton; Override;
  Public
    Function NextPressed:Boolean; Override;
  end;

Implementation

(** TProgressInstallerPage **)

Function TProgressInstallerPage.GetNextButton:TInstallerPageSwitchingButton;
begin
Result := Inherited GetNextButton;
Result.Caption := 'Install';
Result.Enabled := Not Assigned(FTaskThread);
end;

Function TProgressInstallerPage.NextPressed:Boolean;
begin
Result := False;
end;


End.
