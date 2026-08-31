#include "Application/Elements/Expander.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

constexpr int32 EXPANDER_HEADER_INSET       = 4;
constexpr int32 EXPANDER_DISCLOSURE_SIZE    = 8;
constexpr int32 EXPANDER_DISCLOSURE_SPACING = 4;

static void DrawDisclosureTriangle(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& Bounds, bool bIsExpanded, const FFloatColor& Tint)
{
    const float Left   = static_cast<float>(Bounds.Position.X);
    const float Top    = static_cast<float>(Bounds.Position.Y);
    const float Right  = static_cast<float>(Bounds.GetRight());
    const float Bottom = static_cast<float>(Bounds.GetBottom());

    Vector2 Corners[3];
    if (bIsExpanded)
    {
        const float CenterX = (Left + Right) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, Top);
        Corners[2] = Vector2(CenterX, Bottom);
    }
    else
    {
        const float CenterY = (Top + Bottom) * 0.5f;
        Corners[0] = Vector2(Left, Top);
        Corners[1] = Vector2(Right, CenterY);
        Corners[2] = Vector2(Left, Bottom);
    }

    OutCommandList.AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}

TSharedPtr<FExpander> FExpander::Create(const FDesc& Desc)
{
    TSharedPtr<FExpander> NewExpander = MakeSharedPtr<FExpander>();
    NewExpander->Initialize(Desc);
    return NewExpander;
}

FExpander::FExpander()
    : FVisualElement()
    , Content(nullptr)
    , Font(nullptr)
    , Label()
    , ContentPadding(12, 4, 4, 4)
    , HeaderHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , bIsExpanded(true)
    , bIsHeaderHovered(false)
    , OnStateChangedDelegate()
{
}

FExpander::~FExpander() = default;

void FExpander::Initialize(const FDesc& Desc)
{
    Label                  = Desc.Label;
    Font                   = Desc.Font;
    ContentPadding         = Desc.ContentPadding;
    HeaderHeight           = Math::Max(1, Desc.HeaderHeight);
    bIsExpanded            = Desc.bIsExpanded;
    OnStateChangedDelegate = Desc.OnStateChanged;

    SetContent(Desc.Content);
}

IntVector2 FExpander::ComputeDesiredSize() const
{
    int32 HeaderWidth = (EXPANDER_HEADER_INSET * 2) + EXPANDER_DISCLOSURE_SIZE + EXPANDER_DISCLOSURE_SPACING;
    if (Font && !Label.IsEmpty())
    {
        HeaderWidth += Font->MeasureWidth(StringView(Label.Data(), Label.Length()));
    }

    IntVector2 DesiredSize(HeaderWidth, HeaderHeight);
    if (bIsExpanded && Content)
    {
        const IntVector2 ContentSize = Content->GetCachedDesiredSize();
        DesiredSize.X = Math::Max(DesiredSize.X, ContentSize.X + ContentPadding.GetTotalHorizontal());
        DesiredSize.Y += ContentSize.Y + ContentPadding.GetTotalVertical();
    }

    return DesiredSize;
}

void FExpander::OnArrange(const FRectangle& AllottedBounds)
{
    if (!bIsExpanded || !Content)
    {
        return;
    }

    const int32 HeaderRoom = Math::Min(HeaderHeight, AllottedBounds.Height);

    FRectangle ContentBounds = AllottedBounds;
    ContentBounds.Position.Y += HeaderRoom;
    ContentBounds.Height      = AllottedBounds.Height - HeaderRoom;

    Content->Tick(ContentBounds.Deflate(ContentPadding));
}

void FExpander::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Content)
    {
        OutChildren.Add(Content);
    }
}

void FExpander::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (bIsExpanded && Content)
    {
        Content->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

int32 FExpander::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style        = FUIStyle::GetDefault();
    const FRectangle HeaderBounds = GetHeaderBounds(AllottedGeometry.Bounds);

    const FFloatColor& HeaderFill = bIsHeaderHovered ? Style.Colors.ControlHovered : Style.Colors.ControlNormal;
    OutCommandList.AddBox(LayerId, HeaderBounds, HeaderFill, FCornerRadii(Style.Metrics.CornerRadius));

    const IntVector2 DisclosurePosition(
        HeaderBounds.Position.X + EXPANDER_HEADER_INSET,
        HeaderBounds.Position.Y + ((HeaderBounds.Height - EXPANDER_DISCLOSURE_SIZE) / 2));

    const FRectangle DisclosureBounds(DisclosurePosition, EXPANDER_DISCLOSURE_SIZE, EXPANDER_DISCLOSURE_SIZE);
    DrawDisclosureTriangle(OutCommandList, LayerId + 1, DisclosureBounds, bIsExpanded, Style.Colors.Text);

    if (Font && !Label.IsEmpty())
    {
        const int32      LabelLeft = DisclosureBounds.GetRight() + EXPANDER_DISCLOSURE_SPACING;
        const IntVector2 LabelPosition(LabelLeft, HeaderBounds.Position.Y + Font->GetTextBandOffset(HeaderBounds.Height));
        const FRectangle LabelBounds(LabelPosition, Math::Max(HeaderBounds.GetRight() - EXPANDER_HEADER_INSET - LabelLeft, 0), Font->GetTextBandHeight());

        OutCommandList.AddText(LayerId + 1, LabelBounds, Label, Font.Get(), Style.Colors.Text);
    }

    if (!bIsExpanded || !Content)
    {
        return LayerId + 1;
    }

    const FDrawGeometry ContentGeometry(Content->GetContentRectangle(), AllottedGeometry.Scale);
    return Content->OnDraw(ContentGeometry, OutCommandList, LayerId + 2);
}

FEventResponse FExpander::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    if (!GetHeaderBounds(GetContentRectangle()).EncapsulatesPoint(CursorEvent.GetClientPosition()))
    {
        return FEventResponse::Unhandled();
    }

    SetExpanded(!bIsExpanded);
    return FEventResponse::Handled();
}

FEventResponse FExpander::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    bIsHeaderHovered = GetHeaderBounds(GetContentRectangle()).EncapsulatesPoint(CursorEvent.GetClientPosition());
    return FEventResponse::Unhandled();
}

FEventResponse FExpander::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    bIsHeaderHovered = false;
    return FEventResponse::Unhandled();
}

void FExpander::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    Content = InContent;
    if (Content)
    {
        Content->SetParentElement(AsWeakPtr());
    }
}

void FExpander::SetExpanded(bool bInIsExpanded)
{
    if (bIsExpanded == bInIsExpanded)
    {
        return;
    }

    bIsExpanded = bInIsExpanded;
    OnStateChangedDelegate.ExecuteIfBound(bIsExpanded);
}

void FExpander::SetLabel(const String& InLabel)
{
    Label = InLabel;
}

FRectangle FExpander::GetHeaderBounds(const FRectangle& AllottedBounds) const
{
    return FRectangle(AllottedBounds.Position, AllottedBounds.Width, Math::Min(HeaderHeight, AllottedBounds.Height));
}
