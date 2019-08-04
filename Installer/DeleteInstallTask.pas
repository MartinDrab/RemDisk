Unit DeleteInstallTask;

Interface

Uses
  AbstractInstallTask, InstallerSettings;

Type
  TDeleteFileInstallTask = Class (TAbstractInstallTask)
  Private
    FTargetFileName : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; ATargetFile:WideString; ACritical:Boolean = False); Reintroduce;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SysUtils;

Constructor TDeleteFileInstallTask.Create(AInstallerSettings:TInstallerSettings; ATargetFile:WideString; ACritical:Boolean = False);
begin
Inherited Create(InstallerSettings, ACritical);
FTargetFileName := ATargetFile;
end;

Procedure TDeleteFileInstallTask.Execute;
begin
If Not DeleteFileW(PWideChar(FTargetFileName)) Then
  FErrorCode := GetLastError;
end;

Function TDeleteFileInstallTask.GetDescription:WideString;
begin
Result := Format('Deleting file %s', [FTargetFileName]);
end;



End.
