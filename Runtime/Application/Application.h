#pragma once
#include "Core/Core.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// Application API

#if MONOLITHIC_BUILD
#define APPLICATION_API
#else
#if APPLICATION_IMPL
#define APPLICATION_API MODULE_EXPORT
#else
#define APPLICATION_API MODULE_IMPORT
#endif
#endif