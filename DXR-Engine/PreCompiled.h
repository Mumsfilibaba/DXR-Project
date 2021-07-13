#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

/* Windows Specific */
#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include "Windows/Windows.h"

#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>
#endif

#include <imgui.h>

/* Debug */
#include "Debug/Debug.h"

/* Core */
#include "Core.h"
#include "Core/Types.h"
#include "Core/CoreObject/ClassType.h"
#include "Core/CoreObject/CoreObject.h"
#include "Core/RefCountedObject.h"

/* Containers */
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Containers/Utilities.h"

/* Application */
#include "Core/Application/Log.h"

/* Utilities */
#include "Utilities/HashUtilities.h"
#include "Utilities/StringUtilities.h"

/* Math */
#include "Math/Math.h"
#include "Math/AABB.h"
#include "Math/Frustum.h"
#include "Math/Color.h"
#include "Math/Float.h"
#include "Math/IntPoint2.h"
#include "Math/IntPoint3.h"
#include "Math/MathHash.h"
#include "Math/Matrix2.h"
#include "Math/Matrix3.h"
#include "Math/Matrix4.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/Plane.h"
#include "Math/SIMD.h"

/* Memory */
#include "Memory/Memory.h"
#include "Memory/New.h"