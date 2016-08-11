Unit Utils;

Interface

Uses
  Windows;


Function SymbolicLinkExists(ALinkName:WideString):Boolean;
Procedure WinErrorMessage(AMsg:WideString; AErrorCode:Cardinal);
Function GetDiskDriveLetters(ADiskNumber:Cardinal):WideString;

Implementation

Uses
  SysUtils;

Const
  SYMBOLIC_LINK_QUERY = $1;
  OBJ_CASE_INSENSITIVE = $40;

Type
  UNICODE_STRING = Record
    Lenth : Word;
    MaximumLength : Word;
    Buffer : PWideChar;
    end;
  PUNICODE_STRING = ^UNICODE_STRING;

  OBJECT_ATTRIBUTES = Record
    Length : Cardinal;
    RootDirectory : THandle;
    ObjectName : PUNICODE_STRING;
    Attributes : Cardinal;
    SecurityDescriptor : Pointer;
    SecurityQualityOfService : Pointer;
    end;
  POBJECT_ATTRIBUTES = ^OBJECT_ATTRIBUTES;

  _DISK_EXTENT = Record
    DiskNumber : Cardinal;
    StartingOffset : LARGE_INTEGER;
    ExtentLength : LARGE_INTEGER;
    end;
  DISK_EXTENT = _DISK_EXTENT;
  PDISK_EXTENT = ^DISK_EXTENT;

  _VOLUME_DISK_EXTENTS = Record
    NumberOfDiskExtents : Cardinal;
    Extents : Array [0..0] Of DISK_EXTENT;
    end;
  VOLUME_DISK_EXTENTS = _VOLUME_DISK_EXTENTS;
  PVOLUME_DISK_EXTENTS = ^VOLUME_DISK_EXTENTS;

Function NtOpenSymbolicLinkObject(Var LinkHandle:THandle; DesiredAccess:Cardinal; Var ObjectAttributes:OBJECT_ATTRIBUTES):Cardinal; StdCall; External 'ntdll.dll';
Function NtClose(ObjectHandle:THandle):Cardinal; StdCall; External 'ntdll.dll';
Procedure RtlInitUnicodeString(Var UnicodeString:UNICODE_STRING; WStr:PWideChar); StdCall; External 'ntdll.dll';


Procedure WinErrorMessage(AMsg:WideString; AErrorCode:Cardinal);
Var
  msg : WideString;
begin
msg := Format('%s'#13#10'Error code: %u'#13#10'Error message: %s', [AMsg, AErrorCode, SysErrorMessage(AErrorCode)]);
MessageBoxW(0, PWideChar(msg), 'Error', MB_OK Or MB_ICONERROR);
end;

Function SymbolicLinkExists(ALinkName:WideString):Boolean;
Var
  smHandle : THandle;
  uName : UNICODE_STRING;
  oa : OBJECT_ATTRIBUTES;
  status : Cardinal;
begin
RtlInitUnicodeString(uName, PWideChar(ALinkName));
oa.Length := SizeOf(oa);
oa.RootDirectory := 0;
oa.ObjectName := @uName;
oa.Attributes := OBJ_CASE_INSENSITIVE;
oa.SecurityDescriptor := Nil;
oa.SecurityQualityOfService := Nil;
status := NtOpenSymbolicLinkObject(smHandle, SYMBOLIC_LINK_QUERY, oa);
Result := (status < $80000000);
If Result Then
  NtClose(smHandle);
end;


Function GetDiskDriveLetters(ADiskNumber:Cardinal):WideString;
Var
  I : Integer;
  driveHandle : THandle;
  driveLetter : WideChar;
  drivePath : WideString;
  err : cardinal;
  returnedLength : Cardinal;
  Extents : PVOLUME_DISK_EXTENTS;
  ExtentsLen : Cardinal;
begin
Result := '';
For driveLetter := 'A' To 'Z' Do
  begin
  drivePath := '\\.\' + driveLetter + ':';
  driveHandle := CreateFIleW(PWideChar(drivePath), FILE_READ_ATTRIBUTES, FILE_SHARE_READ Or FILE_SHARE_WRITE, Nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  If driveHandle <> INVALID_HANDLE_VALUE Then
    begin
    ExtentsLen := 64;
    Repeat
    Extents := HeapAlloc(GetProcessHeap, 0, ExtentsLen);
    If Assigned(Extents) Then
      begin
      err := ERROR_SUCCESS;
      If Not DeviceIoControl(driveHandle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, Nil, 0, Extents, ExtentsLen, returnedLength, Nil) Then
        begin
        err := GetLastError;
        HeapFree(GetProcessHeap, 0, Extents);
        Inc(ExtentsLen, ExtentsLen);
        end;
      end
    Else  err := ERROR_NOT_ENOUGH_MEMORY;
    Until err <> ERROR_INSUFFICIENT_BUFFER;

    If err = ERROR_SUCCESS Then
      begin
      For I := 0 To Extents.NumberOfDiskExtents - 1 Do
        begin
        If Extents.Extents[I].DiskNumber = ADiskNumber Then
          Result := Result + driveLetter + ' ';
        end;

      HeapFree(GetProcessHeap, 0, Extents);
      end;

    CloseHandle(driveHandle);
    end;
  end;

If Result <> '' Then
  Delete(Result, Length(Result), 1);
end;



End.

