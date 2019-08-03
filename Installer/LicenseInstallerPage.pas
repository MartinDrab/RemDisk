Unit LicenseInstallerPage;

Interface

Uses
  AbstractInstallerPage;

Type
  TLicenseInstallerPage = Class (TAbstractInstallerPage)
  Private
    FAgree : Boolean;
  Protected
    Procedure SetAgree(AValue:Boolean);
    Function GetNextButton:TInstallerPageSwitchingButton; Override;
  Public

    Property Agree : Boolean Read FAgree Write SetAgree;
  end;


Implementation

Procedure TLicenseInstallerPage.SetAgree(AValue:Boolean);
begin
FAgree := AValue;
ReportChange;
end;

Function TLicenseInstallerPage.GetNextButton:TInstallerPageSwitchingButton;
begin
Result := Inherited GetNextButton;
Result.Enabled := FAgree;
end;



End.
