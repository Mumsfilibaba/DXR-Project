#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Core/Math/Vector2.h"
#include "Application/Elements/VisualElement.h"

enum class EGraphPinDirection : uint8
{
    Input,
    Output,
};

struct FGraphPin
{
    FGraphPin()
        : PinId(-1)
        , Direction(EGraphPinDirection::Input)
        , Name()
        , TypeTag()
        , Tint(FFloatColor::White)
    {
    }

    FGraphPin(EGraphPinDirection InDirection, const String& InName, const String& InTypeTag, const FFloatColor& InTint)
        : PinId(-1)
        , Direction(InDirection)
        , Name(InName)
        , TypeTag(InTypeTag)
        , Tint(InTint)
    {
    }

    /** @brief Unique across the whole model, and assigned by it when the pin arrives without one. */
    int32 PinId;

    /** @brief Which side of its node the pin sits on. */
    EGraphPinDirection Direction;

    /** @brief The text drawn beside the pin. */
    String Name;

    /** @brief Gates which pins may connect. A render graph tags by resource kind, a shader graph by value type. */
    String TypeTag;

    /** @brief The color of the pin and of every link leaving it, usually derived from TypeTag. */
    FFloatColor Tint;
};

struct FGraphNode
{
    FGraphNode()
        : NodeId(-1)
        , Position(0.0f, 0.0f)
        , Title()
        , TitleTint(FFloatColor::White)
        , bIsMuted(false)
        , Pins()
        , Content(nullptr)
    {
    }

    /** @brief Unique across the whole model, and assigned by it when the node arrives without one. */
    int32 NodeId;

    /** @brief The top-left corner, in graph space, which zoom and pan turn into a client rectangle. */
    Vector2 Position;

    /** @brief The text the title bar shows. */
    String Title;

    /** @brief The color of the title bar, which is how a graph groups nodes by kind. */
    FFloatColor TitleTint;

    /** @brief Drawn dimmed, which is how a culled render graph pass reads. */
    bool bIsMuted;

    /** @brief The connection points, in the order they are laid out down each side. */
    TArray<FGraphPin> Pins;

    /** @brief Optional body content, which is what lets a shader graph node carry inline value editors. */
    TSharedPtr<FVisualElement> Content;
};

struct FGraphLink
{
    FGraphLink()
        : LinkId(-1)
        , FromPinId(-1)
        , ToPinId(-1)
    {
    }

    FGraphLink(int32 InLinkId, int32 InFromPinId, int32 InToPinId)
        : LinkId(InLinkId)
        , FromPinId(InFromPinId)
        , ToPinId(InToPinId)
    {
    }

    /** @brief Unique across the whole model, and assigned by it as the link is made. */
    int32 LinkId;

    /** @brief The output pin the link leaves. */
    int32 FromPinId;

    /** @brief The input pin the link arrives at. */
    int32 ToPinId;
};
