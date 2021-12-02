#pragma once
#include "Core.h"

#if MONOLITHIC_BUILD
#define INTERFACE_API
#else
#if INTERFACE_IMPL
#define INTERFACE_API MODULE_EXPORT
#else
#define INTERFACE_API MODULE_IMPORT
#endif
#endif