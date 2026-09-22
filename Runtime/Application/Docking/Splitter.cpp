#include "Application/Application.h"
#include "Application/Docking/Splitter.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"

TSharedPtr<FSplitter> FSplitter::Create(const FDesc& Desc)
{
    TSharedPtr<FSplitter> NewSplitter = MakeSharedPtr<FSplitter>();
    NewSplitter->Initialize(Desc);
    return NewSplitter;
}

FSplitter::FSplitter()
    : FVisualElement()
    , Orientation(EDockSplitOrientation::Horizontal)
    , HandleThickness(FDockMetrics::SplitterThickness)
    , ActiveHandleIndex(-1)
    , HoveredHandleIndex(-1)
    , DragOrigin()
    , DragStartFractions()
    , DragStartFixedLengths()
    , Fractions()
    , FixedLengths()
    , MinimumSizes()
    , Children()
    , OnFractionsChangedDelegate()
{
}

FSplitter::~FSplitter() = default;

void FSplitter::Initialize(const FDesc& Desc)
{
    Orientation                = Desc.Orientation;
    HandleThickness            = Math::Max(1, Desc.HandleThickness);
    MinimumSizes               = Desc.MinimumSizes;
    FixedLengths               = Desc.FixedLengths;
    OnFractionsChangedDelegate = Desc.OnFractionsChanged;

    TryNormalizeFractions(Desc.Fractions);
}

IntVector2 FSplitter::ComputeDesiredSize() const
{
    IntVector2 DesiredSize(0, 0);
    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (!Child)
        {
            continue;
        }

        const IntVector2 ChildSize = Child->GetCachedDesiredSize();
        if (Orientation == EDockSplitOrientation::Horizontal)
        {
            DesiredSize.X += ChildSize.X;
            DesiredSize.Y = Math::Max(DesiredSize.Y, ChildSize.Y);
        }
        else
        {
            DesiredSize.X = Math::Max(DesiredSize.X, ChildSize.X);
            DesiredSize.Y += ChildSize.Y;
        }
    }

    const int32 HandleSpace = Math::Max(0, Children.Size() - 1) * HandleThickness;
    if (Orientation == EDockSplitOrientation::Horizontal)
    {
        DesiredSize.X += HandleSpace;
    }
    else
    {
        DesiredSize.Y += HandleSpace;
    }

    return DesiredSize;
}

void FSplitter::OnArrange(const FRectangle& AllottedBounds)
{
    if (Children.IsEmpty())
    {
        return;
    }

    const int32 Available     = GetAvailableLength(AllottedBounds);
    const bool  bIsHorizontal = Orientation == EDockSplitOrientation::Horizontal;

    int32 Offset    = bIsHorizontal ? AllottedBounds.Position.X : AllottedBounds.Position.Y;
    int32 Allocated = 0;

    for (int32 Index = 0; Index < Children.Size(); ++Index)
    {
        const bool  bIsLast     = Index == Children.Size() - 1;
        const int32 FixedLength = GetChildFixedLength(Index);

        int32 TrailingMin = 0;
        for (int32 After = Index + 1; After < Children.Size(); ++After)
        {
            TrailingMin += GetChildMinimumLength(After);
        }

        int32 Length = 0;
        if (FixedLength > 0)
        {
            Length = Math::Clamp(FixedLength, GetChildMinimumLength(Index),
                Math::Max(Available - Allocated - TrailingMin, GetChildMinimumLength(Index)));
        }
        else if (bIsLast)
        {
            Length = Available - Allocated;
        }
        else
        {
            const float Share = Index < Fractions.Size() ? Fractions[Index] : 0.0f;
            Length = Math::RoundToInt(static_cast<float>(Available) * Share);
        }

        Allocated += Length;

        if (Children[Index])
        {
            const FRectangle ChildBounds = bIsHorizontal
                ? FRectangle(IntVector2(Offset, AllottedBounds.Position.Y), Length, AllottedBounds.Height)
                : FRectangle(IntVector2(AllottedBounds.Position.X, Offset), AllottedBounds.Width, Length);

            Children[Index]->Tick(ChildBounds);
        }

        Offset += Length;

        if (!bIsLast)
        {
            Offset += HandleThickness;
        }
    }
}

void FSplitter::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child)
        {
            OutChildren.Add(Child);
        }
    }
}

