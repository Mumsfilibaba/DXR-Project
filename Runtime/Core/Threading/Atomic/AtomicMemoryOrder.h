#pragma once
#include "Core/CoreTypes.h"

/**
 * @brief Ordering constraint for atomic operations.
 */

enum class EMemoryOrder : uint8
{
    Relaxed = 0,                      // No ordering constraints, only atomicity.
    Acquire,                          // No reads/writes in the current thread can be reordered before this load.
    Release,                          // No reads/writes in the current thread can be reordered after this store.
    AcquireRelease,                   // Combines Acquire and Release. Valid on read-modify-write operations only.
    SequentiallyConsistent,           // Sequentially consistent ordering. The strongest order.
    Default = SequentiallyConsistent, // Alias for the strongest order. Used as the default template argument.
};
