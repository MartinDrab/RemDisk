Unit InstallINFInstallTask;

Interface

Uses
  Classes,
  AbstractInstallTask, InstallerSettings, DeleteInstallTask;

Type
  TInstallINFInstallTask = Class (TAbstractInstallTask)
  Private
    FINFFileName : WideString;
    FInstall : Boolean;
    FINFDirectory : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; AINFFileName:WideString; AInstall:Boolean; ACritical:Boolean = False); Reintroduce;

    Function CounterTask:TAbstractInstallTask; Override;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SetupApi, SysUtils;


Constructor TInstallINFInstallTask.Create(AInstallerSettings:TInstallerSettings; AINFFileName:WideString; AInstall:Boolean; ACritical:Boolean = False);
Var
  winDir : Array [0..MAX_PATH - 1] Of WideChar;
begin
Inherited Create(InstallerSettings, ACritical);
FINFFileName := AINFFileName;
FInstall := AInstall;
IF GetWindowsDirectoryW(winDir, SizeOf(WinDir) Div SizeOf(WideChar)) > 0 Then
  FINFDirectory := Format('%s\Inf', [WideString(winDir)]);
end;

Procedure TInstallINFInstallTask.Execute;
Var
  hSetupApi : THandle;
  mode : Cardinal;
  sectionName : WideString;
  targetFileName : WideString;
  command : WideString;
begin
If FInstall Then
  begin
  sectionName := 'DefaultInstall';
  mode := 128;
  end
Else begin
  sectionName := 'DefaultUninstall';
  mode := 128;
  end;

hSetupApi := GetModuleHandleW('setupapi.dll');
If hSetupApi <> 0 Then
  begin
  targetFileName := Format('%s\%s', [FINFDirectory, ExtractFileName(FINFFileName)]);
  If FInstall Then
    begin
    If Not CopyFileW(PWideChar(FINFFileName), PWideChar(targetFileName), False) Then
      FErrorCode := GetLastError;
   end;

  If FErrorCode = 0 Then
    begin
    command := Format('"%s %u %s"', [sectionName, mode, targetFileName]);
    SetupApi.InstallHinfSectionW(0, hSetupApi, PWideChar(command), SW_SHOWNORMAL);
    If (Not Finstall) Or (FErrorCode <> 0) Then
      DeleteFileW(PWideChar(targetFileName));
    end;
  end
Else FErrorCode := GetLastError;
end;

Function TInstallINFInstallTask.GetDescription:WideString;
Var
  actionString : WideString;
begin
actionString := 'Installing';
If Not FInstall Then
  actionString := 'Uninstalling';

Result := Format('%s INF %s', [actionString, FINFFileName]);
end;

Function TInstallINFInstallTask.CounterTask:TAbstractInstallTask;
Var
  targetFileName : WideString;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

targetFileName := Format('%s\%s', [FINFDirectory, ExtractFileName(FINFFileName)]);
Result := TInstallINFInstallTask.Create(InstallerSettings, targetFileName, False);
end;



End.

