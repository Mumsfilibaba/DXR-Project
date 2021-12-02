#pragma once
#include "Core.h"

#if MONOLITHIC_BUILD
#define CORE_API
#else
#if CORE_IMPL
#define CORE_API MODULE_EXPORT
#else
#define CORE_API MODULE_IMPORT
#endif
#endif