#pragma once
#include <cassert>
#include <utility>
#include <algorithm>

// Libs
#define NOMINMAX
#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <imgui.h>

// Common
#include "Defines.h"
#include "Types.h"

// Debug
#include "Debug/Debug.h"

// Containers
#include "Containers/String.h"
#include "Containers/TArray.h"
#include "Containers/TSharedPtr.h"
#include "Containers/TSharedRef.h"
#include "Containers/TUniquePtr.h"

// Application
#include "Application/Log.h"

// Utilities
#include "Utilities/TUtilities.h"
#include "Utilities/HashUtilities.h"

// Math
#include "Math/Math.h"

// Core
#include "Core/ClassType.h"
#include "Core/CoreObject.h"
#include "Core/RefCountedObject.h"

// Memory
#include "Memory/Memory.h"
#include "Memory/New.h"

// Windows
#ifdef _WIN32
	#include "Windows/Windows.h"
#endif