unit MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.ComCtrls, Vcl.StdCtrls;

type
  TForm1 = class(TForm)
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
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

end.
