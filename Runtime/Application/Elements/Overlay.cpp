#include "Application/Elements/Overlay.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

TSharedPtr<FOverlay> FOverlay::Create()
{
    return MakeSharedPtr<FOverlay>();
}

FOverlay::FOverlay()
    : FVisualElement()
    , Slots()
{
}

FOverlay::~FOverlay() = default;

IntVector2 FOverlay::ComputeDesiredSize() const
{
    IntVector2 DesiredSize;
    for (const FOverlaySlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        const IntVector2 ChildSize = Slot.Element->GetCachedDesiredSize();
        DesiredSize.X              = Math::Max(DesiredSize.X, ChildSize.X + Slot.Padding.GetTotalHorizontal());
        DesiredSize.Y              = Math::Max(DesiredSize.Y, ChildSize.Y + Slot.Padding.GetTotalVertical());
    }

    return DesiredSize;
}

void FOverlay::OnArrange(const FRectangle& AllottedBounds)
{
    for (const FOverlaySlot& Slot : Slots)
    {
        if (Slot.Element)
        {
            Slot.Element->Tick(FRectangle::AlignInBounds(AllottedBounds.Deflate(Slot.Padding), Slot.Element->GetCachedDesiredSize(), Slot.HorizontalAlignment, Slot.VerticalAlignment));
        }
    }
}

void FOverlay::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const FOverlaySlot& Slot : Slots)
    {
        if (Slot.Element)
        {
            OutChildren.Add(Slot.Element);
        }
    }
}

int32 FOverlay::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 MaxLayerId = LayerId;
    for (const FOverlaySlot& Slot : Slots)
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
        MaxLayerId = Slot.Element->OnDraw(ChildGeometry, OutCommandList, MaxLayerId + 1);
    }

    return MaxLayerId;
}

void FOverlay::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    for (int32 Index = Slots.Size() - 1; Index >= 0; --Index)
    {
        const FOverlaySlot& Slot = Slots[Index];
        if (!Slot.Element || !Slot.Element->IsVisible())
        {
            continue;
        }

        if (Slot.Element->GetContentRectangle().EncapsulatesPoint(ClientPosition))
        {
            Slot.Element->FindChildrenContainingPoint(ClientPosition, OutChildElements);
            return;
        }
    }
}

FOverlaySlot& FOverlay::AddSlot(const TSharedPtr<FVisualElement>& InElement)
{
    FOverlaySlot& Slot = Slots.Emplace();
    Slot.Element       = InElement;

    if (InElement)
    {
        InElement->SetParentElement(AsWeakPtr());
    }

    return Slot;
}

bool FOverlay::RemoveSlot(const TSharedPtr<FVisualElement>& InElement)
{
    for (int32 Index = 0; Index < Slots.Size(); ++Index)
    {
        if (Slots[Index].Element == InElement)
        {
            Slots.RemoveAt(Index);
            InvalidateDesiredSize();
            return true;
        }
    }

    return false;
}

void FOverlay::ClearSlots()
{
    Slots.Clear();
    InvalidateDesiredSize();
}
