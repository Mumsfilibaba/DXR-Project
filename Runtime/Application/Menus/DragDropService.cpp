#include "Application/Application.h"
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
    , OnWindowPaintingHandle()
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

    if (!OnWindowPaintingHandle.IsValid() && FApplication::IsInitialized())
    {
        OnWindowPaintingHandle = FApplication::Get().GetOnWindowPaintingEvent().AddRaw(this, &FDragDropService::OnWindowPainting);
    }

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

    if (OnWindowPaintingHandle.IsValid())
    {
        if (FApplication::IsInitialized())
        {
            FApplication::Get().GetOnWindowPaintingEvent().Unbind(OnWindowPaintingHandle);
        }

        OnWindowPaintingHandle = FDelegateHandle();
    }
}

void FDragDropService::SetPreview(EDragDropPreviewState InState, const String& InStatusText)
{
    if (!IsDragging())
    {
        return;
    }

    Payload.PreviewState = InState;
    Payload.StatusText   = InStatusText;
}

void FDragDropService::OnWindowPainting(const TSharedPtr<FWindow>& Window)
{
    if (!IsDragging() || !Window || Window != FApplication::Get().FindWindowUnderCursor())
    {
        return;
    }

    const IntVector2 ClientOrigin = Window->GetPosition();
    Window->QueueDeferredPainting(FOnDeferredPaint::CreateLambda([this, ClientOrigin](FDrawCommandList& OutCommandList, int32 LayerId)
    {
        DrawDragVisual(OutCommandList, LayerId, ClientOrigin);
        return LayerId + 1;
    }));
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

    const bool  bHasIcon        = Payload.Icon.IsValid();
    const int32 SourceIconSize  = bHasIcon ? Math::Max(Payload.IconSize, DragVisualIconSize) : 0;
    const int32 IconExtent      = bHasIcon ? SourceIconSize + DragVisualPadding : 0;
    const int32 TextWidth       = Font ? Font->MeasureWidth(StringView(Payload.DisplayText.Data(), Payload.DisplayText.Length())) : 0;
    const int32 TextHeight      = Font ? Font->GetLineHeight() : DragVisualIconSize;

    const int32 StatusWidth  = (Font && !Payload.StatusText.IsEmpty()) ? Font->MeasureWidth(StringView(Payload.StatusText.Data(), Payload.StatusText.Length())) : 0;
    const int32 StatusHeight = (Font && !Payload.StatusText.IsEmpty()) ? Font->GetLineHeight() : 0;
    const FUIBrush& StatusIcon = Payload.PreviewState == EDragDropPreviewState::Forbidden
        ? Payload.ForbiddenStatusIcon
        : Payload.AllowedStatusIcon;
    const bool  bHasStatusIcon = StatusHeight > 0 && StatusIcon.IsValid();
    const int32 StatusIconExtent = bHasStatusIcon ? StatusHeight + DragVisualPadding : 0;

    FRectangle Ghost;
    Ghost.Width    = (DragVisualPadding * 2) + IconExtent + Math::Max(TextWidth, StatusIconExtent + StatusWidth);
    Ghost.Height   = (DragVisualPadding * 2) + Math::Max(TextHeight + StatusHeight, SourceIconSize);
    Ghost.Position = (ScreenPosition - ClientOrigin) + IntVector2(DragVisualCursorOffset, DragVisualCursorOffset);

    const FCornerRadii Radii(Style.Metrics.CornerRadius);
    FFloatColor Outline = Style.Colors.Border;
    if (Payload.PreviewState == EDragDropPreviewState::Allowed)
    {
        Outline = Style.Colors.Accent;
    }
    else if (Payload.PreviewState == EDragDropPreviewState::Forbidden)
    {
        Outline = Style.Colors.TextDisabled;
    }
    else if (Payload.PreviewState == EDragDropPreviewState::Partial)
    {
        Outline = Style.Colors.AccentHovered;
    }

    OutCommandList.AddBox(LayerId, Ghost, Style.Colors.PanelBackground, Radii);
    OutCommandList.AddBoxOutline(LayerId, Ghost, Outline, Style.Metrics.BorderThickness, Radii);

    FRectangle Inner = Ghost.Deflate(FMargin(DragVisualPadding));

    if (bHasIcon)
    {
        FRectangle IconBounds = Inner;
        IconBounds.Width      = SourceIconSize;
        IconBounds.Height     = SourceIconSize;
        IconBounds.Position.Y = Inner.Position.Y + ((Inner.Height - SourceIconSize) / 2);

        OutCommandList.AddImage(LayerId + 1, IconBounds, Payload.Icon, FFloatColor::White);

        if (Payload.SelectionCount > 1 && Font)
        {
            const String CountText = String::Printf("+%d", Payload.SelectionCount - 1);
            const int32  CountWidth = Font->MeasureWidth(StringView(CountText.Data(), CountText.Length()));

            FRectangle BadgeBounds;
            BadgeBounds.Width      = CountWidth + (DragVisualPadding * 2);
            BadgeBounds.Height     = TextHeight + DragVisualPadding;
            BadgeBounds.Position.X = IconBounds.Position.X;
            BadgeBounds.Position.Y = IconBounds.GetBottom() - BadgeBounds.Height;

            OutCommandList.AddBox(LayerId + 2, BadgeBounds, Style.Colors.PanelBackground, Radii);
            OutCommandList.AddText(LayerId + 3, BadgeBounds.Deflate(FMargin(DragVisualPadding, 0)), CountText, Font, Style.Colors.Text);
        }

        Inner.Position.X += IconExtent;
        Inner.Width       = Math::Max(Inner.Width - IconExtent, 0);
    }

    const int32 TextBlockHeight = TextHeight + StatusHeight;
    const int32 TextTop = Inner.Position.Y + Math::Max((Inner.Height - TextBlockHeight) / 2, 0);

    if (!Payload.DisplayText.IsEmpty())
    {
        FRectangle NameBounds = Inner;
        NameBounds.Position.Y = TextTop;
        NameBounds.Height     = TextHeight;
        OutCommandList.AddText(LayerId + 1, NameBounds, Payload.DisplayText, Font, Style.Colors.Text);
    }

    if (!Payload.StatusText.IsEmpty())
    {
        FRectangle StatusBounds = Inner;
        StatusBounds.Position.Y = TextTop + TextHeight;
        StatusBounds.Height      = StatusHeight;

        if (bHasStatusIcon)
        {
            FRectangle StatusIconBounds = StatusBounds;
            StatusIconBounds.Width      = StatusHeight;
            StatusIconBounds.Height     = StatusHeight;
            OutCommandList.AddImage(LayerId + 1, StatusIconBounds, StatusIcon, Outline);

            StatusBounds.Position.X += StatusIconExtent;
            StatusBounds.Width       = Math::Max(StatusBounds.Width - StatusIconExtent, 0);
        }

        OutCommandList.AddText(LayerId + 1, StatusBounds, Payload.StatusText, Font,
            Payload.PreviewState == EDragDropPreviewState::Forbidden ? Style.Colors.TextDisabled : Style.Colors.Text);
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
