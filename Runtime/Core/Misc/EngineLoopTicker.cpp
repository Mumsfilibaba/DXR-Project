#include "EngineLoopTicker.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EngineLoopTicker

FEngineLoopTicker& FEngineLoopTicker::Get()
{
    static FEngineLoopTicker Instance;
    return Instance;
}

void FEngineLoopTicker::Tick(FTimestamp Deltatime)
{
    for (FTickDelegate TickDelegate : TickDelegates)
    {
        TickDelegate.ExecuteIfBound(Deltatime);
    }
}

void FEngineLoopTicker::AddElement(const FTickDelegate& NewElement)
{
    TickDelegates.Push(NewElement);
}

void FEngineLoopTicker::RemoveElement(FDelegateHandle RemoveHandle)
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