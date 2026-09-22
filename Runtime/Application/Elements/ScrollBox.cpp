#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/ScrollBar.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

TSharedPtr<FScrollBox> FScrollBox::Create()
{
    TSharedPtr<FScrollBox> NewScrollBox = MakeSharedPtr<FScrollBox>();
    NewScrollBox->Initialize();
    return NewScrollBox;
}

FScrollBox::FScrollBox()
    : FCompoundElement()
    , ScrollBar(nullptr)
    , ScrollBarVisibility(EScrollBarVisibility::Auto)
    , ScrollOffset(0)
    , ScrollAmountPerWheelStep(DefaultScrollAmountPerWheelStep)
    , ContentHeight(0)
    , ViewHeight(0)
    , ScrollBarGutter(0)
    , bIsScrollToEndPending(false)
{
}

FScrollBox::~FScrollBox() = default;

void FScrollBox::Initialize()
{
    FScrollBar::FDesc BarDesc;
    BarDesc.Orientation     = EOrientation::Vertical;
    BarDesc.bAutoHide       = true;
    BarDesc.OnOffsetChanged = FOnScrollBarOffsetChanged::CreateRaw(this, &FScrollBox::SetScrollOffset);

    ScrollBar = FScrollBar::Create(BarDesc);
    ScrollBar->SetParentElement(AsWeakPtr());
}

IntVector2 FScrollBox::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(Padding.GetTotalHorizontal(), Padding.GetTotalVertical());
    if (Content)
    {
        DesiredSize.X += Content->GetCachedDesiredSize().X;
    }

    if (IsScrollBarVisible())
    {
        DesiredSize.X += ScrollBar->GetCachedDesiredSize().X;
    }

    return DesiredSize;
}

void FScrollBox::OnArrange(const FRectangle& AllottedBounds)
{
    const FRectangle ViewBounds = GetViewBounds(AllottedBounds);

    ViewHeight    = ViewBounds.Height;
    ContentHeight = Content ? Content->GetCachedDesiredSize().Y : 0;

    if (bIsScrollToEndPending)
    {
        ScrollOffset          = GetMaxScrollOffset();
        bIsScrollToEndPending = false;
    }

    ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());

    if (Content)
    {
        FRectangle ChildBounds = ViewBounds;
        ChildBounds.Position.Y -= ScrollOffset;
        ChildBounds.Height      = Math::Max(ContentHeight, ViewBounds.Height);
        Content->Tick(ChildBounds);
    }

    if (ScrollBar)
    {
        ScrollBar->SetScrollState(ContentHeight, ViewHeight, ScrollOffset);
    }

    if (IsScrollBarVisible())
    {
        const FRectangle Inner = AllottedBounds.Deflate(Padding);

        const int32 BarWidth = Math::Max(Inner.GetRight() - ViewBounds.GetRight() - ScrollBarGutter, 0);

        FRectangle BarBounds = ViewBounds;
        BarBounds.Width      = BarWidth;
        BarBounds.Position.X = Inner.GetRight() - BarWidth;

        ScrollBar->Tick(BarBounds);
    }
}

void FScrollBox::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    FCompoundElement::GetChildren(OutChildren);

    if (ScrollBar)
    {
        OutChildren.Add(ScrollBar);
    }
}

int32 FScrollBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    OutCommandList.PushClip(LayerId, GetViewBounds(AllottedGeometry.Bounds));

    int32 MaxLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
    OutCommandList.PopClip(MaxLayerId);

    if (IsScrollBarVisible())
    {
        const FDrawGeometry BarGeometry(ScrollBar->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = ScrollBar->OnDraw(BarGeometry, OutCommandList, MaxLayerId + 1);
    }

    return MaxLayerId;
}

void FScrollBox::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (IsScrollBarVisible() && ScrollBar->GetContentRectangle().EncapsulatesPoint(ClientPosition))
    {
        ScrollBar->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        return;
    }

    if (Content && GetViewBounds(GetContentRectangle()).EncapsulatesPoint(ClientPosition))
    {
        Content->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

FEventResponse FScrollBox::OnMouseScroll(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetScrollAxis() != EScrollAxis::Vertical)
    {
        return FEventResponse::Unhandled();
    }

    const int32 MaxScrollOffset = GetMaxScrollOffset();
    if (MaxScrollOffset <= 0)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Delta = static_cast<int32>(CursorEvent.GetScrollDelta() * static_cast<float>(ScrollAmountPerWheelStep));
    ScrollOffset = Math::Clamp(ScrollOffset - Delta, 0, MaxScrollOffset);
    return FEventResponse::Handled();
}

FEventResponse FScrollBox::OnMouseEntered(const FCursorEvent& CursorEvent)
{
    if (ScrollBar)
    {
        ScrollBar->SetRevealed(true);
    }

    return FCompoundElement::OnMouseEntered(CursorEvent);
}

FEventResponse FScrollBox::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    if (ScrollBar)
    {
        ScrollBar->SetRevealed(false);
    }

    return FCompoundElement::OnMouseLeft(CursorEvent);
}

void FScrollBox::ScrollToEnd()
{
    bIsScrollToEndPending = true;
}

void FScrollBox::ScrollIntoView(const FRectangle& ContentRelativeBounds)
{
    if (ContentRelativeBounds.Position.Y < ScrollOffset)
    {
        ScrollOffset = ContentRelativeBounds.Position.Y;
    }
    else if (ContentRelativeBounds.GetBottom() > (ScrollOffset + ViewHeight))
    {
        ScrollOffset = ContentRelativeBounds.GetBottom() - ViewHeight;
    }

    ScrollOffset = Math::Clamp(ScrollOffset, 0, GetMaxScrollOffset());
}

void FScrollBox::SetScrollOffset(int32 InScrollOffset)
{
    ScrollOffset = Math::Clamp(InScrollOffset, 0, GetMaxScrollOffset());
}

int32 FScrollBox::GetMaxScrollOffset() const
{
    return Math::Max(0, ContentHeight - ViewHeight);
}

bool FScrollBox::IsScrolledToEnd() const
{
    return ScrollOffset >= GetMaxScrollOffset();
}

void FScrollBox::SetScrollAmountPerWheelStep(int32 InAmount)
{
    ScrollAmountPerWheelStep = Math::Max(1, InAmount);
}

void FScrollBox::SetScrollBarVisibility(EScrollBarVisibility InVisibility)
{
    if (ScrollBarVisibility != InVisibility)
    {
        ScrollBarVisibility = InVisibility;
        InvalidateDesiredSize();
    }
}

void FScrollBox::SetScrollBarGutter(int32 InGutter)
{
    const int32 NewGutter = Math::Max(0, InGutter);
    if (ScrollBarGutter != NewGutter)
    {
        ScrollBarGutter = NewGutter;
        InvalidateDesiredSize();
    }
}

bool FScrollBox::IsScrollBarVisible() const
{
    if (!ScrollBar || ScrollBarVisibility == EScrollBarVisibility::Never)
    {
        return false;
    }

    return ScrollBarVisibility == EScrollBarVisibility::Always || GetMaxScrollOffset() > 0;
}

FRectangle FScrollBox::GetViewBounds(const FRectangle& AllottedBounds) const
{
    FRectangle ViewBounds = AllottedBounds.Deflate(Padding);

    if (IsScrollBarVisible())
    {
        ViewBounds.Width = Math::Max(ViewBounds.Width - ScrollBar->GetCachedDesiredSize().X - ScrollBarGutter, 0);
    }

    return ViewBounds;
}
