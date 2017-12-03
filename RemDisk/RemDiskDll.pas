Unit RemDiskDll;

{$MINENUMSIZE 4}

Interface

Uses
  Windows;

Const
  REMDISK_FLAG_WRITABLE						 = $1;
  REMDISK_FLAG_COPY_ON_WRITE			 = $2;
  REMDISK_FLAG_ALLOCATED					 = $4;
  REMDISK_FLAG_SAVE_ON_STOP				 = $8;
  REMDISK_FLAG_ENCRYPTED					 = $10;
  REMDISK_FLAG_OFFLINE						 = $20;
  REMDISK_FLAG_FILE_SOURCE				 = $80;
  REMDISK_FLAG_SPARSE_FILE         = $100;

 REMBUS_FLAG_SUPPORTS_PLAIN_RAM_DISKS			  = $1;
 REMBUS_FLAG_SUPPORTS_PLAIN_FILE_DISKS			= $2;
 REMBUS_FLAG_SUPPORTS_ENCRYPTED_RAM_DISKS		= $4;
 REMBUS_FLAG_SUPPORTS_ENCRYPTED_FILE_DISKS	= $8;
 REMBUS_FLAG_SUPPORTS_EF_RAM_DISKS				  = $10;
 REMBUS_FLAG_SUPPORTS_EF_FILE_DISKS				  = $20;

Type
  _REM_DRIVERS_INFO = Record
    Flags : Cardinal;
	  BusMajorVersion : Cardinal;
	  BusMinorVersion : Cardinal;
	  DiskMajorVersion : Cardinal;
	  DiskMinorVersion : Cardinal;
    end;
  REM_DRIVERS_INFO = _REM_DRIVERS_INFO;
  PREM_DRIVERS_INFO = ^REM_DRIVERS_INFO;

  _ERemBusConnectionMode = (rbcmReadOnly, rbcmReadWrite);
  ERemBusConnectionMode = _ERemBusConnectionMode;
  PERemBusConnectionMode = ^ERemBusConnectionMode;

  _EREmDiskInfoState = (
	  rdisInitialized,
	  rdisWorking,
  	rdisEncrypting,
	  rdisDecrypting,
	  rdisLoading,
	  rdisSaving,
	  rdisPasswordChange,
	  rdisRemoved,
    rdisSurpriseRemoval
  );
  EREmDiskInfoState = _EREmDiskInfoState;
  PEREmDiskInfoState = ^EREmDiskInfoState;

  _EREMDiskType = (rdtRAMDisk, rdtFileDisk);
  EREMDiskType = _EREMDiskType;
  PEREMDiskType = ^EREMDiskType;

  _REMDISK_INFO = Record
	  DiskNumber : Cardinal;
    DiskType : EREMDiskType;
	  DiskSize : UInt64;
	  Flags : Cardinal;
    State : EREmDiskInfoState;
	  FileName : PWideChar;
    end;
  REMDISK_INFO = _REMDISK_INFO;
  PREMDISK_INFO = ^REMDISK_INFO;



Function DllRemDiskInstall(ABusDriverINFPath:PWideChar):Cardinal; StdCall;

