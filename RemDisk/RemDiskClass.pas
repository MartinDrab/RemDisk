Unit RemDiskClass;


Interface

Uses
  Windows, RemDiskDll, Generics.Collections, Utils;

Type
  TRemDisk = Class
  Private
    FDiskNumber : Cardinal;
    FDiskSize : UInt64;
    FFLags : Cardinal;
    FDiskType : EREMDiskType;
    FState : EREmDiskInfoState;
    FFIleName : WideString;
    Class Function GetUnusedDiskNumber(Var ANumber:Cardinal):Boolean;
  Public
    Constructor Create(Var ARecord:REMDISK_INFO); Reintroduce;

    Function ChangePassword(ANewPassword:WideString; AOldPassword:WideString):Cardinal;
    Function Encrypt(APassword:WideString; AFooter:Boolean):Cardinal;
    Function Decrypt(APassword:WideString):Cardinal;
    Function Refresh:Cardinal;
    Procedure RefreshFromRecord(Var ARecord:REMDISK_INFO);
    Procedure RefreshFromClass(AObject:TRemDisk);
    Function Save(AFileName:WideString):Cardinal;
    Function Remove:Cardinal;
    Function IsWritable:Boolean;
    Function IsEncrypted:Boolean;
    Function IsOffline:Boolean;

    Class Function Enumerate(AList:TList<TRemDisk>):Cardinal;
    Class Function Open(ADiskType:EREMDiskType; AFileName:WideString; AFlags:Cardinal; APassword:WideString):Cardinal;
    Class Function CreateRAMDisk(ASize:UInt64; AFlags:Cardinal; AFileName:WideString; APassword:WideString):Cardinal;
    Class Function CreateFileDisk(AFileName:WideString; AFileSize:UInt64; AFlags:Cardinal; APassword:WideString):Cardinal;
    Class Function StateToString(AState:EREmDiskInfoState):WideString;
    Class Function TypeToString(ADiskType:EREMDiskType):WideString;
    Class Function FlagsToString(AFlags:Cardinal):WideString;

    Property DiskNumber : Cardinal Read FDiskNumber;
    Property DiskType : EREMDiskType Read FDiskType;
    Property DiskSize : UInt64 Read FDiskSize;
    Property State : EREmDiskInfoState Read FState;
    Property Flags : Cardinal Read FFlags;
    Property FileName : WideString Read FFileName;
  end;


Implementation

Uses
  SysUtils;

Constructor TRemDisk.Create(Var ARecord:REMDISK_INFO);
begin
Inherited Create;
RefreshFromRecord(ARecord);
end;

Procedure TRemDisk.RefreshFromRecord(Var ARecord:REMDISK_INFO);
begin
FDiskNumber := ARecord.DiskNumber;
FDiskType := ARecord.DiskType;
FDiskSIze := ARecord.DiskSize;
FFlags := ARecord.Flags;
FState := ARecord.State;
FFileName := Copy(WideString(ARecord.FileName), 1, Length(WideString(ARecord.FileName)));
end;

Procedure TRemDisk.RefreshFromClass(AObject:TRemDisk);
begin
FDiskNumber := AObject.DiskNumber;
FDiskType := AObject.DiskType;
FDiskSIze := AObject.DiskSize;
FFlags := AObject.Flags;
FState := AObject.State;
FFileName := AObject.FileName;
end;

Function TRemDisk.ChangePassword(ANewPassword:WideString; AOldPassword:WideString):Cardinal;
begin
Result := DllRemDiskChangePassword(FDiskNumber, PWideChar(ANewPassword), Length(ANewPassword)*SizeOf(WideChar), PWideChar(AOldPassword), Length(AOldPassword)*SizeOf(WideChar));
end;

Function TRemDisk.Encrypt(APassword:WideString; AFooter:Boolean):Cardinal;
begin
Result := DLLRemDiskEncrypt(FDiskNumber, PWideChar(APassword), Length(APassword)*SizeOf(WideChar), AFooter);
end;

Function TRemDisk.Decrypt(APassword:WideString):Cardinal;
begin
Result := DLLRemDiskDecrypt(FDiskNumber, PWideChar(APassword), Length(APassword)*SizeOf(WideChar));
end;


Function TRemDisk.Refresh:Cardinal;
Var
  ri : REMDISK_INFO;
begin
Result := DllRemDiskGetInfo(FDiskNumber, ri);
If Result = ERROR_SUCCESS Then
  begin
  RefreshFromRecord(ri);
  DllRemDiskInfoFree(ri);
  end;
end;

Function TRemDisk.Save(AFileName:WideString):Cardinal;
begin
Result := DllRemDiskSaveFIle(FDiskNumber, PWideChar(AFileName), FILE_SHARE_READ);
end;

