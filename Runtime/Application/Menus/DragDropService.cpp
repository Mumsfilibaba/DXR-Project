#include "Application/Menus/DragDropService.h"
#include "Application/Menus/MenuStack.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Math/Math.h"

TUniquePtr<FDragDropService> FDragDropService::DragDropService = nullptr;

FDragDropService& FDragDropService::Get()
{
    if (!DragDropService)
    {
        DragDropService = MakeUniquePtr<FDragDropService>();
    }

    return *DragDropService;
}

void FDragDropService::Release()
{
    if (DragDropService)
    {
        DragDropService->CancelDrag();
        DragDropService.Reset();
    }
}

FDragDropService::FDragDropService()
    : Payload()
    , ScreenPosition()
    , Targets()
    , TargetIndex(InvalidTargetIndex)
{
}

FDragDropService::~FDragDropService() = default;

void FDragDropService::BeginDrag(const FDragDropPayload& InPayload, const IntVector2& InScreenPosition)
{
    if (!InPayload.IsValid())
    {
        return;
    }

    Payload        = InPayload;
    ScreenPosition = InScreenPosition;

    UpdateDrag(InScreenPosition);
}

void FDragDropService::UpdateDrag(const IntVector2& InScreenPosition)
{
    ScreenPosition = InScreenPosition;

    if (!IsDragging())
    {
        return;
    }

    TargetIndex = FindTargetAt(InScreenPosition);
}

void FDragDropService::EndDrag(const IntVector2& InScreenPosition)
{
    if (!IsDragging())
    {
        return;
    }

    UpdateDrag(InScreenPosition);

    const FDragDropPayload DroppedPayload = Payload;
    const int32            DroppedIndex   = TargetIndex;

    CancelDrag();

    if (DroppedIndex != InvalidTargetIndex)
    {
        Targets[DroppedIndex].OnDropped.ExecuteIfBound(DroppedPayload, InScreenPosition);
    }
}

void FDragDropService::CancelDrag()
{
    Payload = FDragDropPayload();

    TargetIndex = InvalidTargetIndex;
}

void FDragDropService::RegisterTarget(const TWeakPtr<FVisualElement>& Target, const FOnDragDropped& OnDropped, const FOnDragOver& OnOver)
{
    if (!Target.IsValid())
    {
        return;
    }

    UnregisterTarget(Target);

    FTarget& NewTarget = Targets.Emplace();
    NewTarget.Element   = Target;
    NewTarget.OnDropped = OnDropped;
    NewTarget.OnOver    = OnOver;
}

void FDragDropService::UnregisterTarget(const TWeakPtr<FVisualElement>& Target)
{
    const FVisualElement* Element = Target.Get();
    if (!Element)
    {
        return;
    }

    for (int32 Index = Targets.Size() - 1; Index >= 0; --Index)
    {
        if (Targets[Index].Element.Get() != Element)
        {
            continue;
        }

        Targets.RemoveAt(Index);

        if (TargetIndex == Index)
        {
            TargetIndex = InvalidTargetIndex;
        }
        else if (TargetIndex > Index)
        {
            --TargetIndex;
        }
    }
}

void FDragDropService::DrawDragVisual(FDrawCommandList& OutCommandList, int32 LayerId, const IntVector2& ClientOrigin) const
{
    if (!IsDragging())
    {
        return;
    }

    const FUIStyle&  Style = FUIStyle::GetDefault();
    const IFontFace* Font  = Style.NormalFont;

    const bool  bHasIcon   = Payload.Icon.IsValid();
    const int32 IconExtent = bHasIcon ? DragVisualIconSize + DragVisualPadding : 0;
    const int32 TextWidth  = Font ? Font->MeasureWidth(StringView(Payload.DisplayText.Data(), Payload.DisplayText.Length())) : 0;
    const int32 TextHeight = Font ? Font->GetLineHeight() : DragVisualIconSize;

    FRectangle Ghost;
    Ghost.Width    = (DragVisualPadding * 2) + IconExtent + TextWidth;
    Ghost.Height   = (DragVisualPadding * 2) + Math::Max(TextHeight, bHasIcon ? DragVisualIconSize : 0);
    Ghost.Position = (ScreenPosition - ClientOrigin) + IntVector2(DragVisualCursorOffset, DragVisualCursorOffset);

    const FCornerRadii Radii(Style.Metrics.CornerRadius);
    OutCommandList.AddBox(LayerId, Ghost, Style.Colors.PanelBackground, Radii);
    OutCommandList.AddBoxOutline(LayerId, Ghost, HasTarget() ? Style.Colors.Accent : Style.Colors.Border, Style.Metrics.BorderThickness, Radii);

    FRectangle Inner = Ghost.Deflate(FMargin(DragVisualPadding));

    if (bHasIcon)
    {
        FRectangle IconBounds = Inner;
        IconBounds.Width      = DragVisualIconSize;
        IconBounds.Height     = DragVisualIconSize;
        IconBounds.Position.Y = Inner.Position.Y + ((Inner.Height - DragVisualIconSize) / 2);

        OutCommandList.AddImage(LayerId + 1, IconBounds, Payload.Icon, FFloatColor::White);

        Inner.Position.X += IconExtent;
        Inner.Width       = Math::Max(Inner.Width - IconExtent, 0);
    }

    if (!Payload.DisplayText.IsEmpty())
    {
        OutCommandList.AddText(LayerId + 1, Inner, Payload.DisplayText, Font, Style.Colors.Text);
    }
}

FRectangle FDragDropService::ResolveTargetBounds(const TSharedPtr<FVisualElement>& Element)
{
    const FRectangle ScreenBounds = FMenuStack::GetScreenBounds(Element);
    return ScreenBounds.IsEmpty() ? Element->GetContentRectangle() : ScreenBounds;
}

int32 FDragDropService::FindTargetAt(const IntVector2& InScreenPosition) const
{
    for (int32 Index = Targets.Size() - 1; Index >= 0; --Index)
    {
        const FTarget& Target = Targets[Index];
        if (!Target.Element.IsValid())
        {
            continue;
        }

        const TSharedPtr<FVisualElement> Element(Target.Element);
        if (!ResolveTargetBounds(Element).EncapsulatesPoint(InScreenPosition))
        {
            continue;
        }

        if (Target.OnOver.IsBound() && !Target.OnOver.Execute(Payload))
        {
            continue;
        }

        return Index;
    }

    return InvalidTargetIndex;
}
