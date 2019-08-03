program Installer;

uses
  Vcl.Forms,
  MainForm in 'MainForm.pas' {Form1},
  AbstractInstallerPage in 'AbstractInstallerPage.pas',
  LicenseInstallerPage in 'LicenseInstallerPage.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
