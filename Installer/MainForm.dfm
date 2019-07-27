object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'RemDisk Installer'
  ClientHeight = 293
  ClientWidth = 393
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object StepPanel: TPanel
    Left = 0
    Top = 257
    Width = 393
    Height = 36
    Align = alBottom
    TabOrder = 0
    ExplicitTop = 208
    object CancelButton: TButton
      Left = 328
      Top = 2
      Width = 57
      Height = 31
      Caption = 'Cancel'
      TabOrder = 0
    end
    object NextButton: TButton
      Left = 265
      Top = 2
      Width = 57
      Height = 31
      Caption = 'Next >'
      TabOrder = 1
    end
    object BackButton: TButton
      Left = 202
      Top = 2
      Width = 57
      Height = 31
      Caption = '< Back'
      TabOrder = 2
    end
  end
  object PagesPageControl: TPageControl
    Left = 0
    Top = 0
    Width = 393
    Height = 257
    ActivePage = InitialTabSheet
    Align = alClient
    TabOrder = 1
    object ActionTabSheet: TTabSheet
      Caption = 'Action'
    end
    object SettingsTabSheet: TTabSheet
      Caption = 'Settings'
      ImageIndex = 1
    end
    object ProgressTabSheet: TTabSheet
      Caption = 'Progress'
      ImageIndex = 2
    end
    object SuccessTabSheet: TTabSheet
      Caption = 'Success'
      ImageIndex = 3
    end
    object FailureTabSheet: TTabSheet
      Caption = 'Failure'
      ImageIndex = 4
    end
    object InitialTabSheet: TTabSheet
      Caption = 'Initial'
      ImageIndex = 5
    end
  end
end
