#pragma once
#include "Core.h"

#if CORE_API_EXPORT
#define CORE_API MODULE_EXPORT
#else
#define CORE_API MODULE_IMPORT
#endif