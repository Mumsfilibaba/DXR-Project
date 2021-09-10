#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Windows/WindowsPlatform.h"
typedef WindowsPlatform Platform;

#elif defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericPlatform.h"
typedef GenericPlatform Platform;
// TODO: MacPlatform

#else
#include "Core/Application/Generic/GenericPlatform.h"
typedef GenericPlatform Platform;

#endif
