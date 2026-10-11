#include "MemoryProfilerTests.h"

FMemoryNameId GetSharedMemoryNameIdFromOtherUnit()
{
    static CONST_INIT FMemoryName SharedName("MemoryProfilerTest.Shared");
    return SharedName.GetId();
}
