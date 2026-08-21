#pragma once
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Application/Layout/LayoutTypes.h"

struct IFontFace;

enum class EDrawCommandType : uint8
{
    /** A filled axis-aligned rectangle. */
    Box,

    /** A run of text on one line, anchored at the top-left of the command bounds. */
    Text,

    /** A thin axis-aligned rule, used for the text cursor and for separators. */
    Line,

    /** Opens a clip region. A well formed list matches every push with exactly one pop. */
    ClipPush,

    /** Closes the region opened by the matching ClipPush. */
    ClipPop,
};

struct FDrawGeometry
{
    FDrawGeometry()
        : Bounds()
        , Scale(1.0f)
    {
    }

    FDrawGeometry(const FRectangle& InBounds, float InScale)
        : Bounds(InBounds)
        , Scale(InScale)
    {
    }

    FRectangle Bounds;
    float      Scale;
};

struct FDrawCommand
{
    FDrawCommand()
        : Type(EDrawCommandType::Box)
        , Bounds()
        , Tint()
        , Text()
        , Font(nullptr)
        , LayerId(0)
    {
    }

    EDrawCommandType Type;
    FRectangle       Bounds;
    FFloatColor      Tint;
    String           Text;
    const IFontFace* Font;
    int32            LayerId;
};
