#include "EngineLoopTicker.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EngineLoopTicker

CEngineLoopTicker CEngineLoopTicker::Instance;

void CEngineLoopTicker::Tick(CTimestamp Deltatime)
{
    for (CTickDelegate TickDelegate : TickDelegates)
    {
        TickDelegate.ExecuteIfBound(Deltatime);
    }
}

void CEngineLoopTicker::AddElement(const CTickDelegate& NewElement)
{
    TickDelegates.Push(NewElement);
}

void CEngineLoopTicker::RemoveElement(CDelegateHandle RemoveHandle)
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