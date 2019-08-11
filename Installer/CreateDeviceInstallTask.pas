Unit CreateDeviceInstallTask;

Interface

Uses
  Classes,
  AbstractInstallTask, InstallerSettings, DeleteInstallTask;

Type
  TCreateDeviceInstallTask = Class (TAbstractInstallTask)
  Private
    FINFFileName : WideString;
    FClassGuid : TGuid;
    FHardwareIds : TStringList;
    FDeviceId : WideString;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; AINFFileName:WideString; ADeviceId:WideString; AHardwareIds:TStrings; ACritical:Boolean = False); Reintroduce;

    Function CounterTask:TAbstractInstallTask; Override;

    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
  end;


Implementation

Uses
  Windows, SetupApi, SysUtils;


Constructor TCreateDeviceInstallTask.Create(AInstallerSettings:TInstallerSettings; AINFFileName:WideString; ADeviceId:WideString; AHardwareIds:TStrings; ACritical:Boolean = False);
Var
  stringGuid : WideString;
  line : WideString;
  infContent : TStringList;
begin
Inherited Create(InstallerSettings, ACritical);
FINFFileName := AINFFileName;
infContent := TStringList.Create;
infContent.LoadFromFile(FINFFileName);
stringGuid := '';
For line In infContent Do
  begin
  If Pos(WideString('ClassGuid='), line) = 1 Then
    begin
    stringGuid := Copy(line, Pos('{', line), Pos('}', line) - Pos('{', line) + 1);
    FClassGuid := StringToGuid(stringGuid);
    Break;
    end;
  end;

infContent.Free;

FDeviceId := ADeviceId;
FHardwareIds := TStringList.Create;
FHardwareIds.AddStrings(AHardwareIds);
end;

Procedure TCreateDeviceInstallTask.Execute;
Var
  id : WideString;
  h : HDEVINFO;
  tmp : PWideChar;
  hwIds : PWideChar;
  hwIdsSize : Cardinal;
  devData : SP_DEVINFO_DATA;
begin
hwIdsSize := SizeOf(WideChar);
For id In FHardwareIds Do
  Inc(hwIdsSize, (Length(id) + 1)*SizeOf(WideChar));

hwIds := AllocMem(hwIdsSize);
If Assigned(hwIds) Then
  begin
  tmp := hwIds;
  For id In FHardwareIds Do
    begin
    CopyMemory(tmp, PWideChar(id), (Length(id) + 1)*SizeOf(WideChar));
    Inc(tmp, Length(id) + 1);
    end;

  tmp^ := #0;
  h := SetupDiCreateDeviceInfoList(FClassGuid, 0);
  If h <> INVALID_HANDLE_VALUE Then
    begin
    ZeroMemory(@devData, SizeOf(devData));
    devData.cbSize := SizeOf(devData);
    If SetupDiCreateDeviceInfoW(h, PWideChar(FDeviceId), FClassGuid, Nil, 0, DICD_GENERATE_ID, devData) Then
      begin
      If SetupDiSetDeviceRegistryPropertyW(h, devData, SPDRP_HARDWAREID, hwIds, hwIdsSize) Then
        begin
        If SetupDiCallClassInstaller(DIF_REGISTERDEVICE, h, devData) Then
          begin
          If Not UpdateDriverForPlugAndPlayDevices(0, hwIds, PWideChar(FINFFileName), 0, Nil) Then
            FErrorCode := GetLastError;
          end;
        end
      Else FErrorCode := GetLastError;
      end
    Else FErrorCode := GetLastError;
    end
  Else FErrorCode := GetLastError;

  FreeMem(hwIds);
  end
Else FErrorCode := ERROR_NOT_ENOUGH_MEMORY;
end;

Function TCreateDeviceInstallTask.GetDescription:WideString;
begin
Result := Format('Installing device %s (%s)', [FHardwareIds[0], FINFFileName]);
end;

Function TCreateDeviceInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Inherited CounterTask;
If Assigned(Result) Then
  Result.Free;

Result := TDeleteFileInstallTask.Create(InstallerSettings, FINFFileName);
end;



End.