int32 FSplitter::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    int32 NextLayerId = LayerId;
    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child && Child->IsVisible())
        {
            const FDrawGeometry ChildGeometry(Child->GetContentRectangle(), AllottedGeometry.Scale);
            NextLayerId = Child->OnDraw(ChildGeometry, OutCommandList, NextLayerId + 1);
        }
    }

    for (int32 HandleIndex = 0; HandleIndex < Children.Size() - 1; ++HandleIndex)
    {
        const bool bIsActive = HandleIndex == ActiveHandleIndex || HandleIndex == HoveredHandleIndex;
        if (!bIsActive)
        {
            continue;
        }

        const FFloatColor& Tint   = HandleIndex == ActiveHandleIndex ? Style.Colors.Accent : Style.Colors.SeparatorHovered;
        const FRectangle   Handle = GetHandleRectangle(HandleIndex);

        FRectangle Hint = Handle;
        if (Orientation == EDockSplitOrientation::Horizontal)
        {
            Hint.Width      = Math::Min(Style.Metrics.SplitterHintThickness, Handle.Width);
            Hint.Position.X = Handle.Position.X + ((Handle.Width - Hint.Width) / 2);
        }
        else
        {
            Hint.Height     = Math::Min(Style.Metrics.SplitterHintThickness, Handle.Height);
            Hint.Position.Y = Handle.Position.Y + ((Handle.Height - Hint.Height) / 2);
        }

        const FCornerRadii Radius(static_cast<float>(Math::Min(Hint.Width, Hint.Height)) * 0.5f);
        OutCommandList.AddBox(NextLayerId, Hint, Tint, Radius);
    }

    return NextLayerId + 1;
}

void FSplitter::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (GetHandleIndexAt(ClientPosition) >= 0)
    {
        return;
    }

    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child)
        {
            Child->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }
}

bool FSplitter::GetCursor(ECursor& OutCursor) const
{
    if (ActiveHandleIndex < 0 && HoveredHandleIndex < 0)
    {
        return false;
    }

    OutCursor = Orientation == EDockSplitOrientation::Horizontal ? ECursor::ResizeEW : ECursor::ResizeNS;
    return true;
}

FEventResponse FSplitter::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    const int32 HandleIndex = GetHandleIndexAt(CursorEvent.GetClientPosition());
    if (HandleIndex < 0)
    {
        return FEventResponse::Unhandled();
    }

    ActiveHandleIndex  = HandleIndex;
    HoveredHandleIndex = HandleIndex;
    DragOrigin             = CursorEvent.GetClientPosition();
    DragStartFractions     = Fractions;
    DragStartFixedLengths  = FixedLengths;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FSplitter::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (ActiveHandleIndex < 0 || CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    ActiveHandleIndex  = -1;
    HoveredHandleIndex = GetHandleIndexAt(CursorEvent.GetClientPosition());

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    OnFractionsChangedDelegate.ExecuteIfBound(Fractions);
    return FEventResponse::Handled();
}

FEventResponse FSplitter::OnMouseMove(const FCursorEvent& CursorEvent)
{
    const IntVector2 Position = CursorEvent.GetClientPosition();

    if (ActiveHandleIndex < 0)
    {
        HoveredHandleIndex = GetHandleIndexAt(Position);
        return HoveredHandleIndex >= 0 ? FEventResponse::Handled() : FEventResponse::Unhandled();
    }

    Fractions    = DragStartFractions;
    FixedLengths = DragStartFixedLengths;

    const int32 Delta = Orientation == EDockSplitOrientation::Horizontal
        ? Position.X - DragOrigin.X
        : Position.Y - DragOrigin.Y;

    DragHandle(ActiveHandleIndex, Delta);
    return FEventResponse::Handled();
}

FEventResponse FSplitter::OnMouseLeft(const FCursorEvent& CursorEvent)
{
    UNREFERENCED_VARIABLE(CursorEvent);

    if (ActiveHandleIndex < 0)
    {
        HoveredHandleIndex = -1;
    }

    return FEventResponse::Unhandled();
}

void FSplitter::AddChild(const TSharedPtr<FVisualElement>& InChild, const IntVector2& InMinimumSize)
{
    const int32 ChildIndex = Children.Size();
    Children.Add(InChild);

    if (ChildIndex >= MinimumSizes.Size())
    {
        MinimumSizes.Add(InMinimumSize);
    }

    while (FixedLengths.Size() < Children.Size())
    {
        FixedLengths.Add(0);
    }

    if (InChild)
    {
        InChild->SetParentElement(AsWeakPtr());
    }

    if (Fractions.Size() >= Children.Size())
    {
        return;
    }

    const float EvenShare = 1.0f / static_cast<float>(Children.Size());

    Fractions.Clear();
    for (int32 Index = 0; Index < Children.Size(); ++Index)
    {
        Fractions.Add(EvenShare);
    }
}

void FSplitter::ClearChildren()
{
    for (const TSharedPtr<FVisualElement>& Child : Children)
    {
        if (Child)
        {
            Child->SetParentElement(TWeakPtr<FVisualElement>());
        }
    }

    Children.Clear();
    MinimumSizes.Clear();
    Fractions.Clear();
    FixedLengths.Clear();

    ActiveHandleIndex  = -1;
    HoveredHandleIndex = -1;
    InvalidateDesiredSize();
}

void FSplitter::SetFractions(const TArray<float>& InFractions)
{
    if (InFractions.Size() != Children.Size())
    {
        return;
    }

    TryNormalizeFractions(InFractions);
}

