Unit InstallTaskThread;

Interface

Uses
  Classes, Generics.Collections, SyncObjs,
  AbstractInstallTask;

Type
  TInstallTaskThread = Class (TThread)
  Public
    FCurrentTask : TAbstractInstallTask;
    FSemaphore : TSemaphore;
    FTaskListLock : TCriticalSection;
    FCompletedTaskList : TList<TAbstractInstallTask>;
    FTaskList : TList<TAbstractInstallTask>;
    FOnTaskComplete : TNotifyEvent;
    Procedure DoTaskCompletion;
  Protected
    Procedure Execute; Override;
  Public
    Constructor Create(AOnTaskComplete:TNotifyEvent; ASuspend:Boolean); Reintroduce;
    Destructor Destroy; Override;

    Procedure AddTask(ATask:TAbstractInstallTask);
  end;


Implementation

Procedure TInstallTaskThread.Execute;
Var
  t : TAbstractInstallTask;
  counterTask : TAbstractInstallTask;
begin
FreeOnTerminate := False;
While Assigned(FCurrentTask) Do
  begin
  FCurrentTask := Nil;
  FSemaphore.Acquire;
  FTaskListLock.Acquire;
  FCurrentTask := FTaskList[0];
  FTaskList.Delete(0);
  FTaskListLock.Release;
  If Assigned(FCurrentTask) Then
    begin
    FCurrentTask.Execute;
    Synchronize(DoTaskCompletion);
    If FCurrentTask.ErrorCode <> 0 Then
      begin
      If FCurrentTask.Critical Then
        begin
        For t In FCompletedTaskList Do
          begin
          counterTask := t.CounterTask;
          If Assigned(counterTask) Then
            AddTask(counterTask);

          t.Free;
          end;

        FCompletedTaskList.Clear;
        end;
      end
    Else FCompletedTaskList.Insert(0, FCurrentTask);
    end;
  end;
end;

Procedure TInstallTaskThread.DoTaskCompletion;
begin
FOnTaskComplete(FCurrentTask);
end;

Procedure TInstallTaskThread.AddTask(ATask:TAbstractInstallTask);
begin
FTaskListLock.Acquire;
FTaskList.Add(ATask);
FTaskListLock.Release;
FSemaphore.Release;
end;

Constructor TInstallTaskThread.Create(AOnTaskComplete:TNotifyEvent; ASuspend:Boolean);
begin
FSemaphore := TSemaphore.Create(Nil, 0, $7FFFFFFF, '');
FTaskListLock := TCriticalSection.Create;
FTaskList := TList<TAbstractInstallTask>.Create;
FCompletedTaskList := TList<TAbstractInstallTask>.Create;
FOnTaskComplete := AOnTaskComplete;
Inherited Create(ASuspend);
end;

Destructor TInstallTaskThread.Destroy;
begin
FSemaphore.Free;
FTaskListLock.Free;
FTaskList.Free;
FCompletedTaskList.Free;
Inherited Destroy;
end;



End.
