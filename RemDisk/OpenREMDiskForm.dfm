object OpenREMDiskFrm: TOpenREMDiskFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'Open / Mount Disk'
  ClientHeight = 284
  ClientWidth = 287
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object MainPanel: TPanel
    Left = 8
    Top = 0
    Width = 281
    Height = 241
    TabOrder = 0
    object EncryptionGroupBox: TGroupBox
      Left = 1
      Top = 105
      Width = 279
      Height = 120
      Align = alTop
      Caption = 'Encryption'
      TabOrder = 0
      object Label1: TLabel
        Left = 4
        Top = 65
        Width = 46
        Height = 13
        Caption = 'Password'
      end
      object EncryptedCheckBox: TCheckBox
        Left = 56
        Top = 19
        Width = 89
        Height = 17
        Caption = 'Encrypted'
        TabOrder = 0
        OnClick = EncryptedCheckBoxClick
      end
      object EncryptedFooterCheckBox: TCheckBox
        Left = 56
        Top = 42
        Width = 89
        Height = 17
        Caption = 'Encrypted footer'
        TabOrder = 1
      end
      object PasswordEdit: TEdit
        Left = 56
        Top = 64
        Width = 137
        Height = 21
        PasswordChar = '*'
        TabOrder = 2
      end
      object ShowCharactersCheckBox: TCheckBox
        Left = 56
        Top = 91
        Width = 121
        Height = 17
        Caption = 'Show characters'
        TabOrder = 3
        OnClick = ShowCharactersCheckBoxClick
      end
    end
    object GeneralGroupBox: TGroupBox
      Left = 1
      Top = 1
      Width = 279
      Height = 104
      Align = alTop
      Caption = 'General'
      TabOrder = 1
      object Label2: TLabel
        Left = 34
        Top = 27
        Width = 16
        Height = 13
        Caption = 'File'
      end
      object FileNameEdit: TEdit
        Left = 56
        Top = 24
        Width = 137
        Height = 21
        TabOrder = 0
      end
      object BrowseButton: TButton
        Left = 199
        Top = 24
        Width = 58
        Height = 21
        Caption = 'Browse...'
        TabOrder = 1
        OnClick = BrowseButtonClick
      end
      object LoadAsRAMDiskCheckBox: TCheckBox
        Left = 56
        Top = 51
        Width = 121
        Height = 17
        Caption = 'Load as a RAM disk'
        TabOrder = 2
      end
      object ReadOnlyCheckBox: TCheckBox
        Left = 56
        Top = 74
        Width = 89
        Height = 17
        Caption = 'Read only'
        TabOrder = 3
      end
    end
  end
  object StornoButton: TButton
    Left = 224
    Top = 247
    Width = 57
    Height = 34
    Caption = 'Storno'
    TabOrder = 1
    OnClick = StornoButtonClick
  end
  object OkButton: TButton
    Left = 161
    Top = 247
    Width = 57
    Height = 34
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
  object FileNameOpenDialog: TOpenDialog
    Options = [ofHideReadOnly, ofPathMustExist, ofFileMustExist, ofNoTestFileCreate, ofEnableSizing]
    Left = 16
    Top = 40
  end
end
