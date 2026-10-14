#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "Engine/EngineModule.h"

class ENGINE_API FBackgroundTask : public FNonCopyable
{
public:
    static constexpr float UnknownProgress = -1.0f;

    FBackgroundTask(const FString& InLabel, bool bInCancellable, double InStartSeconds);
    ~FBackgroundTask();

    /** @brief Report how much of the work is done, from 0 to 1; any thread can call it */
    void SetProgress(float InProgress);

    /** @return Returns how much of the work is done, from 0 to 1, or UnknownProgress when the work hasn't said */
    NODISCARD float GetProgress() const;

    /** @brief Ask the work to stop; it checks IsCancelRequested and finishes early. Does nothing for work that can't be cancelled. */
    void RequestCancel();

    NODISCARD bool IsCancelRequested() const;

    /** @brief Mark the work done, failed or cancelled, which removes the task from the list; any thread can call it */
    void Finish();

    NODISCARD bool IsFinished() const;

    FORCEINLINE const FString& GetLabel() const
    {
        return Label;
    }

    FORCEINLINE bool CanCancel() const
    {
        return bCancellable;
    }

    FORCEINLINE double GetStartSeconds() const
    {
        return StartSeconds;
    }

private:
    FString      Label;
    bool         bCancellable;
    double       StartSeconds;
    FAtomicInt32 Progress;
    FAtomicBool  bCancelRequested;
    FAtomicBool  bFinished;
};

class ENGINE_API FBackgroundTasks : public FNonCopyable
{
public:

    /** @brief Work that ends sooner than this is never shown, so quick work doesn't flicker */
    static constexpr double ShowAfterSeconds = 0.25;

    static FBackgroundTasks& Get();

    /** @return Returns the seconds since an arbitrary start, the clock tasks are timed with */
    NODISCARD static double GetSeconds();

public:
    FBackgroundTasks();
    ~FBackgroundTasks();

    /** @brief Start tracking work by its label; the work finishes the returned task when it ends */
    TSharedPtr<FBackgroundTask> Begin(const FString& Label, bool bCancellable, double StartSeconds = GetSeconds());

    /** @return Returns the unfinished tasks that have run for ShowAfterSeconds at NowSeconds, oldest first, and forgets the finished ones */
    NODISCARD TArray<TSharedPtr<FBackgroundTask>> GetVisibleTasks(double NowSeconds = GetSeconds());

    /** @return Returns how many tasks haven't finished, whether shown or not */
    NODISCARD int32 GetNumRunningTasks();

private:
    FCriticalSection                    TasksCS;
    TArray<TSharedPtr<FBackgroundTask>> Tasks;
};
