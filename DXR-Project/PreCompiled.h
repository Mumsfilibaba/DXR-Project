#pragma once
#include <vector>
#include <memory>
#include <cassert>
#include <utility>
#include <algorithm>

#define NOMINMAX
#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>

#include <DirectXMath.h>
using namespace DirectX;

#include <imgui.h>

#include "Defines.h"
#include "Types.h"

#include "Debug/Debug.h"

#include "STL/String.h"

#include "Windows/Windows.h"

#include "Application/Log.h"

#include "Utilities/TUtilities.h"
#include "Utilities/HashUtilities.h"
#include "Utilities/MathUtilities.h"