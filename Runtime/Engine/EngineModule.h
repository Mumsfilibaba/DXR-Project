#pragma once
#include "Core.h"

#if ENGINE_API_EXPORT
#define ENGINE_API MODULE_EXPORT
#else
#define ENGINE_API MODULE_IMPORT
#endif