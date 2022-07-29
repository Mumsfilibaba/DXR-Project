#include "EngineLoopTicker.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngineLoopTicker

FEngineLoopTicker::FEngineLoopTicker()
    : TickDelegates()
{ }

FEngineLoopTicker& FEngineLoopTicker::Get()
{
    static FEngineLoopTicker Instance;
    return Instance;
}

void FEngineLoopTicker::Tick(FTimespan Deltatime)
{
    for (FTickDelegate TickDelegate : TickDelegates)
    {
        TickDelegate.ExecuteIfBound(Deltatime);
    }
}

void FEngineLoopTicker::AddDelegate(const FTickDelegate& NewElement)
{
    TickDelegates.Push(NewElement);
}

void FEngineLoopTicker::RemoveDelegate(FDelegateHandle RemoveHandle)
{
    uint32 NumElements = TickDelegates.Size();
    for (uint32 Index = 0; Index < NumElements; Index++)
    {
        if (TickDelegates[Index].GetHandle() == RemoveHandle)
        {
            TickDelegates.RemoveAt(Index);
        }
    }
}