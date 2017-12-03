object NewRemDiskFrm: TNewRemDiskFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'New REM Disk'
  ClientHeight = 286
  ClientWidth = 322
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
    Left = 0
    Top = 0
    Width = 321
    Height = 239
    TabOrder = 0
    object EncryptionGroupBox: TGroupBox
      Left = 1
      Top = 121
      Width = 319
      Height = 118
      Align = alTop
      Caption = 'Encryption'
      TabOrder = 0
      object Label5: TLabel
        Left = 3
        Top = 43
        Width = 46
        Height = 13
        Caption = 'Password'
      end
      object Label6: TLabel
        Left = 3
        Top = 75
        Width = 37
        Height = 13
        Caption = 'Confirm'
      end
      object EncryptedCheckBox: TCheckBox
        Left = 52
        Top = 16
        Width = 69
        Height = 17
        Caption = 'Encrypted'
        TabOrder = 0
        OnClick = EncryptedCheckBoxClick
      end
      object Password1Edit: TEdit
        Left = 52
        Top = 39
        Width = 157
        Height = 21
        PasswordChar = '*'
        TabOrder = 1
      end
      object Password2Edit: TEdit
        Left = 52
        Top = 66
        Width = 157
        Height = 21
        PasswordChar = '*'
        TabOrder = 2
      end
      object ShowCharactersCheckBox: TCheckBox
        Left = 52
        Top = 98
        Width = 109
        Height = 17
        Caption = 'Show characters'
        TabOrder = 3
        OnClick = ShowCharactersCheckBoxClick
      end
    end
    object GeneralGroupBox: TGroupBox
      Left = 1
      Top = 1
      Width = 319
      Height = 120
      Align = alTop
      Caption = 'General'
      TabOrder = 1
      object Label1: TLabel
        Left = 3
        Top = 19
        Width = 24
        Height = 13
        Caption = 'Type'
      end
      object Label2: TLabel
        Left = 3
        Top = 43
        Width = 19
        Height = 13
        Caption = 'Size'
      end
      object Label3: TLabel
        Left = 3
        Top = 67
        Width = 45
        Height = 13
        Caption = 'File name'
      end
      object Label4: TLabel
        Left = 127
        Top = 43
        Width = 14
        Height = 13
        Caption = 'MB'
      end
      object DiskTypeComboBox: TComboBox
        Left = 56
        Top = 16
        Width = 89
        Height = 21
        Style = csDropDownList
        DoubleBuffered = True
        ItemIndex = 0
        ParentDoubleBuffered = False
        TabOrder = 0
        Text = 'RAM disk'
        OnChange = DiskTypeComboBoxChange
        Items.Strings = (
          'RAM disk'
          'File-backed disk')
      end
      object DiskSizeEdit: TEdit
        Left = 56
        Top = 40
        Width = 65
        Height = 21
        TabOrder = 1
        Text = '128'
      end
      object FileNameEdit: TEdit
        Left = 56
        Top = 64
        Width = 177
        Height = 21
        TabOrder = 2
      end
      object WritableCheckBox: TCheckBox
        Left = 56
        Top = 96
        Width = 89
        Height = 17
        Caption = 'Writable'
        Checked = True
        State = cbChecked
        TabOrder = 3
      end
      object BrowseButton: TButton
        Left = 239
        Top = 64
        Width = 57
        Height = 21
        Caption = 'Browse...'
        TabOrder = 4
        OnClick = BrowseButtonClick
      end
      object SparseFIleCheckBox: TCheckBox
        Left = 127
        Top = 94
        Width = 74
        Height = 21
        Caption = 'Sparse file'
        TabOrder = 5
      end
    end
  end
  object StornoButton: TButton
    Left = 264
    Top = 245
    Width = 57
    Height = 33
    Caption = 'Storno'
    TabOrder = 1
    OnClick = StornoButtonClick
  end
  object OkButton: TButton
    Left = 201
    Top = 245
    Width = 57
    Height = 33
    Caption = 'Ok'
    TabOrder = 2
    OnClick = OkButtonClick
  end
  object FilenameOpenDialog: TOpenDialog
    Options = [ofNoValidate, ofEnableSizing]
    Left = 256
    Top = 16
  end
end
