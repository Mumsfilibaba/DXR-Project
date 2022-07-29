#pragma once
#include "Core/Core.h"
#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"
#include "Core/Delegates/Delegate.h"

DECLARE_DELEGATE(FTickDelegate, FTimespan);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FEngineLoopTicker

class CORE_API FEngineLoopTicker
{
    FEngineLoopTicker();
    ~FEngineLoopTicker() = default;

public:

    /**
     * @brief: Retrieve the EngineLoopTicker instance
     * 
     * @return: Returns the EngineLoopTicker instance
     */
    static FEngineLoopTicker& Get();

    /**
     * @brief: Ensures that all delegates are fired
     * 
     * @param DeltaTime: Time since last time the delegates was fired
     */
    void Tick(FTimespan Deltatime);

    /**
     * @brief: Add a new element that should be called when the EngineLoop ticks 
     * 
     * @param NewDelegate: Delegate to add to the engine-loop ticker
     */
    void AddDelegate(const FTickDelegate& NewDelegate);

    /**
     * @brief: Remove all instances of a delegate from the Tick-loop 
     * 
     * @param RemoveHandle: DelegateHandle of the delegate to remove
     */
    void RemoveDelegate(FDelegateHandle RemoveHandle);

private:
    // TODO: These should be stored in some priority, also there should be the possibility to add a delay for systems that does not need to be called every frame
    TArray<FTickDelegate> TickDelegates;
};