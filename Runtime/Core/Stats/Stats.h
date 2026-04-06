#pragma once
#include "Core/Core.h"
#include "Core/Threading/Atomic.h"
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"

enum class EStatType : uint8
{
    Memory,
    Counter,
};

struct FStatData
{
    const CHAR*  StatName;
    const CHAR*  GroupName;
    EStatType    Type;
    FAtomicInt64 Value;
};

class CORE_API FStatRegistry
{
public:
    static FStatRegistry& Get();

    void RegisterStat(FStatData* Stat);
    void UnregisterStat(FStatData* Stat);

    const TArray<FStatData*>& GetAllStats() const;
    void GetStatsByGroup(const CHAR* GroupName, TArray<FStatData*>& OutStats) const;
    void GetGroups(TArray<const CHAR*>& OutGroups) const;

private:
    FStatRegistry();
    ~FStatRegistry();

    TArray<FStatData*> Stats;
    FCriticalSection   StatsCS;
};

struct CORE_API FStatAutoRegistration
{
    FStatAutoRegistration(FStatData* InStat)
        : Stat(InStat)
    {
        FStatRegistry::Get().RegisterStat(InStat);
    }

    ~FStatAutoRegistration()
    {
        FStatRegistry::Get().UnregisterStat(Stat);
    }

    FStatData* Stat;
};

// -------------------------------------------------------------------------------------------------
// Stats Enabled
// -------------------------------------------------------------------------------------------------

#if !RELEASE_BUILD
    #define STATS_ENABLED (1)
#else
    #define STATS_ENABLED (0)
#endif

// -------------------------------------------------------------------------------------------------
// Stat Macros
// -------------------------------------------------------------------------------------------------

#if STATS_ENABLED
    #define STAT_DECLARE_EXTERN(ApiMacro, StatId) \
        extern ApiMacro FStatData StatId

    #define STAT_DEFINE_MEMORY(StatId, DisplayName, GroupName) \
        FStatData StatId = { DisplayName, GroupName, EStatType::Memory, {} }; \
        static FStatAutoRegistration StatId##_AutoReg(&StatId)

    #define STAT_DEFINE_COUNTER(StatId, DisplayName, GroupName) \
        FStatData StatId = { DisplayName, GroupName, EStatType::Counter, {} }; \
        static FStatAutoRegistration StatId##_AutoReg(&StatId)

    #define STAT_ADD(StatId, Amount)      (StatId).Value.Add(static_cast<int64>(Amount))
    #define STAT_SUBTRACT(StatId, Amount) (StatId).Value.Subtract(static_cast<int64>(Amount))
    #define STAT_SET(StatId, NewValue)    (StatId).Value.Store(static_cast<int64>(NewValue))
    #define STAT_GET(StatId)              (StatId).Value.Load()
#else
    #define STAT_DECLARE_EXTERN(ApiMacro, StatId)
    #define STAT_DEFINE_MEMORY(StatId, DisplayName, GroupName)
    #define STAT_DEFINE_COUNTER(StatId, DisplayName, GroupName)
    
    #define STAT_ADD(StatId, Amount)      ((void)0)
    #define STAT_SUBTRACT(StatId, Amount) ((void)0)
    #define STAT_SET(StatId, NewValue)    ((void)0)
    #define STAT_GET(StatId)              (0)
#endif
