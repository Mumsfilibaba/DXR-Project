#include "Core/Windows/WindowsPlatformMemory.h"
#include <psapi.h>

bool FWindowsPlatformMemory::GetStats(FPlatformMemoryStats& OutStats)
{
    OutStats = FPlatformMemoryStats();

    PROCESS_MEMORY_COUNTERS_EX Counters;
    if (!::K32GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&Counters), sizeof(Counters)))
    {
        return false;
    }

    OutStats.FootprintBytes     = static_cast<uint64>(Counters.PrivateUsage);
    OutStats.PeakFootprintBytes = static_cast<uint64>(Counters.PeakPagefileUsage);
    OutStats.ResidentBytes      = static_cast<uint64>(Counters.WorkingSetSize);
    OutStats.PeakResidentBytes  = static_cast<uint64>(Counters.PeakWorkingSetSize);

    HEAP_SUMMARY Summary;
    Summary.cb = sizeof(Summary);

    if (::HeapSummary(::GetProcessHeap(), 0, &Summary))
    {
        OutStats.AllocatorInUseBytes = static_cast<uint64>(Summary.cbAllocated);
    }

    MEMORYSTATUSEX MemoryStatus;
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    if (::GlobalMemoryStatusEx(&MemoryStatus))
    {
        OutStats.TotalPhysicalBytes     = MemoryStatus.ullTotalPhys;
        OutStats.AvailablePhysicalBytes = MemoryStatus.ullAvailPhys;
    }

    return true;
}
