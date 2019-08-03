Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  Vcl.ExtCtrls, Vcl.ComCtrls, Vcl.StdCtrls,
  AbstractInstallerPage, LicenseInstallerPage;

Type
  TForm1 = Class (TForm)
    StepPanel: TPanel;
    PagesPageControl: TPageControl;
    ActionTabSheet: TTabSheet;
    SettingsTabSheet: TTabSheet;
    ProgressTabSheet: TTabSheet;
    SuccessTabSheet: TTabSheet;
    FailureTabSheet: TTabSheet;
    InitialTabSheet: TTabSheet;
    CancelButton: TButton;
    NextButton: TButton;
    BackButton: TButton;
    RepairRadioButton: TRadioButton;
    RemoveRadioButton: TRadioButton;
    DirectoryEdit: TEdit;
    BrowseButton: TButton;
    StartMenuCheckBox: TCheckBox;
    ShortcutCheckBox: TCheckBox;
    AllUsersCheckBox: TCheckBox;
    DirectoryOpenDialog: TOpenDialog;
    LocationLabel: TLabel;
    InstallProgressBar: TProgressBar;
    OperationListBox: TListBox;
    SuccessPanel: TPanel;
    FailurePanel: TPanel;
    WelcomePanel: TPanel;
    LicenseTabSheet: TTabSheet;
    AgreePanel: TPanel;
    AgreeSheckBox: TCheckBox;
    LicenseRichEdit: TRichEdit;
    procedure NextButtonClick(Sender: TObject);
    procedure CancelButtonClick(Sender: TObject);
    procedure BackButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure AgreeSheckBoxClick(Sender: TObject);
  Private
    FCurrentPage : TAbstractInstallerPage;

    FInitialPage : TAbstractInstallerPage;
    FActionPage : TAbstractInstallerPage;
    FLicensePage : TLicenseInstallerPage;
    FSettingsPage : TAbstractInstallerPage;
    FProgressPage : TAbstractInstallerPage;
    FSuccessPage : TAbstractInstallerPage;
    FFailurePage : TAbstractInstallerPage;

    Procedure OnPageSelectedChanged(APage:TAbstractInstallerPage);
  end;

Var
  Form1: TForm1;

Implementation

Uses
  InstallerSettings;

{$R *.DFM}

Procedure TForm1.AgreeSheckBoxClick(Sender: TObject);
begin
FLicensePage.Agree := (Sender As TCheckBox).Checked;
end;

Procedure TForm1.BackButtonClick(Sender: TObject);
begin
FCurrentPage.PreviousPressed;
end;

Procedure TForm1.CancelButtonClick(Sender: TObject);
begin
If MessageDlg('Do you really wish to exit the setup?', mtWarning, [mbYes, mbNo], 0, mbNo) = mrYes Then
  FCurrentPage.CancelPressed;
end;

Procedure TForm1.FormCreate(Sender: TObject);
Var
  instSettings : TInstallerSettings;
begin
instSettings := TInstallerSettings.Create;
FInitialPage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, InitialTabSheet);
FActionPage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, ActionTabSheet);
FLicensePage := TLicenseInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, LicenseTabSheet);
FSettingsPage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, SettingsTabSheet);
FProgressPage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, ProgressTabSheet);
FSuccessPage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, SuccessTabSheet);
FFailurePage := TAbstractInstallerPage.Create(instSettings, OnPageSelectedChanged, OnPageSelectedChanged, FailureTabSheet);

FInitialPage.SetTargets(FActionPage, Nil, FFailurePage);
FActionPage.SetTargets(FLicensePage, FInitialPage, FFailurePage);
FLicensePage.SetTargets(FSettingsPage, FActionPage, FFailurePage);
FSettingsPage.SetTargets(FProgressPage, FLicensePage, FFailurePage);
FProgressPage.SetTargets(FSuccessPage, FSettingsPage, FFailurePage);

OnPageSelectedChanged(FInitialPage);
end;

Procedure TForm1.FormDestroy(Sender: TObject);
begin
FFailurePage.Free;
FSuccessPage.Free;
FProgressPage.Free;
FSettingsPage.Free;
FLicensePage.Free;
FActionPage.Free;
FInitialPage.Free;
end;

Procedure TForm1.NextButtonClick(Sender: TObject);
begin
FCurrentPage.NextPressed;
end;

Procedure TForm1.OnPageSelectedChanged(APage:TAbstractInstallerPage);
begin
FCurrentPage := APage;
PagesPageControl.ActivePage := TTabSheet(FCurrentPage.Context);
CancelButton.Caption := APage.CancelButton.Caption;
CancelButton.Enabled := APage.CancelButton.Enabled;
BackButton.Caption := APage.PreviousButton.Caption;
BackButton.Enabled := APage.PreviousButton.Enabled;
NextButton.Caption := APage.NextButton.Caption;
NextButton.Enabled := APage.NextButton.Enabled;
end;

End.

