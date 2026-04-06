#include "Core/Stats/Stats.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Templates/CString.h"

FStatRegistry::FStatRegistry()  = default;
FStatRegistry::~FStatRegistry() = default;

FStatRegistry& FStatRegistry::Get()
{
    static FStatRegistry Instance;
    return Instance;
}

void FStatRegistry::RegisterStat(FStatData* Stat)
{
    TScopedLock Lock(StatsCS);
    Stats.Add(Stat);
}

void FStatRegistry::UnregisterStat(FStatData* Stat)
{
    TScopedLock Lock(StatsCS);

    const int32 Index = Stats.Find(Stat);
    if (Index != -1)
    {
        Stats.RemoveAt(Index);
    }
}

const TArray<FStatData*>& FStatRegistry::GetAllStats() const
{
    return Stats;
}

void FStatRegistry::GetStatsByGroup(const CHAR* GroupName, TArray<FStatData*>& OutStats) const
{
    for (FStatData* Stat : Stats)
    {
        if (FCString::Strcmp(Stat->GroupName, GroupName) == 0)
        {
            OutStats.Add(Stat);
        }
    }
}

void FStatRegistry::GetGroups(TArray<const CHAR*>& OutGroups) const
{
    for (FStatData* Stat : Stats)
    {
        bool bFound = false;
        for (const CHAR* Existing : OutGroups)
        {
            if (FCString::Strcmp(Existing, Stat->GroupName) == 0)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            OutGroups.Add(Stat->GroupName);
        }
    }
}
