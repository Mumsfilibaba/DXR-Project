#pragma once
#include "Timestamp.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FTimer

class CORE_API FTimer
{
public:
    FTimer();
    ~FTimer() = default;

    /** @brief: Measure time between this and last call to tick */
    void Tick();

    /** @brief: Resets the time to zero */
    FORCEINLINE void Reset()
    {
        DeltaTime = FTimespan(0);
        TotalTime = FTimespan(0);
    }

    FORCEINLINE const FTimespan& GetDeltaTime() const
    {
        return DeltaTime;
    }

    FORCEINLINE const FTimespan& GetTotalTime() const
    {
        return TotalTime;
    }

private:
    FTimespan TotalTime;
    FTimespan DeltaTime;

    uint64     LastTime  = 0;
    uint64     Frequency = 0;
};