bool FSplitter::TryNormalizeFractions(const TArray<float>& InFractions)
{
    float Total = 0.0f;
    for (float Fraction : InFractions)
    {
        Total += Fraction;
    }

    if (Total <= 0.0f)
    {
        return false;
    }

    Fractions.Clear();
    for (float Fraction : InFractions)
    {
        Fractions.Add(Fraction / Total);
    }

    return true;
}

FRectangle FSplitter::GetHandleRectangle(int32 HandleIndex) const
{
    if (HandleIndex < 0 || HandleIndex >= Children.Size() - 1)
    {
        return FRectangle();
    }

    const TSharedPtr<FVisualElement>& Child = Children[HandleIndex];
    if (!Child)
    {
        return FRectangle();
    }

    const FRectangle& Bounds      = GetContentRectangle();
    const FRectangle& ChildBounds = Child->GetContentRectangle();

    if (Orientation == EDockSplitOrientation::Horizontal)
    {
        return FRectangle(IntVector2(ChildBounds.GetRight(), Bounds.Position.Y), HandleThickness, Bounds.Height);
    }

    return FRectangle(IntVector2(Bounds.Position.X, ChildBounds.GetBottom()), Bounds.Width, HandleThickness);
}

int32 FSplitter::GetHandleIndexAt(const IntVector2& ClientPosition) const
{
    for (int32 HandleIndex = 0; HandleIndex < Children.Size() - 1; ++HandleIndex)
    {
        if (GetHandleRectangle(HandleIndex).EncapsulatesPoint(ClientPosition))
        {
            return HandleIndex;
        }
    }

    return -1;
}

void FSplitter::DragHandle(int32 HandleIndex, int32 DeltaPixels)
{
    if (HandleIndex < 0 || HandleIndex + 1 >= Children.Size())
    {
        return;
    }

    const int32 Available = GetAvailableLength(GetContentRectangle());
    if (Available <= 0)
    {
        return;
    }

    const int32 LeadingMinimum  = GetChildMinimumLength(HandleIndex);
    const int32 TrailingMinimum = GetChildMinimumLength(HandleIndex + 1);

    if (GetChildFixedLength(HandleIndex) > 0)
    {
        const int32 DesiredLeading = DragStartFixedLengths.IsValidIndex(HandleIndex)
            ? DragStartFixedLengths[HandleIndex] + DeltaPixels
            : GetChildFixedLength(HandleIndex) + DeltaPixels;

        while (FixedLengths.Size() <= HandleIndex)
        {
            FixedLengths.Add(0);
        }

        FixedLengths[HandleIndex] = Math::Clamp(DesiredLeading, LeadingMinimum, Available - TrailingMinimum);
        OnArrange(GetContentRectangle());
        return;
    }

    if (HandleIndex + 1 >= Fractions.Size())
    {
        return;
    }

    const float LeadingShare  = Fractions[HandleIndex];
    const float TrailingShare = Fractions[HandleIndex + 1];
    const float PairShare     = LeadingShare + TrailingShare;

    const int32 PairLength = Math::RoundToInt(static_cast<float>(Available) * PairShare);

    if (LeadingMinimum + TrailingMinimum > PairLength)
    {
        return;
    }

    const int32 DesiredLeading = Math::RoundToInt(static_cast<float>(Available) * LeadingShare) + DeltaPixels;
    const int32 ClampedLeading = Math::Clamp(DesiredLeading, LeadingMinimum, PairLength - TrailingMinimum);

    Fractions[HandleIndex]     = static_cast<float>(ClampedLeading) / static_cast<float>(Available);
    Fractions[HandleIndex + 1] = PairShare - Fractions[HandleIndex];

    OnArrange(GetContentRectangle());
}

int32 FSplitter::GetChildFixedLength(int32 ChildIndex) const
{
    if (ChildIndex < 0 || ChildIndex >= FixedLengths.Size())
    {
        return 0;
    }

    return Math::Max(FixedLengths[ChildIndex], 0);
}

bool FSplitter::HasAnyFixedLength() const
{
    for (int32 Length : FixedLengths)
    {
        if (Length > 0)
        {
            return true;
        }
    }

    return false;
}

int32 FSplitter::GetAvailableLength(const FRectangle& Bounds) const
{
    const int32 HandleSpace = Math::Max(0, Children.Size() - 1) * HandleThickness;
    const int32 Length      = Orientation == EDockSplitOrientation::Horizontal ? Bounds.Width : Bounds.Height;

    return Math::Max(0, Length - HandleSpace);
}

int32 FSplitter::GetChildMinimumLength(int32 ChildIndex) const
{
    if (ChildIndex < 0 || ChildIndex >= MinimumSizes.Size())
    {
        return 0;
    }

    return Orientation == EDockSplitOrientation::Horizontal ? MinimumSizes[ChildIndex].X : MinimumSizes[ChildIndex].Y;
}
