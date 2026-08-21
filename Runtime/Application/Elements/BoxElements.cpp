#include "Application/Elements/BoxElements.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Core/Math/Math.h"

FBoxElement::FBoxElement()
    : FVisualElement()
    , Slots()
{
}

FBoxElement::~FBoxElement() = default;

void FBoxElement::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const FBoxSlot& Slot : Slots)
    {
        if (Slot.Element)
        {
            OutChildren.Add(Slot.Element);
        }
    }
}

int32 FBoxElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    int32 MaxLayerId = LayerId;
    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        const FDrawGeometry ChildGeometry(Slot.Element->GetContentRectangle(), AllottedGeometry.Scale);
        MaxLayerId = Math::Max(MaxLayerId, Slot.Element->OnDraw(ChildGeometry, OutCommandList, LayerId + 1));
    }

    return MaxLayerId;
}

void FBoxElement::FindChildrenContainingPoint(const IntVector2& ScreenCursorPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ScreenCursorPosition, OutChildElements);

    for (const FBoxSlot& Slot : Slots)
    {
        if (Slot.Element)
        {
            Slot.Element->FindChildrenContainingPoint(ScreenCursorPosition, OutChildElements);
        }
    }
}

FBoxSlot& FBoxElement::AddSlot(const TSharedPtr<FVisualElement>& InElement)
{
    FBoxSlot& Slot = Slots.Emplace();
    Slot.Element   = InElement;

    if (InElement)
    {
        InElement->SetParentElement(AsWeakPtr());
    }

    return Slot;
}

void FBoxElement::ClearSlots()
{
    Slots.Clear();
}

FRectangle FBoxElement::ArrangeInSlot(const FRectangle& SlotBounds, const IntVector2& ChildDesiredSize, const FBoxSlot& Slot)
{
    const FRectangle Available = SlotBounds.Deflate(Slot.Padding);

    FRectangle Result = Available;

    switch (Slot.HorizontalAlignment)
    {
        case EHorizontalAlignment::Left:
        {
            Result.Width = Math::Min(ChildDesiredSize.X, Available.Width);
            break;
        }
        case EHorizontalAlignment::Center:
        {
            Result.Width      = Math::Min(ChildDesiredSize.X, Available.Width);
            Result.Position.X = Available.Position.X + ((Available.Width - Result.Width) / 2);
            break;
        }
        case EHorizontalAlignment::Right:
        {
            Result.Width      = Math::Min(ChildDesiredSize.X, Available.Width);
            Result.Position.X = Available.GetRight() - Result.Width;
            break;
        }
        default:
        {
            break;
        }
    }

    switch (Slot.VerticalAlignment)
    {
        case EVerticalAlignment::Top:
        {
            Result.Height = Math::Min(ChildDesiredSize.Y, Available.Height);
            break;
        }
        case EVerticalAlignment::Center:
        {
            Result.Height     = Math::Min(ChildDesiredSize.Y, Available.Height);
            Result.Position.Y = Available.Position.Y + ((Available.Height - Result.Height) / 2);
            break;
        }
        case EVerticalAlignment::Bottom:
        {
            Result.Height     = Math::Min(ChildDesiredSize.Y, Available.Height);
            Result.Position.Y = Available.GetBottom() - Result.Height;
            break;
        }
        default:
        {
            break;
        }
    }

    return Result;
}

int32 FBoxElement::CountFillSlots() const
{
    int32 Count = 0;
    for (const FBoxSlot& Slot : Slots)
    {
        if (Slot.Element && Slot.IsFillSlot())
        {
            Count++;
        }
    }

    return Count;
}

TSharedPtr<FVerticalBoxElement> FVerticalBoxElement::Create()
{
    return MakeSharedPtr<FVerticalBoxElement>();
}

FVerticalBoxElement::FVerticalBoxElement()
    : FBoxElement()
{
}

FVerticalBoxElement::~FVerticalBoxElement() = default;

IntVector2 FVerticalBoxElement::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(0, 0);
    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        const IntVector2 ChildSize = Slot.Element->GetCachedDesiredSize();
        DesiredSize.X  = Math::Max(DesiredSize.X, ChildSize.X + Slot.Padding.GetTotalHorizontal());
        DesiredSize.Y += ChildSize.Y + Slot.Padding.GetTotalVertical();
    }

    return DesiredSize;
}

