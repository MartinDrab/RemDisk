Unit AbstractInstallTask;

Interface

Uses
  InstallerSettings;

Type
  TAbstractInstallTask = Class
  Private
    FCritical : Boolean;
    FErrorCode : Cardinal;
    FInstallerSettings : TInstallerSettings;
  Public
    Constructor Create(AInstallerSettings:TInstallerSettings; ACritical:Boolean = False); Reintroduce;

    Procedure Execute; Virtual; Abstract;
    Function GetDescription:WideString; Virtual; Abstract;
    Function CounterTask:TAbstractInstallTask; Virtual;

    Property Critical : Boolean Read FCritical;
    Property InstallerSettings : TInstallerSettings Read FInstallerSettings;
    Property ErrorCode : Cardinal Read FErrorCode;
  end;


Implementation

Constructor TAbstractInstallTask.Create(AInstallerSettings:TInstallerSettings; ACritical:Boolean = False);
begin
Inherited Create;
FInstallerSettings := AInstallerSettings;
FCritical := ACritical;
end;

Function TAbstractInstallTask.CounterTask:TAbstractInstallTask;
begin
Result := Nil;
end;



End.
