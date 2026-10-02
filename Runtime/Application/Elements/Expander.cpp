#include "Application/Elements/Expander.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DisclosureGlyph.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"
constexpr int32 EXPANDER_DISCLOSURE_SIZE    = 8;
constexpr int32 EXPANDER_DISCLOSURE_SPACING = 4;

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
    , Style()
    , ExpandedArrow()
    , CollapsedArrow()
    , ArrowSize(16)
    , HeaderHeight(FUIStyle::GetDefault().Metrics.RowHeight)
    , AnimationStartHeight(0)
    , ExpandAnimation()
    , AnimationAlpha(1.0f)
    , bIsExpanded(true)
    , bIsHeaderHovered(false)
    , bDrawBottomBorderWhenClosed(false)
    , OnStateChangedDelegate()
{
}

FExpander::~FExpander() = default;

void FExpander::Initialize(const FDesc& Desc)
{
    Label                       = Desc.Label;
    Font                        = Desc.Font;
    ContentPadding              = Desc.ContentPadding;
    Style                       = Desc.Style;
    ExpandedArrow               = Desc.ExpandedArrow;
    CollapsedArrow              = Desc.CollapsedArrow;
    ArrowSize                   = Math::Max(1, Desc.ArrowSize);
    HeaderHeight                = Math::Max(1, Desc.HeaderHeight);
    bIsExpanded                 = Desc.bIsExpanded;
    bDrawBottomBorderWhenClosed = Desc.bDrawBottomBorderWhenClosed;
    OnStateChangedDelegate      = Desc.OnStateChanged;
    AnimationStartHeight        = HeaderHeight;
    AnimationAlpha              = 1.0f;
    ExpandAnimation.Start(Style.ExpandDuration, 0.0f, 1.0f);

    SetContent(Desc.Content);
}

IntVector2 FExpander::PrepareDesiredSize()
{
    AnimationAlpha = ComputeAnimationAlpha();

    return FVisualElement::PrepareDesiredSize();
}

IntVector2 FExpander::ComputeDesiredSize() const
{
    int32 HeaderWidth = Style.FramePadding.GetTotalHorizontal() + GetArrowExtent() + EXPANDER_DISCLOSURE_SPACING;
    if (Font && !Label.IsEmpty())
    {
        HeaderWidth += Font->MeasureWidth(StringView(Label.Data(), Label.Length()));
    }

    IntVector2 DesiredSize(HeaderWidth, GetDisplayedHeight());
    if (Content)
    {
        const IntVector2 ContentSize = Content->GetCachedDesiredSize();
        DesiredSize.X = Math::Max(DesiredSize.X, ContentSize.X + ContentPadding.GetTotalHorizontal());
    }

    return DesiredSize;
}

void FExpander::OnArrange(const FRectangle& AllottedBounds)
{
    if (AnimationAlpha < 1.0f)
    {
        InvalidateDesiredSize();
    }

    if (!IsContentShown() || !Content)
    {
        return;
    }

    const int32 HeaderRoom = Math::Min(HeaderHeight, AllottedBounds.Height);

    FRectangle ContentBounds = AllottedBounds;
    ContentBounds.Position.Y += HeaderRoom;
    ContentBounds.Height      = Math::Max(0, AllottedBounds.Height - HeaderRoom);

    Content->Arrange(ContentBounds.Deflate(ContentPadding));
}

EChildVisit FExpander::VisitChildren(FChildVisitor& Visitor, EChildOrder /*Order*/) const
{
    return VisitChild(Visitor, Content);
}

void FExpander::HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath)
{
    if (IsContentShown() && Content)
    {
        Content->HitTest(ClientPosition, OutPath);
    }
}

