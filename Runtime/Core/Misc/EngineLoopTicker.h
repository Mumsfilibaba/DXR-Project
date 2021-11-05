#pragma once
#include "CoreModule.h"

#include "Core/Time/Timestamp.h"
#include "Core/Containers/Array.h"
#include "Core/Delegates/Delegate.h"

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Globally available TickDelegate class */
DECLARE_DELEGATE( CTickDelegate, CTimestamp );

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Enables systems to bind to the tick function, this enables modules to be called every frame */
class CORE_API CEngineLoopTicker
{
public:

    static FORCEINLINE CEngineLoopTicker& Get()
    {
        return Instance;
    }

    /* Ensures that all delegates are fired */
    void Tick( CTimestamp Deltatime );

    /* Add a new element that should be called when the engineloop ticks */ 
    void AddElement( const CTickDelegate& NewElement );

    /* Remove all instances of a delegate from the Tick-loop */
    void RemoveElement( CDelegateHandle RemoveHandle );

private:

    CEngineLoopTicker() = default;
    ~CEngineLoopTicker() = default;

    // TODO: These should be stored in some priority, also there should be the possibility to add a delay for systems that does not need to be called every frame
    TArray<CTickDelegate> TickDelegates;

    static CEngineLoopTicker Instance;
};