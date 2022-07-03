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
        DeltaTime = FTimestamp(0);
        TotalTime = FTimestamp(0);
    }

    FORCEINLINE const FTimestamp& GetDeltaTime() const
    {
        return DeltaTime;
    }

    FORCEINLINE const FTimestamp& GetTotalTime() const
    {
        return TotalTime;
    }

private:
    FTimestamp TotalTime;
    FTimestamp DeltaTime;

    uint64     LastTime  = 0;
    uint64     Frequency = 0;
};