Unit AbstractInstallerPage;

Interface

Uses
  Windows;

Type
  TAbstractInstallerPage = Class;

  TOnInstallerPageSelected = Procedure (APage:TAbstractInstallerPage) Of Object;
  TOnInstallerPageChanged = Procedure (APage:TAbstractInstallerPage) Of Object;

  TInstallerPageSwitchingButton  = Record
    Enabled : Boolean;
    Caption : WideString;
    end;

  TAbstractInstallerPage = Class
  Private
    FOnSelected : TOnInstallerPageSelected;
    FOnChanged : TOnInstallerPageChanged;
    FNext : TAbstractInstallerPage;
    FPrevious : TAbstractInstallerPage;
    FCancelPage : TAbstractInstallerPage;
    FContext : Pointer;
  Protected
    Procedure GoNext;
    Procedure GoPrevious;
    Procedure GoCancel;
    Procedure ReportChange;
    Function GetNextButton:TInstallerPageSwitchingButton; Virtual;
    Function GetPreviousButton:TInstallerPageSwitchingButton; Virtual;
    Function GetCancelButton:TInstallerPageSwitchingButton; Virtual;
  Public
    Constructor Create(AOnSelected:TOnInstallerPageSelected; AOnChanged:TOnInstallerPageChanged; AContext:Pointer); Reintroduce;
    Procedure SetTargets(ANext:TAbstractInstallerPage; APrevious:TAbstractInstallerPage; ACancel:TAbstractInstallerPage);

    Function NextPressed : Boolean; Virtual;
    Function PreviousPressed : Boolean; Virtual;
    Function CancelPressed : Boolean; Virtual;
    Function Skip : Boolean; Virtual;

    Property NextButton : TInstallerPageSwitchingButton Read GetNextButton;
    Property PreviousButton : TInstallerPageSwitchingButton Read GetPreviousButton;
    Property CancelButton : TInstallerPageSwitchingButton Read GetCancelButton;
    Property Context : Pointer Read FContext;
  end;


Implementation

Constructor TAbstractInstallerPage.Create(AOnSelected:TOnInstallerPageSelected; AOnChanged:TOnInstallerPageChanged; AContext:Pointer);
begin
Inherited Create;
FOnSelected := AOnSelected;
FOnChanged := AOnChanged;
FContext := AContext;
end;

Procedure TAbstractInstallerPage.GoNext;
Var
  p : TAbstractInstallerPage;
begin
p := FNext;
While (Assigned(p)) ANd (p.Skip) Do
  p := p.FNext;

If Assigned(p) Then
  FOnSelected(p);
end;

Procedure TAbstractInstallerPage.GoPrevious;
Var
  p : TAbstractInstallerPage;
begin
p := FPrevious;
While (Assigned(p)) ANd (p.Skip) Do
  p := p.FPrevious;

If Assigned(p) Then
  FOnSelected(p);
end;

Procedure TAbstractInstallerPage.GoCancel;
begin
FOnSelected(FCancelPage);
end;

Procedure TAbstractInstallerPage.ReportChange;
begin
FOnChanged(Self);
end;

Function TAbstractInstallerPage.GetNextButton:TInstallerPageSwitchingButton;
begin
Result.Caption := 'Next >';
Result.Enabled := Assigned(FNext);
end;

Function TAbstractInstallerPage.GetPreviousButton:TInstallerPageSwitchingButton;
begin
Result.Caption := '< Back';
Result.Enabled := Assigned(FPrevious);
end;

Function TAbstractInstallerPage.GetCancelButton:TInstallerPageSwitchingButton;
begin
Result.Caption := 'Cancel';
Result.Enabled := Assigned(FCancelPage);
end;

Function TAbstractInstallerPage.Skip:Boolean;
begin
Result := False;
end;

Procedure TAbstractInstallerPage.SetTargets(ANext:TAbstractInstallerPage; APrevious:TAbstractInstallerPage; ACancel:TAbstractInstallerPage);
begin
FNext := ANext;
FPrevious := APrevious;
FCancelPage := ACancel;
end;

Function TAbstractInstallerPage.NextPressed:Boolean;
begin
Result := Assigned(FNext);
If Result Then
  GoNext;
end;

Function TAbstractInstallerPage.PreviousPressed:Boolean;
begin
Result := Assigned(FPrevious);
If Result Then
  GoPrevious;
end;

Function TAbstractInstallerPage.CancelPressed:Boolean;
begin
Result := Assigned(FCancelPage);
If Result Then
  GoCancel;
end;


End.
