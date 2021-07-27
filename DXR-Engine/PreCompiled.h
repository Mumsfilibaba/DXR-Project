#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

/* Windows Specific */
#ifdef PLATFORM_WINDOWS
#define NOMINMAX
#include "Core/Windows/Windows.h"

#include <dxgi1_6.h>
#include <d3d12.h>

#include <wrl/client.h>
#endif

#include <imgui.h>

/* Debug */
#include "Core/Debug/Debug.h"

/* Core */
#include "Core.h"
#include "Core/CoreObject/ClassType.h"
#include "Core/CoreObject/CoreObject.h"
#include "Core/RefCountedObject.h"

/* Containers */
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/FixedArray.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"

/* Templates */
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/AddressOf.h"
#include "Core/Templates/ObjectHandling.h"

/* Application */
#include "Core/Application/Log.h"

/* Utilities */
#include "Core/Utilities/HashUtilities.h"
#include "Core/Utilities/StringUtilities.h"

/* Math */
#include "Core/Math/Math.h"
#include "Core/Math/AABB.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Color.h"
#include "Core/Math/Float.h"
#include "Core/Math/IntPoint2.h"
#include "Core/Math/IntPoint3.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Matrix2.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Plane.h"
#include "Core/Math/SIMD.h"

/* Memory */
#include "Core/Memory/Memory.h"
#include "Core/Memory/New.h"