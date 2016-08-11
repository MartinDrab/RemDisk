Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils,
  System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs,
  Vcl.ComCtrls, Vcl.Menus,
  ListModel, RemDiskListModel, RemDiskClass, Vcl.ImgList;

Type
  TMainFrm = Class(TForm)
    REMDiskListView: TListView;
    MainMenu1: TMainMenu;
    FileMenuItem: TMenuItem;
    CreateDiskMenuItem: TMenuItem;
    RefreshMenuItem: TMenuItem;
    N2: TMenuItem;
    Exit1: TMenuItem;
    DiskMenuItem: TMenuItem;
    HelpMenuItem: TMenuItem;
    AboutREMDiskManager1: TMenuItem;
    EncryptMenuItem: TMenuItem;
    DecryptMenuItem: TMenuItem;
    ChangePasswordMenuItem: TMenuItem;
    N3: TMenuItem;
    SaveMenuItem: TMenuItem;
    N4: TMenuItem;
    ForceRemoveMenuItem: TMenuItem;
    SaveDialog1: TSaveDialog;
    EncryptedFooterMenuItem: TMenuItem;
    OpenDiskMenuItem: TMenuItem;
    N5: TMenuItem;
    RemDiskPopupMenu: TPopupMenu;
    IconImageList: TImageList;
    ColumnsMenuItem: TMenuItem;
    Procedure Exit1Click(Sender: TObject);
    Procedure FormClose(Sender: TObject; var Action: TCloseAction);
    Procedure FormCreate(Sender: TObject);
    procedure CreateDiskMenuItemClick(Sender: TObject);
    procedure EncryptDecryptClick(Sender: TObject);
    procedure OpenDiskMenuItemClick(Sender: TObject);
    procedure DiskMenuItemClick(Sender: TObject);
    procedure AboutREMDiskManager1Click(Sender: TObject);
  Private
    FModel : TRemDiskListModel;
  end;

Var
  MainFrm: TMainFrm;

Implementation

{$R *.DFM}

Uses
  Utils, REMDiskDll, RemDriversInfo,
  AboutForm, NewREMDiskForm, OpenREMDiskForm;




Procedure TMainFrm.AboutREMDiskManager1Click(Sender: TObject);
begin
With TAboutBox.Create(Application) DO
  begin
  ShowModal;
  Free;
  end;
end;

Procedure TMainFrm.CreateDiskMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
begin
With TNewRemDiskFrm.Create(Application) Do
  begin
  ShowModal;
  If Not cancelled Then
    begin
    Case DiskType Of
      rdtRAMDisk : err := TRemDisk.CreateRAMDisk(DiskSize, CreateFlags, FileName, Password);
      rdtFileDisk : err := TRemDisk.CreateFileDisk(FileName, DiskSize, CreateFlags, Password);
      end;

    If err = ERROR_SUCCESS Then
      FModel.Update
    Else WinErrorMessage('Unable to create disk', err);
    end;

  Free;
  end;
end;

Procedure TMainFrm.DiskMenuItemClick(Sender: TObject);
Var
  rd : TRemDisk;

begin
rd := FModel.Selected;
If Assigned(rd) Then
  begin
  EncryptMenuItem.Enabled := (rd.State = rdisWorking) And (Not rd.IsEncrypted);
  DecryptMenuItem.Enabled := (rd.State = rdisWorking) And (rd.IsEncrypted);
  ChangePasswordMenuItem.Enabled := (rd.State = rdisWorking) And (rd.IsEncrypted);
  EncryptedFooterMenuItem.Enabled := (rd.State = rdisWorking) And (Not rd.IsEncrypted);
  ForceRemoveMenuItem.Enabled := True;
  SaveMenuItem.Enabled := (rd.State = rdisWorking);
  end
Else begin
  EncryptMenuItem.Enabled := False;
  DecryptMenuItem.Enabled := False;
  ChangePasswordMenuItem.Enabled := False;
  EncryptedFooterMenuItem.Enabled := False;
  ForceRemoveMenuItem.Enabled := False;
  SaveMenuItem.Enabled := False;
  end;

