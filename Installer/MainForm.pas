Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  Vcl.ExtCtrls, Vcl.ComCtrls, Vcl.StdCtrls,
  AbstractInstallerPage;

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
  Private
    Procedure OnPageSelectedChanged(APage:TAbstractInstallerPage);
  end;

Var
  Form1: TForm1;

Implementation

{$R *.DFM}

Procedure TForm1.OnPageSelectedChanged(APage:TAbstractInstallerPage);
begin
CancelButton.Caption := APage.CancelButton.Caption;
CancelButton.Enabled := APage.CancelButton.Enabled;
BackButton.Caption := APage.PreviousButton.Caption;
BackButton.Enabled := APage.PreviousButton.Enabled;
NextButton.Caption := APage.NextButton.Caption;
NextButton.Enabled := APage.NextButton.Enabled;
end;

End.