int32 FExpander::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle PanelBounds = AllottedGeometry.Bounds;
    const FCornerRadii Radii(Style.CornerRadius);

    OutCommandList.AddBox(LayerId, PanelBounds, Style.Fill, Radii);
    if (Style.BorderThickness > 0.0f)
    {
        OutCommandList.AddBoxOutline(LayerId, PanelBounds, Style.Border, Style.BorderThickness, Radii);
    }

    const FRectangle HeaderBounds = GetHeaderBounds(PanelBounds);

    const FUIBrush&  ArrowBrush  = bIsExpanded ? ExpandedArrow : CollapsedArrow;
    const int32      ArrowExtent = GetArrowExtent();
    const IntVector2 ArrowPosition(
        HeaderBounds.Position.X + Style.FramePadding.Left,
        HeaderBounds.Position.Y + ((HeaderBounds.Height - ArrowExtent) / 2));

    const FRectangle ArrowBounds(ArrowPosition, ArrowExtent, ArrowExtent);
    if (ArrowBrush.IsValid())
    {
        OutCommandList.AddImage(LayerId + 1, ArrowBounds, ArrowBrush, Style.ArrowTint);
    }
    else
    {
        FDisclosureGlyph::Draw(OutCommandList, LayerId + 1, ArrowBounds, bIsExpanded, Style.ArrowTint);
    }

    if (Font && !Label.IsEmpty())
    {
        const int32      LabelLeft  = ArrowBounds.GetRight() + EXPANDER_DISCLOSURE_SPACING;
        const int32      LabelRight = HeaderBounds.GetRight() - Style.FramePadding.Right;
        const IntVector2 LabelPosition(LabelLeft, HeaderBounds.Position.Y + Font->GetTextBandOffset(HeaderBounds.Height));
        const FRectangle LabelBounds(LabelPosition, Math::Max(LabelRight - LabelLeft, 0), Font->GetTextBandHeight());

        OutCommandList.AddText(LayerId + 1, LabelBounds, Label, Font.Get(), FUIStyle::GetDefault().Colors.Text);
    }

    if (!IsContentShown() || !Content)
    {
        return LayerId + 1;
    }

    OutCommandList.PushClip(LayerId + 2, PanelBounds);

    const FDrawGeometry ContentGeometry(Content->GetContentRectangle(), AllottedGeometry.Scale);
    const int32         NextLayerId = Content->Draw(ContentGeometry, OutCommandList, LayerId + 3);

    OutCommandList.PopClip(NextLayerId);
    return NextLayerId;
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

FEventResponse FExpander::OnMouseMove(const FCursorEvent& CursorEvent)
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

bool FExpander::GetCursor(ECursor& OutCursor) const
{
    if (!bIsHeaderHovered)
    {
        return false;
    }

    OutCursor = ECursor::Hand;
    return true;
}

void FExpander::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    Content = InContent;
    if (Content)
    {
        Content->SetParentElement(AsWeakPtr());
    }

    InvalidateDesiredSize();
}

void FExpander::SetExpanded(bool bInIsExpanded)
{
    if (bIsExpanded == bInIsExpanded)
    {
        return;
    }

    AnimationStartHeight = GetDisplayedHeight();
    AnimationAlpha       = 0.0f;
    bIsExpanded          = bInIsExpanded;
    ExpandAnimation.Start(Style.ExpandDuration, 0.0f, 1.0f);
    InvalidateDesiredSize();
    OnStateChangedDelegate.ExecuteIfBound(bIsExpanded);
}

void FExpander::SetLabel(const String& InLabel)
{
    if (Label != InLabel)
    {
        Label = InLabel;
        InvalidateDesiredSize();
    }
}

FRectangle FExpander::GetHeaderBounds(const FRectangle& AllottedBounds) const
{
    return FRectangle(AllottedBounds.Position, AllottedBounds.Width, Math::Min(HeaderHeight, AllottedBounds.Height));
}

int32 FExpander::GetArrowExtent() const
{
    const bool bHasBrush = ExpandedArrow.IsValid() || CollapsedArrow.IsValid();
    return bHasBrush ? ArrowSize : EXPANDER_DISCLOSURE_SIZE;
}

int32 FExpander::GetContentDesiredHeight() const
{
    if (!Content)
    {
        return 0;
    }

    return Content->GetCachedDesiredSize().Y + ContentPadding.GetTotalVertical();
}

int32 FExpander::GetSettledHeight() const
{
    return bIsExpanded ? (HeaderHeight + GetContentDesiredHeight()) : HeaderHeight;
}

int32 FExpander::GetDisplayedHeight() const
{
    const int32 Target = GetSettledHeight();
    if (Style.ExpandDuration <= 0.0f)
    {
        return Target;
    }

    const float Alpha = GetAnimationAlpha();
    if (Alpha >= 1.0f)
    {
        return Target;
    }

    return Math::RoundToInt(Math::Lerp(static_cast<float>(AnimationStartHeight), static_cast<float>(Target), Alpha));
}

float FExpander::ComputeAnimationAlpha() const
{
    if (Style.ExpandDuration <= 0.0f)
    {
        return 1.0f;
    }

    const float Linear = ExpandAnimation.GetProgress();
    return Linear * Linear * (3.0f - (2.0f * Linear));
}

bool FExpander::IsContentShown() const
{
    return GetDisplayedHeight() > HeaderHeight && Content != nullptr;
}
