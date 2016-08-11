Unit REMDiskListModel;

Interface

Uses
  Windows, ComCtrls, ListModel,
  RemDiskDll, RemDiskClass, Generics.Collections;

Type
  TRemDiskListModel = Class (TListModel<TRemDisk>)
  Private
    FDiskList : TList<TRemDisk>;
    FDiskMap : TDictionary<Cardinal, TRemDisk>;
  Protected
    Function GetColumn(AItem:TRemDisk; ATag:NativeUInt):WideString; Override;
    Procedure FreeItem(AItem:TRemDisk); Override;
    Function _Item(AIndex:Integer):TRemDisk; Override;
    Function GetImageIndex(AItem:TRemDisk):Integer; Override;
  Public
    Constructor Create(ADisplayer:TListView); Reintroduce;
    Destructor Destroy; Override;

    Function RowCount:Cardinal; Override;
    Function Update:Cardinal; Override;
  end;

Implementation

Uses
  SysUtils, Utils;

Constructor TRemDiskListModel.Create(ADisplayer:TListView);
begin
FDiskList := TList<TRemDisk>.Create;
FDiskMap := TDictionary<Cardinal, TRemDisk>.Create;
Inherited Create(ADisplayer);
end;

Destructor TRemDiskListModel.Destroy;
Var
  rd : TRemDisk;
begin
FDiskMap.Free;
For rd In FDiskList Do
  FreeItem(rd);

FDiskList.Free;

Inherited Destroy;
end;

Procedure TRemDiskListModel.FreeItem(AItem:TRemDisk);
begin
AItem.Free;
end;


Function TRemDiskListModel.GetColumn(AItem:TRemDisk; ATag:NativeUInt):WideString;
begin
Case ATag Of
  0 : Result := '0x' + IntToHex(AItem.DiskNumber, 2);
  1 : Result := TRemDisk.TypeToString(AItem.DiskType);
  2 : Result := Format('%u MB', [AItem.DiskSize Div (1024*1024)]);
  3 : Result := TRemDisk.FlagsToString(AItem.Flags);
  4 : Result := TRemDisk.StateToString(AItem.State);
  5 : Result := AItem.FileName;
  6 : Result := GetDiskDriveLetters(AItem.DiskNumber);
  end;
end;

Function TRemDiskListModel.GetImageIndex(AItem:TRemDisk):Integer;
begin
Result := Inherited GetImageIndex(AItem);
Case AItem.DiskType Of
  rdtRAMDisk : Result := 8;
  rdtFileDisk : Result := 9;
  end;
end;

Function TRemDiskListModel.Update:Cardinal;
Var
  found : Boolean;
  I, J : Integer;
  rd : TRemDisk;
  rdMap : TRemDisk;
  l : TList<TRemDisk>;
begin
l := TList<TRemDisk>.Create;
Result := TRemDisk.Enumerate(l);
If Result = ERROR_SUCCESS Then
  begin
  I := 0;
  While (I < FDisklist.Count) Do
    begin
    rd := FDiskList[I];
    found := False;
    For J := 0 To l.Count - 1 Do
      begin
      found := l[J].DiskNumber = rd.DiskNumber;
      If found Then
        Break;
      end;

    If Not found Then
      begin
      FDiskList.Delete(I);
      FDiskMap.Remove(rd.DiskNumber);
      rd.Free;
      Continue;
      end;

    Inc(I);
    end;

  For rd In l Do
    begin
    If FDiskMap.TryGetValue(rd.DiskNumber, rdMap) Then
      begin
      rdMap.RefreshFromClass(rd);
      rd.Free;
      end
    Else begin
      FDiskMap.Add(rd.DiskNumber, rd);
      FDiskList.Add(rd);
      end;
    end;

  If Assigned(FDisplayer) Then
    begin
    Displayer.Items.Count := FDiskList.Count;
    Displayer.Invalidate;
    end;
  end;

l.Free;
end;


Function TRemDiskListModel._Item(AIndex:Integer):TRemDisk;
begin
Result := FDiskList[AIndex];
end;

Function TRemDiskListModel.RowCount:Cardinal;
begin
Result := FDiskList.Count;
end;


End.