Function TRemDisk.Remove:Cardinal;
begin
Result := DllRemDiskFree(FDiskNumber);
end;

Function TRemDisk.IsWritable:Boolean;
begin
Result := ((FFlags And REMDISK_FLAG_WRITABLE) <> 0);
end;

Function TREmDisk.IsEncrypted:Boolean;
begin
Result := ((FFlags And REMDISK_FLAG_ENCRYPTED) <> 0);
end;

Function TRemDisk.IsOffline:Boolean;
begin
Result := ((FFlags And REMDISK_FLAG_OFFLINE) <> 0);
end;

(** Static routines **)

Class Function TRemDisk.Enumerate(AList:TList<TRemDisk>):Cardinal;
Var
  I : Integer;
  count : Cardinal;
  pri : PREMDISK_INFO;
  current : PREMDISK_INFO;
begin
Result := DllRemdiskEnumerate(pri, count);
If Result = ERROR_SUCCESS Then
  begin
  current := pri;
  For I := 0 To count - 1 Do
    begin
    AList.Add(TRemDisk.Create(current^));
    Inc(current);
    end;

  DllRemDiskEnumerationFree(pri, count);
  end;
end;

Class Function TRemDisk.Open(ADiskType:EREMDiskType; AFileName:WideString; AFlags:Cardinal; APassword:WideString):Cardinal;
Var
  diskNumber : Cardinal;
begin
If GetUnusedDiskNumber(diskNumber) Then
  Result := DllRemDiskOpen(diskNumber, ADiskType, PWideChar(AFileName), 0, AFlags, PWideChar(APassword), Length(APassword)*SizeOf(WideChar))
Else Result := ERROR_NO_MORE_ITEMS;
end;

Class Function TRemDisk.CreateRAMDisk(ASize:UInt64; AFlags:Cardinal; AFileName:WideString; APassword:WideString):Cardinal;
Var
  diskNumber : Cardinal;
begin
If GetUnusedDiskNumber(diskNumber) Then
  Result := DllRemDiskCreate(diskNumber, rdtRAMDisk, ASize, PWideChar(AFileName), 0, AFlags, PWideChar(APassword), Length(APassword)*SizeOf(WideChar))
Else Result := ERROR_NO_MORE_ITEMS;
end;


Class Function TRemDisk.CreateFileDisk(AFileName:WideString; AFileSize:UInt64; AFlags:Cardinal; APassword:WideString):Cardinal;
Var
  diskNumber : Cardinal;
begin
If GetUnusedDiskNumber(diskNumber) Then
  Result := DllRemDiskCreate(diskNumber, rdtFileDisk, AFileSize, PWideChar(AFileName), 0, AFlags, PWideChar(APassword), Length(APassword)*SizeOf(WideChar))
Else Result := ERROR_NO_MORE_ITEMS;
end;


Class Function TRemDisk.StateToString(AState:EREmDiskInfoState):WideString;
begin
Case AState Of
  rdisInitialized : Result := 'Initialized';
  rdisWorking : Result := 'Working';
  rdisEncrypting: Result := 'Encrypting';
  rdisDecrypting : Result := 'Decrypting';
  rdisLoading : Result := 'Loading';
  rdisSaving : Result := 'Saving';
  rdisPasswordChange : Result := 'Password';
  rdisRemoved : Result := 'Removed';
  rdisSurpriseRemoval : Result := 'SurpriseRemoval';
  end;
end;


Class Function TRemDisk.TypeToString(ADiskType:EREMDiskType):WideString;
begin
Case ADiskType Of
  rdtRAMDisk : Result := 'RAM';
  rdtFileDisk : Result := 'File';
  end;
end;


Class Function TRemDisk.FlagsToString(AFlags:Cardinal):WideString;
begin
Result := '';
If ((AFlags And REMDISK_FLAG_WRITABLE) = 0) Then
  Result := Result + 'ReadOnly ';

If ((AFlags And REMDISK_FLAG_ENCRYPTED) <> 0) Then
  Result := Result + 'Encrypted ';

If ((AFlags And REMDISK_FLAG_OFFLINE) <> 0) Then
  Result := Result + 'Offline ';

If ((AFlags And REMDISK_FLAG_FILE_SOURCE) <> 0) Then
  Result := Result + 'FileSource ';

If Length(Result) > 0 Then
  Delete(Result, Length(Result), 1);
end;


Class Function TRemDisk.GetUnusedDiskNumber(Var ANumber:Cardinal):Boolean;
Var
  I : Cardinal;
begin
Result := False;
For I := 16 To 255 Do
  begin
  Result := (Not SymbolicLinkExists(Format('\DosDevices\PhysicalDrive%u', [I])));
  If Result Then
    begin
    ANumber := I;
    Break;
    end;
  end;
end;


End.
