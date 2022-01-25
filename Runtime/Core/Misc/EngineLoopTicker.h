#pragma once
#include "Core/Core.h"
#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"
#include "Core/Delegates/Delegate.h"

DECLARE_DELEGATE(CTickDelegate, CTimestamp);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Enables systems to bind to the tick function, this enables modules to be called every frame */

class CORE_API CEngineLoopTicker
{
public:

    /**
     * Retrieve the EngineLoopTicker instance
     * 
     * @return: Returns the EngineLoopTicker instance
     */
    static CEngineLoopTicker& Get();

    /**
     * Ensures that all delegates are fired
     * 
     * @param DeltaTime: Time since last time the delegates was fired
     */
    void Tick(CTimestamp Deltatime);

    /**
     * Add a new element that should be called when the EngineLoop ticks 
     * 
     * @param NewElement: Delegate to add to the engine-loop ticker
     */
    void AddElement(const CTickDelegate& NewElement);

    /**
     * Remove all instances of a delegate from the Tick-loop 
     * 
     * @param RemoveHandle: DelegateHandle of the delegate to remove
     */
    void RemoveElement(CDelegateHandle RemoveHandle);

private:

    CEngineLoopTicker() = default;
    ~CEngineLoopTicker() = default;

    // TODO: These should be stored in some priority, also there should be the possibility to add a delay for systems that does not need to be called every frame
    TArray<CTickDelegate> TickDelegates;

    static CEngineLoopTicker Instance;
};