Function DllRemDiskCreate(ADiskNumber:Cardinal; ADiskType:EREMDiskType; ADiskSize:UInt64; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall;
Function DllRemDiskOpen(ADiskNumber:Cardinal; ADiskType:EREMDiskType; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:PWideChar; APasswordLength:NativeUInt):Cardinal; StdCall;
Function DllRemDiskChangePassword(ADiskNumber:Cardinal; ANewPassword:Pointer; ANewPasswordLength:NativeUInt; AOldPassword:Pointer; AOldPasswordLength:NativeUInt):Cardinal; StdCall;
Function DllRemDiskEncrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt; AFooter:ByteBool):Cardinal; StdCall;
Function DllRemDiskDecrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall;
Function DllRemDiskFree(ADiskNumber:Cardinal):Cardinal; StdCall;
Function DllRemDiskEnumerate(Var AArray:PREMDISK_INFO; Var ACount:Cardinal):Cardinal; StdCall;
Procedure DllRemDiskEnumerationFree(AArray:PREMDISK_INFO; ACount:Cardinal); StdCall;
Function DllRemDiskGetInfo(ADiskNumber:Cardinal; Var AInfo:REMDISK_INFO):Cardinal; StdCall;
Procedure DllRemDiskInfoFree(Var AInfo:REMDISK_INFO); StdCall;
Function DllRemDiskLoadFile(ADiskNumber:Cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall;
Function DllRemDiskSaveFile(ADiskNumber:cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall;

Function DllRemBusConnect(AMode:ERemBusConnectionMode; Var ADriversInfo:REM_DRIVERS_INFO):Cardinal; StdCall;
Procedure DllRemBusDisconnect; StdCall;


Implementation

Const
  LibraryName = 'remdisk.dll';

{$IFNDEF WIN32}

Function DllRemDiskInstall(ABusDriverINFPath:PWideChar):Cardinal; StdCall; External LibraryName;

Function DllRemDiskCreate(ADiskNumber:Cardinal; ADiskType:EREMDiskType; ADiskSize:UInt64; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName;
Function DllRemDiskOpen(ADiskNumber:Cardinal; ADiskType:EREMDiskType; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:PWideChar; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName;
Function DllRemDiskChangePassword(ADiskNumber:Cardinal; ANewPassword:Pointer; ANewPasswordLength:NativeUInt; AOldPassword:Pointer; AOldPasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName;
Function DllRemDiskEncrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt; AFooter:ByteBool):Cardinal; StdCall; External LibraryName;
Function DllRemDiskDecrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName;
Function DllRemDiskFree(ADiskNumber:Cardinal):Cardinal; StdCall; External LibraryName;
Function DllRemDiskEnumerate(Var AArray:PREMDISK_INFO; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName;
Procedure DllRemDiskEnumerationFree(AArray:PREMDISK_INFO; ACount:Cardinal); StdCall; External LibraryName;
Function DllRemDiskGetInfo(ADiskNumber:Cardinal; Var AInfo:REMDISK_INFO):Cardinal; StdCall; External LibraryName;
Procedure DllRemDiskInfoFree(Var AInfo:REMDISK_INFO); StdCall; External LibraryName;
Function DllRemDiskLoadFile(ADiskNumber:Cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall; External LibraryName;
Function DllRemDiskSaveFile(ADiskNumber:cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall; External LibraryName;

Function DllRemBusConnect(AMode:ERemBusConnectionMode; Var ADriversInfo:REM_DRIVERS_INFO):Cardinal; StdCall; External LibraryName;
Procedure DllRemBusDisconnect; StdCall; External LibraryName;

{$ELSE}

Function DllRemDiskInstall(ABusDriverINFPath:PWideChar):Cardinal; StdCall; External LibraryName name '_DllRemDiskInstall@4';

Function DllRemDiskCreate(ADiskNumber:Cardinal; ADiskType:EREMDiskType; ADiskSize:UInt64; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName name '_DllRemDiskCreate@36';
Function DllRemDiskOpen(ADiskNumber:Cardinal; ADiskType:EREMDiskType; AFileName:PWideChar; AShareMode:Cardinal; AFlags:Cardinal; APassword:PWideChar; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName name '_DllRemDiskOpen@28';
Function DllRemDiskChangePassword(ADiskNumber:Cardinal; ANewPassword:Pointer; ANewPasswordLength:NativeUInt; AOldPassword:Pointer; AOldPasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName name '_DllRemDiskChangePassword@20';
Function DllRemDiskEncrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt; AFooter:ByteBool):Cardinal; StdCall; External LibraryName name '_DllRemDiskEncrypt@16';
Function DllRemDiskDecrypt(ADiskNumber:Cardinal; APassword:Pointer; APasswordLength:NativeUInt):Cardinal; StdCall; External LibraryName name '_DllRemDiskDecrypt@12';
Function DllRemDiskFree(ADiskNumber:Cardinal):Cardinal; StdCall; External LibraryName name '_DllRemDiskFree@4';
Function DllRemDiskEnumerate(Var AArray:PREMDISK_INFO; Var ACount:Cardinal):Cardinal; StdCall; External LibraryName name '_DllRemDiskEnumerate@8';
Procedure DllRemDiskEnumerationFree(AArray:PREMDISK_INFO; ACount:Cardinal); StdCall; External LibraryName name '_DllRemDiskEnumerationFree@8';
Function DllRemDiskGetInfo(ADiskNumber:Cardinal; Var AInfo:REMDISK_INFO):Cardinal; StdCall; External LibraryName name '_DllRemDiskGetInfo@8';
Procedure DllRemDiskInfoFree(Var AInfo:REMDISK_INFO); StdCall; External LibraryName name '_DllRemDiskInfoFree@4';
Function DllRemDiskLoadFile(ADiskNumber:Cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall; External LibraryName name '_DllRemDiskLoadFile@12';
Function DllRemDiskSaveFile(ADiskNumber:cardinal; AFileName:PWideChar; AShareMode:Cardinal):Cardinal; StdCall; External LibraryName name '_DllRemDiskSaveFile@12';

Function DllRemBusConnect(AMode:ERemBusConnectionMode; Var ADriversInfo:REM_DRIVERS_INFO):Cardinal; StdCall; External LibraryName name '_DllRemBusConnect@8';
Procedure DllRemBusDisconnect; StdCall; External LibraryName name '_DllRemBusDisconnect@0';

{$ENDIF}

End.
