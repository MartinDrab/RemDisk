Unit CreateDirInstallTask;

Interface

Uses
  AbstractInstallTask, InstallerSettings, DeleteInstallTask;

Type
  TCreateDirInstallTask = Class (TAbstractInstallTask)
  Private
    FTargetFileName : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; ATargetFile:WideString; ACritical:Boolean = False); Reintroduce;

    Function CounterTask:TAbstractInstallTask; Override;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SysUtils;

Constructor TCreateDirInstallTask.Create(AInstallerSettings:TInstallerSettings; ATargetFile:WideString; ACritical:Boolean = False);
begin
Inherited Create(InstallerSettings, ACritical);
FTargetFileName := ATargetFile;
end;

Procedure TCreateDirInstallTask.Execute;
Var
  I : Integer;
  tmp : WideString;
begin
For I := 1 To Length(FTargetFileName) Do
  begin
  If (FTargetFileName[I] = '\') And (I > 3) Then
    begin
    If Not CreateDirectoryW(PWideChar(tmp), Nil) Then
      FErrorCode := GetLastError;

    If FErrorCode = ERROR_ALREADY_EXISTS Then
      FErrorCode := 0;
    end;

  If FErrorCode <> 0 Then
    Break;
  end;
end;

Function TCreateDirInstallTask.GetDescription:WideString;
begin
Result := Format('Creating directory %s', [FTargetFileName]);
end;

Function TCreateDirInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

Result := TDeleteFileInstallTask.Create(InstallerSettings, FTargetFileName, Critical);
end;



End.

