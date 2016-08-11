Unit OpenREMDiskForm;

Interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, RemDiskDll,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls;

Type
  TOpenREMDiskFrm = Class (TForm)
    MainPanel: TPanel;
    EncryptionGroupBox: TGroupBox;
    GeneralGroupBox: TGroupBox;
    FileNameEdit: TEdit;
    BrowseButton: TButton;
    LoadAsRAMDiskCheckBox: TCheckBox;
    ReadOnlyCheckBox: TCheckBox;
    EncryptedCheckBox: TCheckBox;
    EncryptedFooterCheckBox: TCheckBox;
    PasswordEdit: TEdit;
    ShowCharactersCheckBox: TCheckBox;
    StornoButton: TButton;
    OkButton: TButton;
    Label1: TLabel;
    Label2: TLabel;
    FileNameOpenDialog: TOpenDialog;
    Procedure BrowseButtonClick(Sender: TObject);
    Procedure StornoButtonClick(Sender: TObject);
    Procedure OkButtonClick(Sender: TObject);
    Procedure ShowCharactersCheckBoxClick(Sender: TObject);
    Procedure EncryptedCheckBoxClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  Private
    FCancelled : Boolean;
    FFileName : WideString;
    FFlags : Cardinal;
    FDiskType : EREMDiskType;
    FPassword : WideString;
  Public
    Property Cancelled : Boolean Read FCancelled;
    Property FileName : WideString Read FFileName;
    Property Flags : Cardinal Read FFlags;
    Property DiskType : EREMDiskType Read FDiskType;
    Property Password : WideString Read FPassword;
  end;


Implementation

{$R *.dfm}

Procedure TOpenREMDiskFrm.BrowseButtonClick(Sender: TObject);
begin
If FileNameOpenDialog.Execute Then
  FileNameEdit.Text := FileNameOpenDialog.FileName;
end;


Procedure TOpenREMDiskFrm.EncryptedCheckBoxClick(Sender: TObject);
begin
If EncryptedCheckBox.Checked Then
  begin
  EncryptedFooterCheckBox.Enabled := True;
  ShowCharactersCheckBox.Enabled := True;
  PasswordEdit.ReadOnly := False;
  PasswordEdit.Color := clWindow;
  end
Else begin
  EncryptedFooterCheckBox.Enabled := False;
  ShowCharactersCheckBox.Enabled := False;
  PasswordEdit.ReadOnly := True;
  PasswordEdit.Color := clBtnFace;
  end;
end;

procedure TOpenREMDiskFrm.FormCreate(Sender: TObject);
begin
FCancelled := True;
EncryptedCheckBoxClick(EncryptedCheckBox);
ShowCharactersCheckBoxClick(ShowCharactersCheckBox);
end;

Procedure TOpenREMDiskFrm.OkButtonClick(Sender: TObject);
begin
If (FileNameEdit.Text <> '') And
   ((Not EncryptedCheckBox.Checked) Or (PasswordEdit.Text <> '')) Then
  begin
  FCancelled := False;
  FFilename := FileNameEdit.Text;
  FDiskType := rdtFileDisk;
  If LoadASRAMDiskCheckBox.Checked Then
    FDiskType := rdtRAMDisk;

  FFlags := REMDISK_FLAG_FILE_SOURCE;
  If Not ReadOnlyCheckBox.Checked Then
    FFlags := (FFlags Or REMDISK_FLAG_WRITABLE);

  If EncryptedCheckBox.Checked Then
    begin
    FFlags := (FFlags Or REMDISK_FLAG_ENCRYPTED);
    If EncryptedFooterCheckBox.Checked Then
      FFlags := (FFlags Or REMDISK_FLAG_ENCRYPTED_FOOTER);
    end;

  FPassword := PasswordEdit.Text;
  Close;
  end;
end;

Procedure TOpenREMDiskFrm.ShowCharactersCheckBoxClick(Sender: TObject);
begin
If ShowCharactersCheckBox.Checked Then
  PasswordEdit.PasswordChar := #0
Else PasswordEdit.PasswordChar := '*';
end;

Procedure TOpenREMDiskFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;

End.
