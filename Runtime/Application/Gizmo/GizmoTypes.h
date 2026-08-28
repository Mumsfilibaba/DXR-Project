#pragma once
#include "Core/CoreTypes.h"

enum class EGizmoOperation : uint8
{
    Translate,
    Rotate,
    Scale,

    /** @brief Scale, where every handle scales all three axes by the same factor. */
    UniversalScale,
};

enum class EGizmoMode : uint8
{
    Local,
    World,
};

enum class EGizmoHandle : uint8
{
    None,

    TranslateX,
    TranslateY,
    TranslateZ,

    /** @brief The quad spanning the two axes that are not named, moving in that plane. */
    TranslateYZ,
    TranslateZX,
    TranslateXY,

    /** @brief The square at the pivot, moving in the plane facing the camera. */
    TranslateScreen,

    RotateX,
    RotateY,
    RotateZ,

    /** @brief The outer ring, turning about the axis pointing at the camera. */
    RotateScreen,

    ScaleX,
    ScaleY,
    ScaleZ,

    /** @brief The knob at the pivot, scaling all three axes together. */
    ScaleUniform,
};

struct FGizmoSnapSettings
{
    FGizmoSnapSettings()
        : TranslationSnap(0.0f)
        , RotationSnapDegrees(0.0f)
        , ScaleSnap(0.0f)
    {
    }

    /** @brief In world units. */
    float TranslationSnap;

    /** @brief In degrees turned about the active axis. */
    float RotationSnapDegrees;

    /** @brief In multiples of the original scale. */
    float ScaleSnap;
};

/**
 * @brief Gets whether a handle belongs to the translate set.
 *
 * @param Handle The handle to test.
 * @return True for one of the three axes, the three plane quads or the square at the pivot.
 */
NODISCARD FORCEINLINE bool IsTranslateHandle(EGizmoHandle Handle)
{
    return Handle >= EGizmoHandle::TranslateX && Handle <= EGizmoHandle::TranslateScreen;
}

/**
 * @brief Gets whether a handle belongs to the rotate set.
 *
 * @param Handle The handle to test.
 * @return True for one of the three axis rings or the outer ring.
 */
NODISCARD FORCEINLINE bool IsRotateHandle(EGizmoHandle Handle)
{
    return Handle >= EGizmoHandle::RotateX && Handle <= EGizmoHandle::RotateScreen;
}

/**
 * @brief Gets whether a handle belongs to the scale set.
 *
 * @param Handle The handle to test.
 * @return True for one of the three axis knobs or the knob at the pivot.
 */
NODISCARD FORCEINLINE bool IsScaleHandle(EGizmoHandle Handle)
{
    return Handle >= EGizmoHandle::ScaleX && Handle <= EGizmoHandle::ScaleUniform;
}

/**
 * @brief The axis a handle runs along, for the handles that name one.
 *
 * @param Handle The handle to read.
 * @return Zero for X, one for Y, two for Z, or -1 when the handle names no single axis.
 */
NODISCARD FORCEINLINE int32 GetHandleAxis(EGizmoHandle Handle)
{
    switch (Handle)
    {
        case EGizmoHandle::TranslateX:
        case EGizmoHandle::RotateX:
        case EGizmoHandle::ScaleX:
            return 0;

        case EGizmoHandle::TranslateY:
        case EGizmoHandle::RotateY:
        case EGizmoHandle::ScaleY:
            return 1;

        case EGizmoHandle::TranslateZ:
        case EGizmoHandle::RotateZ:
        case EGizmoHandle::ScaleZ:
            return 2;

        // A plane handle is named for the axis it is normal to, which is the one it does not span
        case EGizmoHandle::TranslateYZ:
            return 0;
        case EGizmoHandle::TranslateZX:
            return 1;
        case EGizmoHandle::TranslateXY:
            return 2;

        default:
            return -1;
    }
}
