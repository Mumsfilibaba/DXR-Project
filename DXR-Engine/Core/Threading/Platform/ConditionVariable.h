#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Threading/Windows/WindowsConditionVariable.h"
typedef CWindowsConditionVariable ConditionVariable;

#elif defined(PLATFORM_MACOS)
#include "Core/Threading/Generic/GenericConditionVariable.h"
typedef CGenericConditionVariable ConditionVariable;
//TODO: MacCondition variable

#else
#include "Core/Threading/Generic/GenericConditionVariable.h"
typedef CGenericConditionVariable ConditionVariable;

#endif
