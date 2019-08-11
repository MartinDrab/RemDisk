program Installer;

uses
  Vcl.Forms,
  MainForm in 'MainForm.pas' {Form1},
  AbstractInstallerPage in 'AbstractInstallerPage.pas',
  LicenseInstallerPage in 'LicenseInstallerPage.pas',
  InstallerSettings in 'InstallerSettings.pas',
  SettingsInstallerPage in 'SettingsInstallerPage.pas',
  ProgressInstallerPage in 'ProgressInstallerPage.pas',
  InstallTaskThread in 'InstallTaskThread.pas',
  AbstractInstallTask in 'AbstractInstallTask.pas',
  ShortcutInstallTask in 'ShortcutInstallTask.pas',
  DeleteInstallTask in 'DeleteInstallTask.pas',
  CopyFileInstallTask in 'CopyFileInstallTask.pas',
  CreateDirInstallTask in 'CreateDirInstallTask.pas',
  ResExtractInstallTask in 'ResExtractInstallTask.pas',
  CreateDeviceInstallTask in 'CreateDeviceInstallTask.pas',
  SetupApi in 'SetupApi.pas',
  InstallINFInstallTask in 'InstallINFInstallTask.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
