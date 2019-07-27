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
      object RepairRadioButton: TRadioButton
        Left = 32
        Top = 17
        Width = 81
        Height = 15
        Caption = 'Repair'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 0
      end
      object RemoveRadioButton: TRadioButton
        Left = 32
        Top = 105
        Width = 81
        Height = 15
        Caption = 'Remove'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 1
      end
    end
    object SettingsTabSheet: TTabSheet
      Caption = 'Settings'
      ImageIndex = 1
      object LocationLabel: TLabel
        Left = 18
        Top = 67
        Width = 40
        Height = 13
        Caption = 'Location'
      end
      object DirectoryEdit: TEdit
        Left = 64
        Top = 64
        Width = 209
        Height = 21
        TabOrder = 0
      end
      object BrowseButton: TButton
        Left = 288
        Top = 64
        Width = 65
        Height = 21
        Caption = 'Browse...'
        TabOrder = 1
      end
      object StartMenuCheckBox: TCheckBox
        Left = 64
        Top = 96
        Width = 161
        Height = 25
        Caption = 'Create entry in Start Menu'
        TabOrder = 2
      end
      object ShortcutCheckBox: TCheckBox
        Left = 64
        Top = 120
        Width = 161
        Height = 25
        Caption = 'Create shortcut on Desktop'
        TabOrder = 3
      end
      object AllUsersCheckBox: TCheckBox
        Left = 64
        Top = 144
        Width = 161
        Height = 25
        Caption = 'Install for all users'
        TabOrder = 4
      end
    end
    object ProgressTabSheet: TTabSheet
      Caption = 'Progress'
      ImageIndex = 2
      object InstallProgressBar: TProgressBar
        Left = 0
        Top = 0
        Width = 385
        Height = 25
        Align = alTop
        TabOrder = 0
        ExplicitLeft = 80
        ExplicitTop = 56
        ExplicitWidth = 121
      end
      object OperationListBox: TListBox
        Left = 0
        Top = 25
        Width = 385
        Height = 204
        Align = alClient
        ItemHeight = 13
        TabOrder = 1
        ExplicitLeft = 72
        ExplicitTop = 72
        ExplicitWidth = 145
        ExplicitHeight = 73
      end
    end
    object SuccessTabSheet: TTabSheet
      Caption = 'Success'
      ImageIndex = 3
      object SuccessPanel: TPanel
        Left = 0
        Top = 0
        Width = 385
        Height = 229
        Align = alClient
        Caption = 'Success'
        TabOrder = 0
        ExplicitLeft = 80
        ExplicitTop = 88
        ExplicitWidth = 113
        ExplicitHeight = 89
      end
    end
    object FailureTabSheet: TTabSheet
      Caption = 'Failure'
      ImageIndex = 4
      object FailurePanel: TPanel
        Left = 0
        Top = 0
        Width = 385
        Height = 229
        Align = alClient
        Caption = 'Failure'
        TabOrder = 0
        ExplicitLeft = 80
        ExplicitTop = 88
        ExplicitWidth = 113
        ExplicitHeight = 89
      end
    end
    object InitialTabSheet: TTabSheet
      Caption = 'Initial'
      ImageIndex = 5
      object WelcomePanel: TPanel
        Left = 0
        Top = 0
        Width = 385
        Height = 229
        Align = alClient
        Caption = 'Welcome'
        TabOrder = 0
        ExplicitLeft = 112
        ExplicitTop = 72
        ExplicitWidth = 137
        ExplicitHeight = 89
      end
    end
  end
  object DirectoryOpenDialog: TOpenDialog
    Left = 252
    Top = 136
  end
end