RemDiskPopupMenu.Items[0].Enabled := EncryptMenuItem.Enabled;
RemDiskPopupMenu.Items[1].Enabled := DecryptMenuItem.Enabled;
RemDiskPopupMenu.Items[2].Enabled := ChangePasswordMenuItem.Enabled;
RemDiskPopupMenu.Items[3].Enabled := EncryptedFooterMenuItem.Enabled;
RemDiskPopupMenu.Items[5].Enabled := SaveMenuItem.Enabled;
RemDiskPopupMenu.Items[7].Enabled := ForceRemoveMenuItem.Enabled;
end;

Procedure TMainFrm.EncryptDecryptClick(Sender: TObject);
Var
  err : Cardinal;
  M : TMenuItem;
  fileName : WideString;
  password : WideString;
  password2 : WIdeString;
  rd : TREMDisk;
begin
rd := FModel.Selected;
If Assigned(rd) Then
  begin
  M := (Sender As TMenuItem);
  Case M.tag Of
    3 : begin
      password := InputBox('Enter password', 'Enter password to encrypt the disk', '');
      If password <> '' Then
        begin
        err := rd.Encrypt(password, False);
        FModel.Update;
        end;
      end;
    4 : begin
      password := InputBox('Enter password', 'Enter password to decrypt the disk', '');
      If password <> '' Then
        begin
        err := rd.Decrypt(password);
        FModel.Update;
        end;
      end;
    5 : begin
      password := InputBox('Old Password', 'Enter the old password for the disk', '');
      If password <> '' Then
        begin
        password2 := InputBox('New Password', 'Enter a new password for the disk', '');
        If password2 <> '' Then
          begin
          err := rd.ChangePassword(password2, password);
          FModel.Update;
          end;
        end;
      end;
    6 : begin
      password := InputBox('Enter password', 'Enter password to encrypt the disk', '');
      If password <> '' Then
        begin
        err := rd.Encrypt(password, True);
        FModel.Update;
        end;
      end;
    7 : begin
      If SaveDialog1.Execute Then
        begin
        Filename := SaveDialog1.FileName;
        err := rd.Save(FIleName);
        FModel.Update;
        end;
      end;
    8 : begin
      err := rd.Remove;
      FModel.Update;
      end;
    9 : FModel.Update;
    end;
  end;
end;

Procedure TMainFrm.Exit1Click(Sender: TObject);
begin
Close;
end;

Procedure TMainFrm.FormClose(Sender: TObject; var Action: TCloseAction);
begin
FModel.Free;
end;

Procedure TMainFrm.FormCreate(Sender: TObject);
Var
  c : TListModelColumn;
  I : Integer;
  diskItem : TMenuItem;
  M : TMenuItem;
begin
For I := 3 To DiskMenuItem.Count - 1 Do
  begin
  diskItem := DiskMenuItem[I];
  M := TMenuItem.Create(RemDiskPopupMenu.Items);
  M.Tag := diskItem.Tag;
  M.Caption := diskItem.Caption;
  M.OnClick := diskMenuItem.OnClick;
  M.ImageIndex := diskMenuItem.ImageIndex;
  RemDiskPopupMenu.Items.Add(M);
  end;

FModel := TRemDiskListModel.Create(Nil);
FModel
  .ColumnAdd('#', 0)
  .ColumnAdd('Type', 1, False, 75)
  .ColumnAdd('Size', 2, False, 100)
  .ColumnAdd('Flags', 3, True)
  .ColumnAdd('State', 4, False, 75)
  .ColumnAdd('File', 5, True)
  .ColumnAdd('Drive letters', 6, False, 75);

FModel.CreateColumnsMenu(ColumnsMenuItem);
FModel.SetDisplayer(RemDiskListView);
end;

Procedure TMainFrm.OpenDiskMenuItemClick(Sender: TObject);
Var
  err : Cardinal;
begin
With TOpenREMDiskFrm.Create(Application) Do
  begin
  ShowModal;
  If Not cancelled Then
    begin
    err := TRemdisk.Open(DiskType, FileName, Flags, Password);
    If err <> ERROR_SUCCESS Then
      WinErrorMessage('Unable to open the disk', err);
    end;

  Free;
  end;
end;

End.

