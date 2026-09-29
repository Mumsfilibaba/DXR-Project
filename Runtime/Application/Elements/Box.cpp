#include "Application/Elements/Box.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

FBox::FBox()
    : FVisualElement()
    , Slots()
{
}

FBox::~FBox() = default;

EChildVisit FBox::VisitChildren(FChildVisitor& Visitor, EChildOrder Order) const
{
    return VisitChildArray(Visitor, Order, Slots, [](const FBoxSlot& Slot) -> const TSharedPtr<FVisualElement>&
    {
        return Slot.Element;
    });
}

int32 FBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 MaxLayerId = LayerId;
    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element || !Slot.Element->IsVisible())
        {
            continue;
        }

        const FRectangle  ChildBounds = Slot.Element->GetContentRectangle();
        const FRectangle& ClipBounds  = OutCommandList.GetCurrentClipRectangle();

        if (!ClipBounds.IsEmpty() && ClipBounds.Intersect(ChildBounds).IsEmpty())
        {
            continue;
        }

        const FDrawGeometry ChildGeometry(ChildBounds, AllottedGeometry.Scale);
        MaxLayerId = Slot.Element->Draw(ChildGeometry, OutCommandList, MaxLayerId + 1);
    }

    return MaxLayerId;
}

FBoxSlot& FBox::AddSlot(const TSharedPtr<FVisualElement>& InElement)
{
    FBoxSlot& Slot = Slots.Emplace();
    Slot.Element   = InElement;

    if (InElement)
    {
        InElement->SetParentElement(AsWeakPtr());
    }

    return Slot;
}

void FBox::ClearSlots()
{
    Slots.Clear();
    InvalidateDesiredSize();
}

void FBox::RemoveSlotAt(int32 Index)
{
    if (Index < 0 || Index >= Slots.Size())
    {
        return;
    }

    if (Slots[Index].Element)
    {
        Slots[Index].Element->SetParentElement(TWeakPtr<FVisualElement>());
    }

    Slots.RemoveAt(Index);
    InvalidateDesiredSize();
}

FRectangle FBox::ArrangeInSlot(const FRectangle& SlotBounds, const IntVector2& ChildDesiredSize, const FBoxSlot& Slot)
{
    return FLayout::AlignInBounds(SlotBounds.Deflate(Slot.Padding), ChildDesiredSize, Slot.HorizontalAlignment, Slot.VerticalAlignment);
}

TSharedPtr<FVerticalBox> FVerticalBox::Create()
{
    return MakeSharedPtr<FVerticalBox>();
}

FVerticalBox::FVerticalBox()
    : TStackBox<EOrientation::Vertical>()
{
}

FVerticalBox::~FVerticalBox() = default;

TSharedPtr<FHorizontalBox> FHorizontalBox::Create()
{
    return MakeSharedPtr<FHorizontalBox>();
}

FHorizontalBox::FHorizontalBox()
    : TStackBox<EOrientation::Horizontal>()
{
}

FHorizontalBox::~FHorizontalBox() = default;
