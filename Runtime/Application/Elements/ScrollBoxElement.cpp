#include "Application/Elements/ScrollBoxElement.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

TSharedPtr<FScrollBoxElement> FScrollBoxElement::Create()
{
    return MakeSharedPtr<FScrollBoxElement>();
}

FScrollBoxElement::FScrollBoxElement()
    : FCompoundElement()
    , ScrollOffset(0)
    , ScrollAmountPerWheelStep(DefaultScrollAmountPerWheelStep)
    , ContentHeight(0)
    , ViewHeight(0)
    , bIsScrollToEndPending(false)
{
}

FScrollBoxElement::~FScrollBoxElement() = default;

IntVector2 FScrollBoxElement::ComputeDesiredSize() const
{
    // A scroll box takes whatever it is given vertically, so it asks only for its own padding
    IntVector2 DesiredSize(Padding.GetTotalHorizontal(), Padding.GetTotalVertical());
    if (Content)
    {
        DesiredSize.X += Content->GetCachedDesiredSize().X;
    }

    return DesiredSize;
}

void FScrollBoxElement::OnArrange(const FRectangle& AllottedBounds)
{
    const FRectangle ViewBounds = AllottedBounds.Deflate(Padding);

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
}

int32 FScrollBoxElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    OutCommandList.PushClip(LayerId, AllottedGeometry.Bounds.Deflate(Padding));
    const int32 MaxLayerId = FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
    OutCommandList.PopClip(MaxLayerId);
    return MaxLayerId;
}

FEventResponse FScrollBoxElement::OnMouseScroll(const FCursorEvent& CursorEvent)
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

void FScrollBoxElement::ScrollToEnd()
{
    bIsScrollToEndPending = true;
}

void FScrollBoxElement::ScrollIntoView(const FRectangle& ContentRelativeBounds)
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

void FScrollBoxElement::SetScrollOffset(int32 InScrollOffset)
{
    ScrollOffset = Math::Clamp(InScrollOffset, 0, GetMaxScrollOffset());
}

int32 FScrollBoxElement::GetMaxScrollOffset() const
{
    return Math::Max(0, ContentHeight - ViewHeight);
}

bool FScrollBoxElement::IsScrolledToEnd() const
{
    return ScrollOffset >= GetMaxScrollOffset();
}

void FScrollBoxElement::SetScrollAmountPerWheelStep(int32 InAmount)
{
    ScrollAmountPerWheelStep = Math::Max(1, InAmount);
}
