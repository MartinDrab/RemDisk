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
