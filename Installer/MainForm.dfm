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
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object StepPanel: TPanel
    Left = 0
    Top = 257
    Width = 393
    Height = 36
    Align = alBottom
    TabOrder = 0
    object CancelButton: TButton
      Left = 328
      Top = 2
      Width = 57
      Height = 31
      Caption = 'Cancel'
      TabOrder = 0
      OnClick = CancelButtonClick
    end
    object NextButton: TButton
      Left = 265
      Top = 2
      Width = 57
      Height = 31
      Caption = 'Next >'
      TabOrder = 1
      OnClick = NextButtonClick
    end
    object BackButton: TButton
      Left = 202
      Top = 2
      Width = 57
      Height = 31
      Caption = '< Back'
      TabOrder = 2
      OnClick = BackButtonClick
    end
  end
  object PagesPageControl: TPageControl
    Left = 0
    Top = 0
    Width = 393
    Height = 257
    ActivePage = SettingsTabSheet
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
        OnClick = BrowseButtonClick
      end
      object StartMenuCheckBox: TCheckBox
        Left = 64
        Top = 96
        Width = 161
        Height = 25
        Caption = 'Create entry in Start Menu'
        TabOrder = 2
        OnClick = StartMenuCheckBoxClick
      end
      object ShortcutCheckBox: TCheckBox
        Left = 64
        Top = 120
        Width = 161
        Height = 25
        Caption = 'Create shortcut on Desktop'
        TabOrder = 3
        OnClick = ShortcutCheckBoxClick
      end
      object AllUsersCheckBox: TCheckBox
        Left = 64
        Top = 144
        Width = 161
        Height = 25
        Caption = 'Install for all users'
        TabOrder = 4
        OnClick = AllUsersCheckBoxClick
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
      end
      object OperationListBox: TListBox
        Left = 0
        Top = 25
        Width = 385
        Height = 204
        Align = alClient
        ItemHeight = 13
        TabOrder = 1
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
      end
    end
    object LicenseTabSheet: TTabSheet
      Caption = 'License'
      ImageIndex = 6
      object AgreePanel: TPanel
        Left = 0
        Top = 200
        Width = 385
        Height = 29
        Align = alBottom
        TabOrder = 0
        object AgreeSheckBox: TCheckBox
          Left = 8
          Top = 2
          Width = 89
          Height = 17
          Caption = 'I agree'
          TabOrder = 0
          OnClick = AgreeSheckBoxClick
        end
      end
      object LicenseRichEdit: TRichEdit
        Left = 0
        Top = 0
        Width = 385
        Height = 200
        Align = alClient
        Font.Charset = EASTEUROPE_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        Lines.Strings = (
          'MIT License'
          ''
          'Copyright (c) 2016-2019 Martin Dr'#225'b'
          ''
          
            'Permission is hereby granted, free of charge, to any person obta' +
            'ining a '
          'copy'
          
            'of this software and associated documentation files (the "Softwa' +
            're"), to '
          'deal'
          
            'in the Software without restriction, including without limitatio' +
            'n the rights'
          
            'to use, copy, modify, merge, publish, distribute, sublicense, an' +
            'd/or sell'
          
            'copies of the Software, and to permit persons to whom the Softwa' +
            're is'
          'furnished to do so, subject to the following conditions:'
          ''
          
            'The above copyright notice and this permission notice shall be i' +
            'ncluded in all'
          'copies or substantial portions of the Software.'
          ''
          'THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY '
          'KIND, EXPRESS OR'
          'IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF '
          'MERCHANTABILITY,'
          'FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO '
          'EVENT SHALL THE'
          'AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, '
          'DAMAGES OR OTHER'
          'LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR '
          'OTHERWISE, ARISING FROM,'
          'OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR '
          'OTHER DEALINGS IN THE'
          'SOFTWARE.')
        ParentFont = False
        PlainText = True
        ReadOnly = True
        ScrollBars = ssVertical
        TabOrder = 1
        Zoom = 100
      end
    end
  end
  object DirectorySaveDialog: TSaveDialog
    Options = [ofHideReadOnly, ofNoTestFileCreate, ofNoNetworkButton, ofEnableSizing, ofDontAddToRecent]
    Left = 260
    Top = 144
  end
end
