#pragma once
#include "AsyncThreadPool.h"

#include "Core/Core.h"
#include "Core/Platform/PlatformThreadMisc.h"
#include "Core/Containers/PriorityQueue.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IAsyncTask

struct IAsyncTask
{
    virtual ~IAsyncTask() = default;

    /** @breif: Perform work async on a work-thread */
    virtual void DoAsyncWork() = 0;

    /** @breif: Abandon this task, called by the task-queue if enqueued during shutdown */
    virtual void Abandon() = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncTaskBase

class CORE_API FAsyncTaskBase
    : public IAsyncTask
{
public:
    FAsyncTaskBase();
    virtual ~FAsyncTaskBase();

    virtual void DoAsyncWork() override final;
    virtual void Abandon()     override final;

    /**
     * @breif: Launches the task on a new thread.
     * 
     * @param bAsync: True if the task should execute on the calling thread or async
     * @return: Returns true if the task was successfully queued up
     */
    bool Launch(EQueuePriority Priority = EQueuePriority::Normal, bool bAsync = true);

    /**
     * @breif: Cancels the task. Must be called before the task is scheduled on the worker-thread.
     * 
     * @return: Returns true if the task was successfully canceled
     */
    bool Cancel();

    /** @brief: Waits for the task to finish executing (Blocks the calling thread) */
    void WaitForCompletion()
    {
        if (TaskCompleteEvent)
        {
            TaskCompleteEvent->Wait(FTimespan::Infinity());
        }
    }

    /** @return: Returns true if the task is complete or false otherwise */
    bool IsComplete() const
    {
        return (NumInvokations.Load() == 0);
    }

protected:

    /** @brief: Execute the actual work for this task */
    virtual void Execute() = 0;

    /** @return: Tries to abandon the task, returns true if successful, false otherwise */
    virtual bool TryAbandon() = 0;

private:
    FORCEINLINE void FinishAsyncTask()
    {
        if (TaskCompleteEvent)
        {
            TaskCompleteEvent->Trigger();
        }
    }

    FORCEINLINE void DoWork()
    {
        Execute();
        Check(NumInvokations.Load() == 1);
        NumInvokations--;
    }

    FORCEINLINE void DestroyWaitEvent()
    {
        TaskCompleteEvent.Reset();
    }

    FGenericEventRef TaskCompleteEvent;
    FAtomicInt32     NumInvokations;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAbanbonableTask

struct FAbanbonableTask
{
    bool CanAbandon() const { return true; }

    void Abandon() { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FNonAbanbonableTask

struct FNonAbanbonableTask
{
    bool CanAbandon() const { return false; }

    void Abandon() { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAsyncTask

template<typename TaskType>
class TAsyncTask
    : public FAsyncTaskBase
{
public:
    template<typename... ArgTypes>
    TAsyncTask(ArgTypes&&... Args)
        : FAsyncTaskBase()
        , Task(Forward<ArgTypes>(Args)...)
    { }

private:
    virtual void Execute() override final
    {
        Task.DoWork();
    }

    virtual bool TryAbandon() override final
    {
        if (Task.CanAbandon())
        {
            Task.Abandon();
            return true;
        }

        return false;
    }

    TaskType Task;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAsyncLambdaTask

template<typename LambdaType>
class TAsyncLambdaTask
    : public FAsyncTaskBase
{
public:
    TAsyncLambdaTask(LambdaType&& InLambda)
        : FAsyncTaskBase()
        , Lambda(Forward<LambdaType>(InLambda))
    { }

private:
    virtual void Execute() override final
    {
        Lambda();
    }

    virtual bool TryAbandon() override final
    {
        return false;
    }

    LambdaType Lambda;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TAutoAsyncTask

template<typename TaskType>
class TAutoAsyncTask
    : public IAsyncTask
{
public:
    TAutoAsyncTask(TaskType&& InTask)
        : Task(::Forward<TaskType>(InTask))
    { }

    virtual void DoAsyncWork() override final
    {
        DoWork();
    }

    virtual void Abandon() override final
    {
        if (Task.CanAbandon())
        {
            Task.Abandon();
            delete this;
        }
        else
        {
            DoWork();
        }
    }

    bool Launch(EQueuePriority Priority = EQueuePriority::Normal, bool bAsync = true)
    {
        if (bAsync)
        {
            FAsyncThreadPool::Get().SubmitTask(this, Priority);
        }
        else
        {
            DoWork();
        }

        return true;
    }

private:
    FORCEINLINE void DoWork()
    {
        Task.DoWork();
        delete this;
    }

    TaskType Task;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FAsyncLambda

template<typename LambdaType>
class TAsyncLambda
    : public FNonAbanbonableTask
{
public:
    FORCEINLINE TAsyncLambda(LambdaType&& InLambda)
        : Lambda(::Forward<LambdaType>(InLambda))
    { }

    void DoWork()
    {
        Lambda();
    }

private:
    LambdaType Lambda;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Async

template<typename LambdaType>
inline void Async(LambdaType&& InLambda, EQueuePriority Priority = EQueuePriority::Normal, bool bExecuteAsync = true)
{
    using AsyncType = TAutoAsyncTask<TAsyncLambda<LambdaType>>;

    AsyncType* AsynTask = dbg_new AsyncType(::Forward<LambdaType>(InLambda));
    Check(AsynTask != nullptr);
    AsynTask->Launch(Priority, bExecuteAsync);
}
