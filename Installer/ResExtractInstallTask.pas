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
  Windows, SysUtils;

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
begin
hModule := GetModuleHandleW(Nil);
If hModule <> 0 Then
  begin
  hRsrc := FindResourceExW(hModule, RT_RCDATA, PWideChar(FResourceName), 1033);
  If hRsrc <> 0 Then
    begin
    dataSize := SizeOfResource(hModule, hRsrc);
    hGlobal := LoadResource(hModule, hRsrc);
    If hGlobal <> 0 Then
      begin
      data := LockResource(hGlobal);
      If Assigned(data) Then
        begin
        hFile := CreateFileW(PWideChar(FTargetFileName), GENERIC_WRITE, 0, Nil, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        If hFile <> INVALID_HANDLE_VALUE Then
          begin
          If Not WriteFile(hFile, data^, dataSize, dataSize, Nil) Then
            FErrorCode := GetLastError;

          CloseHandle(hFile);
          end
        Else FErrorCode := GetLastError;

        UnlockResource(THandle(data));
        end
      Else FErrorCode := GetLastError;

      FreeResource(hGlobal);
      end
    Else FErrorCode := GetLastError;
    end
  Else FErrorCode := GetLastError;
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

