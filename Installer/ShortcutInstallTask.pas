Unit ShortcutInstallTask;

Interface

Uses
  AbstractInstallTask;

Type
  EShortcutType = (
    stDesktop,
    stStartMenu,
    stQuickLaunch
  );

  TShortcutInstallTask = Class (TAbstractInstallTask)
  Public
    Procedure Execute; Override;
    Function GetDescription:WideString; Override;
    Function CounterTask:TAbstractInstallTask; Override;
  end;

Implementation

Uses
  SysUtils, Classes, ShellAPI, ShlObj, ComObj, ActiveX, Registry;

Procedure CreateShortcut(SourceFileName:WideString; AType:EShortcutType; SubDirectory:WideString);
Var
  MyObject: IUnknown;
  MySLink: IShellLink;
  MyPFile: IPersistFile;
  Directory,
  LinkName: Widestring;
  WFileName: WideString;
  MyReg, QuickLaunchReg: TRegIniFile;
begin
MyObject := CreateComObject(CLSID_ShellLink);
MySLink := MyObject As IShellLink;
MyPFile := MyObject As IPersistFile;
MySLink.SetPath(PWideChar(SourceFileName));
MySLink.SetWorkingDirectory(PWideChar(ExtractFileDir(SourceFileName)));
MySLink.SetIconLocation(PWideChar(SourceFileName), 0);
MyReg := TRegIniFile.Create('Software\MicroSoft\Windows\Cur rentVersion\Explorer');
Try
  LinkName := ExtractFileName(ChangeFileExt(SourceFileName, '.lnk'));
  Case AType Of
    stDesktop : Directory := MyReg.ReadString('Shell Folders', 'Desktop', '');
    stStartMenu : Directory := MyReg.ReadString('Shell Folders', 'Start Menu', '');
    stQuickLaunch : begin
      QuickLaunchReg := TRegIniFile.Create('Software\MicroSoft\Windows\Cur rentVersion\GrpConv');
      Try
        Directory := QuickLaunchReg.ReadString('MapGroups', 'Quick Launch', '');
      Finally
        QuickLaunchReg.Free;
        end;
      end;
    end;

  If Directory <> '' Then
    begin
    If (SubDirectory <> '') Then
      WFileName := Directory + '\' + SubDirectory + '\' + LinkName
    Else WFileName := Directory + '\' + LinkName;

    MyPFile.Save(PWideChar(WFileName), False);
    end;
Finally
  MyReg.Free;
  end;

MyObject._Release;
end;

Procedure TShortcutInstallTask.Execute;
begin
end;

Function TShortcutInstallTask.GetDescription:WideString;
begin
end;

Function TShortcutInstallTask.CounterTask:TAbstractInstallTask;
begin
end;



End.
