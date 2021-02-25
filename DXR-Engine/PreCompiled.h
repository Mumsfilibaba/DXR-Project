#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

// Libs
#ifdef PLATFORM_WINDOWS
    // Windows
    #define NOMINMAX
    #include "Windows/Windows.h"

    #include <dxgi1_6.h>
    #include <d3d12.h>

    #include <wrl/client.h>
#endif

#include <imgui.h>

// Common
#include "Core.h"

// Debug
#include "Debug/Debug.h"

// Containers
#include <Containers/Types.h>
#include <Containers/Array.h>
#include <Containers/ArrayView.h>
#include <Containers/StaticArray.h>
#include <Containers/Function.h>
#include <Containers/SharedPtr.h>
#include <Containers/UniquePtr.h>
#include <Containers/Utilities.h>

// Application
#include "Application/Log.h"

// Utilities
#include "Utilities/HashUtilities.h"
#include "Utilities/StringUtilities.h"

// Math
#include "Math/Math.h"

// Core
#include "Core/ClassType.h"
#include "Core/CoreObject.h"
#include "Core/RefCountedObject.h"
#include "Core/Ref.h"

// Memory
#include "Memory/Memory.h"
#include "Memory/New.h"