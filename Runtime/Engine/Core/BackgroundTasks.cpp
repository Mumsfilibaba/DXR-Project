#include "Core/Math/Math.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Threading/ScopedLock.h"
#include "Engine/Core/BackgroundTasks.h"

static constexpr float ProgressScale = 1000000.0f;

FBackgroundTask::FBackgroundTask(const FString& InLabel, bool bInCancellable, double InStartSeconds)
    : Label(InLabel)
    , bCancellable(bInCancellable)
    , StartSeconds(InStartSeconds)
    , Progress(-1)
    , bCancelRequested(false)
    , bFinished(false)
{
}

FBackgroundTask::~FBackgroundTask() = default;

void FBackgroundTask::SetProgress(float InProgress)
{
    Progress.Store(static_cast<int32>(Math::Clamp(InProgress, 0.0f, 1.0f) * ProgressScale));
}

float FBackgroundTask::GetProgress() const
{
    const int32 Value = Progress.Load();
    return Value < 0 ? UnknownProgress : static_cast<float>(Value) / ProgressScale;
}

void FBackgroundTask::RequestCancel()
{
    if (bCancellable)
    {
        bCancelRequested.Store(true);
    }
}

bool FBackgroundTask::IsCancelRequested() const
{
    return bCancelRequested.Load();
}

void FBackgroundTask::Finish()
{
    bFinished.Store(true);
}

bool FBackgroundTask::IsFinished() const
{
    return bFinished.Load();
}

FBackgroundTasks& FBackgroundTasks::Get()
{
    static FBackgroundTasks Instance;
    return Instance;
}

double FBackgroundTasks::GetSeconds()
{
    static const double Frequency = static_cast<double>(FPlatformTime::QueryPerformanceFrequency());
    return static_cast<double>(FPlatformTime::QueryPerformanceCounter()) / Frequency;
}

FBackgroundTasks::FBackgroundTasks()
    : TasksCS()
    , Tasks()
{
}

FBackgroundTasks::~FBackgroundTasks() = default;

TSharedPtr<FBackgroundTask> FBackgroundTasks::Begin(const FString& Label, bool bCancellable, double StartSeconds)
{
    TSharedPtr<FBackgroundTask> Task = MakeSharedPtr<FBackgroundTask>(Label, bCancellable, StartSeconds);

    SCOPED_LOCK(TasksCS);
    Tasks.Add(Task);
    return Task;
}

TArray<TSharedPtr<FBackgroundTask>> FBackgroundTasks::GetVisibleTasks(double NowSeconds)
{
    TArray<TSharedPtr<FBackgroundTask>> Visible;

    SCOPED_LOCK(TasksCS);
    for (int32 Index = 0; Index < Tasks.Size();)
    {
        if (Tasks[Index]->IsFinished())
        {
            Tasks.RemoveAt(Index);
            continue;
        }

        if (NowSeconds - Tasks[Index]->GetStartSeconds() >= ShowAfterSeconds)
        {
            Visible.Add(Tasks[Index]);
        }

        ++Index;
    }

    return Visible;
}

int32 FBackgroundTasks::GetNumRunningTasks()
{
    SCOPED_LOCK(TasksCS);

    int32 NumRunning = 0;
    for (const TSharedPtr<FBackgroundTask>& Task : Tasks)
    {
        NumRunning += Task->IsFinished() ? 0 : 1;
    }

    return NumRunning;
}
