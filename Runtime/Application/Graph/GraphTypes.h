#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Core/Math/Vector2.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"

enum class EGraphPinDirection : uint8
{
    Input,
    Output,
};

/** @brief How far a muted node fades when the style leaves its muted colors at their defaults. */
constexpr float GRAPH_NODE_MUTED_OPACITY = 0.45f;

/**
 * @brief Fades a color the way a muted node dims by default.
 *
 * @param Color The color to fade.
 * @return The color with its alpha scaled by the muted factor.
 */
NODISCARD FORCEINLINE FFloatColor GraphMutedColor(const FFloatColor& Color)
{
    return FFloatColor(Color.R, Color.G, Color.B, Color.A * GRAPH_NODE_MUTED_OPACITY);
}

struct FGraphNodeStyle
{
    /** @brief The fill behind the pin rows. */
    FFloatColor Body = FUIStyle::GetDefault().Colors.PanelBackground;

    /** @brief The stroke around the node, which selecting it replaces with the accent. */
    FFloatColor Border = FUIStyle::GetDefault().Colors.Border;

    /** @brief The color of the title and of every pin label. */
    FFloatColor Text = FUIStyle::GetDefault().Colors.Text;

    /** @brief The fill of a node flagged muted, which is how a culled pass reads. */
    FFloatColor MutedBody = GraphMutedColor(FUIStyle::GetDefault().Colors.PanelBackground);

    /** @brief The stroke of a muted node. */
    FFloatColor MutedBorder = GraphMutedColor(FUIStyle::GetDefault().Colors.Border);

    /** @brief The title and pin label color of a muted node. */
    FFloatColor MutedText = GraphMutedColor(FUIStyle::GetDefault().Colors.Text);

    /** @brief The stroke around a pin circle, left transparent to draw the circle unstroked. */
    FFloatColor PinOutline = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

    /** @brief The rule between stacked input and output pins. */
    FFloatColor PinSeparator = FUIStyle::GetDefault().Colors.Border;

    /** @brief How far the corners are rounded, in graph space, so the radius tracks zoom. */
    float CornerRadius = FUIStyle::GetDefault().Metrics.CornerRadius;

    /**
     * @brief What a muted node's title tint and pin tints are scaled by, since those colors come from the
     * node rather than from here. A style that gives muted nodes a palette of their own sets this to one.
     */
    float MutedTintOpacity = GRAPH_NODE_MUTED_OPACITY;

    /** @brief True to stack the inputs above the outputs in one column rather than pairing them into rows. */
    bool bStackPinRows = false;
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
