#include "Core/Mac/MacPlatformMemory.h"
#include <mach/mach.h>
#include <sys/sysctl.h>

bool FMacPlatformMemory::GetStats(FPlatformMemoryStats& OutStats)
{
    OutStats = FPlatformMemoryStats();

    task_vm_info_data_t    TaskInfo;
    mach_msg_type_number_t TaskInfoCount = TASK_VM_INFO_COUNT;

    if (::task_info(::mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&TaskInfo), &TaskInfoCount) != KERN_SUCCESS)
    {
        return false;
    }

    OutStats.FootprintBytes    = TaskInfo.phys_footprint;
    OutStats.ResidentBytes     = TaskInfo.resident_size;
    OutStats.PeakResidentBytes = TaskInfo.resident_size_peak;

    const bool bHasFootprintPeak = (TaskInfoCount >= TASK_VM_INFO_REV3_COUNT) && (TaskInfo.ledger_phys_footprint_peak > 0);
    OutStats.PeakFootprintBytes = bHasFootprintPeak ? static_cast<uint64>(TaskInfo.ledger_phys_footprint_peak) : TaskInfo.phys_footprint;

    malloc_statistics_t MallocStats;
    ::malloc_zone_statistics(nullptr, &MallocStats);
    OutStats.AllocatorInUseBytes = MallocStats.size_in_use;

    uint64 PhysicalBytes     = 0;
    size_t PhysicalBytesSize = sizeof(PhysicalBytes);

    if (::sysctlbyname("hw.memsize", &PhysicalBytes, &PhysicalBytesSize, nullptr, 0) == 0)
    {
        OutStats.TotalPhysicalBytes = PhysicalBytes;
    }

    const mach_port_t Host = ::mach_host_self();

    vm_size_t              HostPageSize = 0;
    vm_statistics64_data_t VMStats;
    mach_msg_type_number_t VMStatsCount = HOST_VM_INFO64_COUNT;

    if (::host_page_size(Host, &HostPageSize) == KERN_SUCCESS &&
        ::host_statistics64(Host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&VMStats), &VMStatsCount) == KERN_SUCCESS)
    {
        OutStats.AvailablePhysicalBytes = (static_cast<uint64>(VMStats.free_count) + static_cast<uint64>(VMStats.inactive_count)) * static_cast<uint64>(HostPageSize);
    }

    ::mach_port_deallocate(::mach_task_self(), Host);
    return true;
}
