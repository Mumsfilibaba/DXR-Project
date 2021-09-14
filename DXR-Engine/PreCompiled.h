#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

/* Windows Specific */
#if defined(PLATFORM_WINDOWS)
#include "Core/Windows/PreCompiledWindows.h"
#endif

#include <imgui.h>

/* Debug */
#include "Core/Debug/Debug.h"

/* Core */
#include "Core.h"
#include "Core/RefCounted.h"
#include "Core/CoreObject/ClassType.h"
#include "Core/CoreObject/CoreObject.h"

/* Containers */
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Containers/Function.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StaticString.h"
#include "Core/Containers/StringView.h"

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
#include "Core/Memory/New.h"
#include "Core/Memory/Memory.h"
