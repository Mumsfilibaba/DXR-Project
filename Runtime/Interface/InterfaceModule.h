#pragma once
#include "Core.h"

#if MONOLITHIC_BUILD
#define INTERFACE_API
#else
#if INTERFACE_API_EXPORT
#define INTERFACE_API MODULE_EXPORT
#else
#define INTERFACE_API MODULE_IMPORT
#endif
#endif