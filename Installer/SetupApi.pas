Unit SetupApi;

Interface

Uses
  Windows;

Const
  DICD_GENERATE_ID   = $1;
  SPDRP_HARDWAREID   = $1;
  DIF_REGISTERDEVICE = $19;
  INSTALLFLAG_FORCE  = $1;

Type
  HDEVINFO = THandle;

  _SP_DEVINFO_DATA = Record
    cbSize : Cardinal;
    ClassGuid : TGuid;
    DevInst : Cardinal;    // DEVINST handle
    Reserved : NativeUInt;
    end;
  SP_DEVINFO_DATA = _SP_DEVINFO_DATA;
  PSP_DEVINFO_DATA = ^SP_DEVINFO_DATA;


Function SetupDiCreateDeviceInfoList(Var AClassGuid:TGuid; hwndParent:HWND):HDEVINFO; StdCall;
Function SetupDiDestroyDeviceInfoList(ADevInfoSet:HDEVINFO):LongBool; StdCall;
Function SetupDiCreateDeviceInfoW(ADeviceInfoSet:HDEVINFO; ADeviceName:PWideChar; Var AClassGuid:TGuid; ADeviceDescription:PWideChar; AHwndParent:HWND; ACreationFlags:Cardinal; Var ADeviceInfoData:SP_DEVINFO_DATA):LongBool; StdCall;
Function SetupDiSetDeviceRegistryPropertyW(ADeviceInfoSet:HDEVINFO; Var ADeviceInfo:SP_DEVINFO_DATA; AProperty:Cardinal; APropertyBuffer:Pointer; APropertyBufferSize:Cardinal):LongBool; StdCall;
Function SetupDiCallClassInstaller(AInstallFunction:Cardinal; ADeviceInfoSet:HDEVINFO; Var ADevInfoData:SP_DEVINFO_DATA):LongBool; StdCall;
Function UpdateDriverForPlugAndPlayDevices(AHwndParent:HWND; AHardwareId:PWideChar; AFullInfPath:PWideChar; AInstallFlags:Cardinal; ARebootRequired:PLongBool):LongBool; StdCall;
Procedure InstallHinfSectionW(AWindow:HWND; AModuleHandle:THandle; ACommandLine:PWideChar; AShowCommand:Cardinal); StdCall;


Implementation

Const
  SetupApiDll = 'setupapi.dll';


Function SetupDiCreateDeviceInfoList(Var AClassGuid:TGuid; hwndParent:HWND):HDEVINFO; StdCall; External SetupApiDll;
Function SetupDiDestroyDeviceInfoList(ADevInfoSet:HDEVINFO):LongBool; StdCall; External SetupApiDll;
Function SetupDiCreateDeviceInfoW(ADeviceInfoSet:HDEVINFO; ADeviceName:PWideChar; Var AClassGuid:TGuid; ADeviceDescription:PWideChar; AHwndParent:HWND; ACreationFlags:Cardinal; Var ADeviceInfoData:SP_DEVINFO_DATA):LongBool; StdCall; External SetupApiDll;
Function SetupDiSetDeviceRegistryPropertyW(ADeviceInfoSet:HDEVINFO; Var ADeviceInfo:SP_DEVINFO_DATA; AProperty:Cardinal; APropertyBuffer:Pointer; APropertyBufferSize:Cardinal):LongBool; StdCall; External SetupApiDll;
Function SetupDiCallClassInstaller(AInstallFunction:Cardinal; ADeviceInfoSet:HDEVINFO; Var ADevInfoData:SP_DEVINFO_DATA):LongBool; StdCall; External SetupApiDll;
Function UpdateDriverForPlugAndPlayDevices(AHwndParent:HWND; AHardwareId:PWideChar; AFullInfPath:PWideChar; AInstallFlags:Cardinal; ARebootRequired:PLongBool):LongBool; StdCall; External SetupApiDll;
Procedure InstallHinfSectionW(AWindow:HWND; AModuleHandle:THandle; ACommandLine:PWideChar; AShowCommand:Cardinal); StdCall; External SetupApiDll;



End.
