#pragma once
#include "Core/Core.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Engine API

#if MONOLITHIC_BUILD
#define ENGINE_API
#else
#if ENGINE_IMPL
#define ENGINE_API MODULE_EXPORT
#else
#define ENGINE_API MODULE_IMPORT
#endif
#endif