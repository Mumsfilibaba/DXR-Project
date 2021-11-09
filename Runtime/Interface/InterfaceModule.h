#pragma once
#include "Core.h"

#if INTERFACE_API_EXPORT
#define INTERFACE_API MODULE_EXPORT
#else
#define INTERFACE_API MODULE_IMPORT
#endif