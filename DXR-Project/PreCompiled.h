#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

// Libs
#define NOMINMAX
#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <imgui.h>

// Common
#include "Core.h"

// Debug
#include "Debug/Debug.h"

// Containers
#include <Containers/Types.h>
#include <Containers/TArray.h>
#include <Containers/TArrayView.h>
#include <Containers/TStaticArray.h>
#include <Containers/TFunction.h>
#include <Containers/TSharedPtr.h>
#include <Containers/TUniquePtr.h>
#include <Containers/TUtilities.h>

// Application
#include "Application/Log.h"

// Utilities
#include "Utilities/HashUtilities.h"

// Math
#include "Math/Math.h"

// Core
#include "Core/ClassType.h"
#include "Core/CoreObject.h"
#include "Core/RefCountedObject.h"
#include "Core/TSharedRef.h"

// Memory
#include "Memory/Memory.h"
#include "Memory/New.h"

// Windows
#ifdef _WIN32
	#include "Windows/Windows.h"
#endif