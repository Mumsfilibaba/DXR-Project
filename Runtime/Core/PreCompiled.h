#pragma once
#include <cassert>
#include <utility>
#include <algorithm>
#include <random>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Windows Specific

#if PLATFORM_WINDOWS
#include "Core/Windows/PreCompiledWindows.h"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Debug

#include "Core/Debug/Debug.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Core

#include "Core/Core.h"
#include "Core/RefCounted.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Containers

#include "Core/Containers/BitArray.h"
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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Templates

#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/CallableWrapper.h"
#include "Core/Templates/ClassUtilities.h"
#include "Core/Templates/Conditional.h"
#include "Core/Templates/Decay.h"
#include "Core/Templates/DeclVal.h"
#include "Core/Templates/EnableIf.h"
#include "Core/Templates/EnumUtilities.h"
#include "Core/Templates/Identity.h"
#include "Core/Templates/InitializerList.h"
#include "Core/Templates/InPlace.h"
#include "Core/Templates/Invoke.h"
#include "Core/Templates/MemberPointerTraits.h"
#include "Core/Templates/Move.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Templates/ReferenceWrapper.h"
#include "Core/Templates/UnderlyingType.h"
#include "Core/Templates/AddressOf.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Misc

#include "Core/Misc/OutputDevice.h"
#include "Core/Misc/OutputDeviceConsole.h"
#include "Core/Misc/OutputDeviceLogger.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Utilities

#include "Core/Utilities/HashUtilities.h"
#include "Core/Utilities/StringUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Math

#include "Core/Math/Math.h"
#include "Core/Math/AABB.h"
#include "Core/Math/Frustum.h"
#include "Core/Math/Color.h"
#include "Core/Math/Float.h"
#include "Core/Math/IntVector2.h"
#include "Core/Math/IntVector3.h"
#include "Core/Math/MathHash.h"
#include "Core/Math/Matrix2.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Plane.h"
#include "Core/Math/VectorOp.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Memory 

#include "Core/Memory/New.h"
#include "Core/Memory/Memory.h"
