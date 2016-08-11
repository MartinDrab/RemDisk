program RemDisk;

uses
  Windows,
  Vcl.Forms,
  MainForm in 'MainForm.pas' {MainFrm},
  RemDiskDll in 'RemDiskDll.pas',
  RemDiskClass in 'RemDiskClass.pas',
  REMDiskListModel in 'REMDiskListModel.pas',
  NewRemDiskForm in 'NewRemDiskForm.pas' {NewRemDiskFrm},
  Utils in 'Utils.pas',
  OpenREMDiskForm in 'OpenREMDiskForm.pas' {OpenREMDiskFrm},
  RemDriversInfo in 'RemDriversInfo.pas',
  AboutForm in 'AboutForm.pas' {AboutBox},
  ListModel in 'ListModel.pas';

{$R *.RES}

Var
  driversInfo: REM_DRIVERS_INFO;
begin
If DllRemBusConnect(rbcmReadWrite, driversInfo) = ERROR_SUCCESS Then
  begin
  RemInfoInit(driversInfo);
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TMainFrm, MainFrm);
  Application.Run;
  DllRemBusDisconnect;
  end;
end.

