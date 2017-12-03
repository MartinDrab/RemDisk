Unit NewRemDiskForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics, RemDiskDll,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls, Vcl.StdCtrls;

Type
  TNewRemDiskFrm = Class(TForm)
    MainPanel: TPanel;
    StornoButton: TButton;
    OkButton: TButton;
    EncryptionGroupBox: TGroupBox;
    GeneralGroupBox: TGroupBox;
    DiskTypeComboBox: TComboBox;
    DiskSizeEdit: TEdit;
    FileNameEdit: TEdit;
    WritableCheckBox: TCheckBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    BrowseButton: TButton;
    EncryptedCheckBox: TCheckBox;
    Password1Edit: TEdit;
    Password2Edit: TEdit;
    ShowCharactersCheckBox: TCheckBox;
    Label5: TLabel;
    Label6: TLabel;
    FilenameOpenDialog: TOpenDialog;
    SparseFIleCheckBox: TCheckBox;
    procedure EncryptedCheckBoxClick(Sender: TObject);
    procedure BrowseButtonClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure DiskTypeComboBoxChange(Sender: TObject);
    procedure StornoButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
    procedure ShowCharactersCheckBoxClick(Sender: TObject);
  Private
    FCreateFlags : Cardinal;
    FSparseFile : Boolean;
    FDiskSize : UInt64;
    FFileName : WideString;
    FPassword : WideString;
    FDiskType : ERemDiskType;
    FCancelled : Boolean;
  Public
    Property Cancelled : Boolean Read FCancelled;
    Property DiskSize : UInt64 Read FDisKSize;
    Property Disktype : EREMDiskType Read FDiskType;
    Property FileName : WideString Read FFileName;
    Property Password : WideString Read FPassword;
    Property SparseFile : Boolean Read FSparseFIle;
    Property CreateFlags : Cardinal Read FCreateFlags;
  end;


Implementation

{$R *.DFM}


Procedure TNewRemDiskFrm.BrowseButtonClick(Sender: TObject);
begin
If FileNameOpenDialog.Execute Then
  FileNameEdit.Text := FIleNameOpenDialog.FileName;
end;

Procedure TNewRemDiskFrm.DiskTypeComboBoxChange(Sender: TObject);
begin
SparseFIleCheckBox.Enabled := (EREMDiskType(DiskTypeComboBox.ItemIndex) = rdtFileDisk);
end;

Procedure TNewRemDiskFrm.EncryptedCheckBoxClick(Sender: TObject);
begin
If EncryptedCheckBox.Checked Then
  begin
  Password1Edit.ReadOnly := False;
  Password1Edit.Color := clWindow;
  Password2Edit.ReadOnly := False;
  Password2Edit.Color := clWindow;
  ShowCharactersCheckBox.Enabled := True;
  end
Else begin
  Password1Edit.ReadOnly := True;
  Password1Edit.Color := clBtnFace;
  Password2Edit.ReadOnly := True;
  Password2Edit.Color := clBtnFace;
  ShowCharactersCheckBox.Enabled := False;
  end;
end;

Procedure TNewRemDiskFrm.FormCreate(Sender: TObject);
begin
FCancelled := True;
EncryptedCheckBoxClick(EncryptedCheckBox);
DiskTypeComboBoxChange(DiskTypeComboBox);
end;

Procedure TNewRemDiskFrm.OkButtonClick(Sender: TObject);
begin
If (Not EncryptedCheckBox.Checked) Or
   ((Password1Edit.Text <> '') And (Password1Edit.Text = Password2Edit.Text)) Then
  begin
  FCancelled := False;
  FDiskType := EREMDiskType(DiskTypeComboBox.ItemIndex);
  FDiskSize := StrToInt64(DiskSizeEdit.Text)*1024*1024;
  FFileName := FileNameEdit.Text;
  FCreateFlags := 0;
  If WritableCheckBox.Checked Then
    FCreateFlags := (FCreateFlags Or REMDISK_FLAG_WRITABLE);

  If EncryptedCheckBox.Checked Then
    FCreateFlags := (FCreateFlags Or REMDISK_FLAG_ENCRYPTED);

  If SparseFileCheckBox.Checked Then
    FCreateFlags := (FCreateFlags Or REMDISK_FLAG_SPARSE_FILE);

  FPassword := Password1Edit.Text;
  FSparseFile := SparseFileCheckBox.Checked;
  Close;
  end;
end;

Procedure TNewRemDiskFrm.ShowCharactersCheckBoxClick(Sender: TObject);
begin
If ShowCharactersCheckBox.Checked Then
  begin
  Password1Edit.PasswordChar := #0;
  Password2Edit.PasswordChar := #0;
  end
Else begin
  Password1Edit.PasswordChar := '*';
  Password2Edit.PasswordChar := '*';
  end;
end;

Procedure TNewRemDiskFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;



End.
