Unit RemDriversInfo;

Interface

Uses
  RemDiskDll;

Procedure RemInfoInit(Var AInfo:REM_DRIVERS_INFO);
Function RemInfoPlainRAMDisksSupported:Boolean;
Function RemInfoPlainFileDisksSupported:Boolean;
Function RemInfoEncryptedRAMDisksSupported:Boolean;
Function RemInfoEncryptedFileDisksSupported:Boolean;
Function RemInfoEFRAMDisksSupported:Boolean;
Function RemInfoEFFileDisksSupported:Boolean;

Implementation

Var
  driversInfo : REM_DRIVERS_INFO;

Procedure RemInfoInit(Var AInfo:REM_DRIVERS_INFO);
begin
driversInfo := AInfo;
end;

Function RemInfoPlainRAMDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_PLAIN_RAM_DISKS) <> 0;
end;

Function RemInfoPlainFileDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_PLAIN_FILE_DISKS) <> 0;
end;

Function RemInfoEncryptedRAMDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_ENCRYPTED_RAM_DISKS) <> 0;
end;

Function RemInfoEncryptedFileDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_ENCRYPTED_FILE_DISKS) <> 0;
end;

Function RemInfoEFRAMDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_EF_RAM_DISKS) <> 0;
end;

Function RemInfoEFFileDisksSupported:Boolean;
begin
Result := (driversInfo.Flags And REMBUS_FLAG_SUPPORTS_EF_FILE_DISKS) <> 0;
end;


end.
