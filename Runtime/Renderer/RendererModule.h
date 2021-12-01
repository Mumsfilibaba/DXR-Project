#pragma once
#include "Core.h"

#if MONOLITHIC_BUILD
#define RENDERER_API
#else
#if RENDERER_API_EXPORT
#define RENDERER_API MODULE_EXPORT
#else
#define RENDERER_API MODULE_IMPORT
#endif
#endif