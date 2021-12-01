#pragma once
#include "Core.h"

#if MONOLITHIC_BUILD
#define ENGINE_API
#else
#if ENGINE_API_EXPORT
#define ENGINE_API MODULE_EXPORT
#else
#define ENGINE_API MODULE_IMPORT
#endif
#endif