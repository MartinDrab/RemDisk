Unit ResExtractInstallTask;

Interface

Uses
  AbstractInstallTask, InstallerSettings, DeleteInstallTask;

Type
  TResExtractInstallTask = Class (TAbstractInstallTask)
  Private
    FResourceName : WideString;
    FTargetFileName : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; ATempDirectory:WideString; ATargetFile:WideString; ACritical:Boolean = False); Reintroduce;

    Function CounterTask:TAbstractInstallTask; Override;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SysUtils, Classes;

Constructor TResExtractInstallTask.Create(AInstallerSettings:TInstallerSettings; ATempDirectory:WideString; ATargetFile:WideString; ACritical:Boolean = False);
begin
Inherited Create(InstallerSettings, ACritical);
FResourceName := ATargetFile;
FTargetFileName := Format('%s\%s', [ATempDirectory, ATargetFile]);
end;

Procedure TResExtractInstallTask.Execute;
Var
  data : Pointer;
  hFile : THandle;
  dataSize : Cardinal;
  hModule : THandle;
  hRsrc : THandle;
  hGlobal : THandle;
  rs : TResourceStream;
begin
hModule := GetModuleHandleW(Nil);
If hModule <> 0 Then
  begin
  Try
    rs := Nil;
    rs := TResourceStream.Create(hModule, FResourceName, RT_RCDATA);
    Try
      rs.Position := 0;
      rs.SaveToFile(FTargetFileName);
    Finally
      rs.Free;
      end;
  Except
    FErrorCode := GetLastError;
    end;
  end
Else FErrorCode := GetLastError;
end;

Function TResExtractInstallTask.GetDescription:WideString;
begin
Result := Format('Extracting %s', [FResourceName]);
end;

Function TResExtractInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

Result := TDeleteFileInstallTask.Create(InstallerSettings, FTargetFileName);
end;



End.

