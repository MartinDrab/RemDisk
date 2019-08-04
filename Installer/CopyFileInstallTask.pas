Unit CopyFileInstallTask;

Interface

Uses
  AbstractInstallTask, InstallerSettings, DeleteInstallTask;

Type
  TCopyFileInstallTask = Class (TAbstractInstallTask)
  Private
    FSourceFileName : WideString;
    FTargetFileName : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; ASourceFile:WideString; ATargetFile:WideString; ACritical:Boolean = False); Reintroduce;

    Function CounterTask:TAbstractInstallTask; Override;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SysUtils;

Constructor TCopyFileInstallTask.Create(AInstallerSettings:TInstallerSettings; ASourceFile:WideString; ATargetFile:WideString; ACritical:Boolean = False);
begin
Inherited Create(InstallerSettings, ACritical);
FSourceFileName := ASourceFile;
FTargetFileName := ATargetFile;
end;

Procedure TCopyFileInstallTask.Execute;
begin
If Not CopyFileW(PWideChar(FSourceFileName), PWideChar(FTargetFileName), True) Then
  FErrorCode := GetLastError;
end;

Function TCopyFileInstallTask.GetDescription:WideString;
begin
Result := Format('Copying %s --> %s', [ExtractFileName(FSourceFileName), FTargetFileName]);
end;

Function TCopyFileInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

Result := TDeleteFileInstallTask.Create(InstallerSettings, FTargetFileName, Critical);
end;



End.

