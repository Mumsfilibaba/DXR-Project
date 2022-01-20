#pragma once
#include "CoreDefines.h"
#include "CoreTypes.h"

#include "Core/Memory/New.h"

#if MONOLITHIC_BUILD
#define CORE_API
#else
#if CORE_IMPL
#define CORE_API MODULE_EXPORT
#else
#define CORE_API MODULE_IMPORT
#endif
#endif