void FVerticalBoxElement::OnArrange(const FRectangle& AllottedBounds)
{
    int32 AutoHeightTotal      = 0;
    float FillCoefficientTotal = 0.0f;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        if (Slot.IsFillSlot())
        {
            FillCoefficientTotal += Slot.FillCoefficient;
        }
        else
        {
            AutoHeightTotal += Slot.Element->GetCachedDesiredSize().Y + Slot.Padding.GetTotalVertical();
        }
    }

    const int32 RemainingHeight = Math::Max(0, AllottedBounds.Height - AutoHeightTotal);
    const int32 NumFillSlots    = CountFillSlots();

    int32 CurrentY        = AllottedBounds.Position.Y;
    int32 DistributedFill = 0;
    int32 FillSlotsSeen   = 0;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        int32 SlotHeight = 0;
        if (Slot.IsFillSlot())
        {
            FillSlotsSeen++;

            // The last filling slot absorbs the rounding remainder so the slots always add up
            if (FillSlotsSeen == NumFillSlots)
            {
                SlotHeight = RemainingHeight - DistributedFill;
            }
            else
            {
                SlotHeight = static_cast<int32>((static_cast<float>(RemainingHeight) * Slot.FillCoefficient) / FillCoefficientTotal);
            }

            DistributedFill += SlotHeight;
        }
        else
        {
            SlotHeight = Slot.Element->GetCachedDesiredSize().Y + Slot.Padding.GetTotalVertical();
        }

        FRectangle SlotBounds;
        SlotBounds.Position.X = AllottedBounds.Position.X;
        SlotBounds.Position.Y = CurrentY;
        SlotBounds.Width      = AllottedBounds.Width;
        SlotBounds.Height     = SlotHeight;

        Slot.Element->Tick(ArrangeInSlot(SlotBounds, Slot.Element->GetCachedDesiredSize(), Slot));

        CurrentY += SlotHeight;
    }
}

TSharedPtr<FHorizontalBoxElement> FHorizontalBoxElement::Create()
{
    return MakeSharedPtr<FHorizontalBoxElement>();
}

FHorizontalBoxElement::FHorizontalBoxElement()
    : FBoxElement()
{
}

FHorizontalBoxElement::~FHorizontalBoxElement() = default;

IntVector2 FHorizontalBoxElement::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(0, 0);
    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        const IntVector2 ChildSize = Slot.Element->GetCachedDesiredSize();
        DesiredSize.X += ChildSize.X + Slot.Padding.GetTotalHorizontal();
        DesiredSize.Y  = Math::Max(DesiredSize.Y, ChildSize.Y + Slot.Padding.GetTotalVertical());
    }

    return DesiredSize;
}

void FHorizontalBoxElement::OnArrange(const FRectangle& AllottedBounds)
{
    int32 AutoWidthTotal       = 0;
    float FillCoefficientTotal = 0.0f;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        if (Slot.IsFillSlot())
        {
            FillCoefficientTotal += Slot.FillCoefficient;
        }
        else
        {
            AutoWidthTotal += Slot.Element->GetCachedDesiredSize().X + Slot.Padding.GetTotalHorizontal();
        }
    }

    const int32 RemainingWidth = Math::Max(0, AllottedBounds.Width - AutoWidthTotal);
    const int32 NumFillSlots   = CountFillSlots();

    int32 CurrentX        = AllottedBounds.Position.X;
    int32 DistributedFill = 0;
    int32 FillSlotsSeen   = 0;

    for (const FBoxSlot& Slot : Slots)
    {
        if (!Slot.Element)
        {
            continue;
        }

        int32 SlotWidth = 0;
        if (Slot.IsFillSlot())
        {
            FillSlotsSeen++;

            // The last filling slot absorbs the rounding remainder so the slots always add up
            if (FillSlotsSeen == NumFillSlots)
            {
                SlotWidth = RemainingWidth - DistributedFill;
            }
            else
            {
                SlotWidth = static_cast<int32>((static_cast<float>(RemainingWidth) * Slot.FillCoefficient) / FillCoefficientTotal);
            }

            DistributedFill += SlotWidth;
        }
        else
        {
            SlotWidth = Slot.Element->GetCachedDesiredSize().X + Slot.Padding.GetTotalHorizontal();
        }

        FRectangle SlotBounds;
        SlotBounds.Position.X = CurrentX;
        SlotBounds.Position.Y = AllottedBounds.Position.Y;
        SlotBounds.Width      = SlotWidth;
        SlotBounds.Height     = AllottedBounds.Height;

        Slot.Element->Tick(ArrangeInSlot(SlotBounds, Slot.Element->GetCachedDesiredSize(), Slot));

        CurrentX += SlotWidth;
    }
}
