#include "Application/Elements/VisualElement.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/ElementPath.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/DrawCache.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Templates/NumericLimits.h"

static TAutoConsoleVariable<bool> CVarDrawCacheEnable(
    "UI.DrawCache.Enable",
    "Replays the draw commands a clean subtree recorded before instead of walking it again",
    true,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMinCommands(
    "UI.DrawCache.MinCommands",
    "How many commands a subtree has to emit before keeping its recording is worth the copy",
    24,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMinCleanFrames(
    "UI.DrawCache.MinCleanFrames",
    "How many frames in a row a subtree has to stay clean before its recording is kept",
    2,
    EConsoleVariableFlags::Default);

static TAutoConsoleVariable<int32> CVarDrawCacheMaxDirtyFrames(
    "UI.DrawCache.MaxDirtyFrames",
    "How many recent frames may dirty a subtree before its recording stops being cached at all",
    3,
    EConsoleVariableFlags::Default);

static uint64 GHitTestGeneration = 0;

static IntVector2            GArrangeTranslation(0, 0);

static const FVisualElement* GArrangeTranslationOwner = nullptr;

uint64 FVisualElement::GetHitTestGeneration()
{
    return GHitTestGeneration;
}

FVisualElement::FVisualElement()
    : TSharedFromThis<FVisualElement>()
    , ContentRectangle()
    , CachedDesiredSize()
    , ParentElement()
    , DrawCacheBlock(nullptr)
    , LastRecordedCommandCount(0)
    , Flags(EElementFlags::DesiredSizeDirty | EElementFlags::ArrangeDirty | EElementFlags::PaintDirty | EElementFlags::HitTestable)
    , Visibility(EVisibility::Visible)
    , DrawCachePolicy(EDrawCachePolicy::Auto)
    , CleanPaintFrameCount(0)
    , RecentDirtyFrameCount(0)
{
}

FVisualElement::~FVisualElement()
{
}

void FVisualElement::Arrange(const FRectangle& AssignedBounds)
{
    if (!IsArrangeDirty() && ContentRectangle == AssignedBounds)
    {
        return;
    }

    const FRectangle PreviousRectangle = ContentRectangle;

    SetContentRectangle(AssignedBounds);
    ClearElementFlags(EElementFlags::ArrangeDirty);

    const FVisualElement* const OuterOwner       = GArrangeTranslationOwner;
    const IntVector2            OuterTranslation = GArrangeTranslation;

    const bool bOnlyMoved = PreviousRectangle.Width == AssignedBounds.Width && PreviousRectangle.Height == AssignedBounds.Height;
    GArrangeTranslationOwner = this;
    GArrangeTranslation      = bOnlyMoved ? AssignedBounds.Position - PreviousRectangle.Position : IntVector2(0, 0);

    OnArrange(AssignedBounds);

    GArrangeTranslationOwner = OuterOwner;
    GArrangeTranslation      = OuterTranslation;
}

void FVisualElement::InvalidateArrange()
{
    RequestContinuousArrange();
}

void FVisualElement::RequestContinuousArrange() const
{
    SetElementFlags(EElementFlags::ArrangeDirty);

    for (FVisualElement* Parent = GetLiveParent(); Parent && !Parent->IsArrangeDirty(); Parent = Parent->GetLiveParent())
    {
        Parent->SetElementFlags(EElementFlags::ArrangeDirty);
    }
}

FScrollBox* FVisualElement::AsScrollBox()
{
    return HasAnyElementFlags(EElementFlags::IsScrollBox) ? static_cast<FScrollBox*>(this) : nullptr;
}

void FVisualElement::SetHitTestable(bool bInHitTestable)
{
    ++GHitTestGeneration;

    if (bInHitTestable)
    {
        SetElementFlags(EElementFlags::HitTestable);
    }
    else
    {
        ClearElementFlags(EElementFlags::HitTestable);
    }
}

void FVisualElement::EnableHitTestOverflow()
{
    SetElementFlags(EElementFlags::HitTestOverflow);
    PropagateHitTestOverflow();
}

void FVisualElement::PropagateHitTestOverflow()
{
    for (FVisualElement* Parent = GetLiveParent(); Parent && !Parent->HasAnyElementFlags(EElementFlags::HitTestOverflow); Parent = Parent->GetLiveParent())
    {
        Parent->SetElementFlags(EElementFlags::HitTestOverflow);
    }
}

bool FVisualElement::CapturesAllInput() const
{
    return false;
}

bool FVisualElement::SupportsKeyboardFocus() const
{
    return false;
}

bool FVisualElement::WantsTextInput() const
{
    return false;
}

TSharedPtr<FVisualElement> FVisualElement::GetFocusTarget()
{
    return AsSharedPtr();
}

FEventResponse FVisualElement::OnAnalogGamepadChange(const FAnalogGamepadEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyDown(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyUp(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnKeyChar(const FKeyEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseMove(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonDown(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseButtonUp(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseScroll(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseDoubleClick(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseLeft(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnMouseEntered(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnHighPrecisionMouseInput(const FCursorEvent&)
{
    return FEventResponse::Unhandled();
}

bool FVisualElement::GetCursor(ECursor&) const
{
    return false;
}

FEventResponse FVisualElement::OnFocusLost()
{
    return FEventResponse::Unhandled();
}

FEventResponse FVisualElement::OnFocusGained()
{
    return FEventResponse::Unhandled();
}

IntVector2 FVisualElement::ComputeDesiredSize() const
{
    return IntVector2(0, 0);
}

void FVisualElement::OnArrange(const FRectangle& /*AllottedBounds*/)
{
}

EChildVisit FVisualElement::VisitChildren(FChildVisitor& /*Visitor*/, EChildOrder /*Order*/) const
{
    return EChildVisit::Continue;
}

void FVisualElement::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    ForEachChild([&OutChildren](FVisualElement& Child)
    {
        OutChildren.Add(Child.AsSharedPtr());
        return EChildVisit::Continue;
    });
}

int32 FVisualElement::OnDraw(const FDrawGeometry& /*AllottedGeometry*/, FDrawCommandList& /*OutCommandList*/, int32 LayerId) const
{
    return LayerId;
}

int32 FVisualElement::Draw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    if (DrawCachePolicy == EDrawCachePolicy::Never)
    {
        OutCommandList.BlockDrawCache();

        ClearElementFlags(EElementFlags::PaintDirty);
        return OnDraw(AllottedGeometry, OutCommandList, LayerId);
    }

    if (!CVarDrawCacheEnable.GetValue() || OutCommandList.IsDrawCacheSuppressed())
    {
        ClearElementFlags(EElementFlags::PaintDirty);
        return OnDraw(AllottedGeometry, OutCommandList, LayerId);
    }

    if (!IsPaintDirty() && DrawCacheBlock)
    {
        const int32       ClipDepth     = OutCommandList.GetClipDepth();
        const FRectangle& ClipRectangle = OutCommandList.GetCurrentClipRectangle();

        IntVector2 ReplayOffset(0, 0);
        if (DrawCacheBlock->CanReplay(AllottedGeometry, LayerId, ClipDepth, ClipRectangle, ReplayOffset))
        {
            if (ReplayOffset != IntVector2(0, 0))
            {
                DrawCacheBlock->Translate(ReplayOffset);
            }

            DrawCacheBlock->LastUsedFrame = DrawCacheRegistry::GetCurrentFrame();

            CleanPaintFrameCount = TNumericLimits<uint8>::Max();
            return OutCommandList.AppendDrawCache(*DrawCacheBlock);
        }
    }

    const int32 CommandBase = OutCommandList.GetCommands().Size();

    if (OutCommandList.IsDrawCacheOpen() || !ShouldUseDrawCache())
    {
        DrawCacheBlock.Reset();

        ClearElementFlags(EElementFlags::PaintDirty);
        const int32 MaxLayerId = OnDraw(AllottedGeometry, OutCommandList, LayerId);

        NoteWalked(OutCommandList.GetCommands().Size() - CommandBase);
        return MaxLayerId;
    }

    const FDrawCommandList::FDrawCacheMarker Marker = OutCommandList.BeginDrawCache();

    ClearElementFlags(EElementFlags::PaintDirty);
    const int32 MaxLayerId = OnDraw(AllottedGeometry, OutCommandList, LayerId);

    const bool  bStillClean  = !IsPaintDirty();
    const int32 CommandCount = OutCommandList.GetCommands().Size() - CommandBase;

    const bool bWorthKeeping = bStillClean
        && (DrawCachePolicy == EDrawCachePolicy::Always || CommandCount >= CVarDrawCacheMinCommands.GetValue());

    if (!bWorthKeeping)
    {
        OutCommandList.AbandonDrawCache();
        DrawCacheBlock.Reset();
    }
    else
    {
        if (!DrawCacheBlock)
        {
            DrawCacheBlock = MakeUniquePtr<FDrawCacheBlock>();
        }

        if (!OutCommandList.CaptureDrawCache(Marker, AllottedGeometry, LayerId, MaxLayerId, *DrawCacheBlock))
        {
            DrawCacheBlock->Reset();
        }
    }

    if (OutCommandList.GetDrawCacheBlockCounter() != Marker.BlockCounter)
    {
        SetElementFlags(EElementFlags::DrawCacheBlocked);
    }

    NoteWalked(CommandCount);
    return MaxLayerId;
}

bool FVisualElement::ShouldUseDrawCache() const
{
    if (DrawCachePolicy == EDrawCachePolicy::Always)
    {
        return true;
    }

    if (HasAnyElementFlags(EElementFlags::DrawCacheBlocked))
    {
        return false;
    }

    if (RecentDirtyFrameCount > static_cast<uint8>(Math::Clamp(CVarDrawCacheMaxDirtyFrames.GetValue(), 0, 255)))
    {
        return false;
    }

    if (CleanPaintFrameCount < static_cast<uint8>(Math::Clamp(CVarDrawCacheMinCleanFrames.GetValue(), 0, 255)))
    {
        return false;
    }

    return LastRecordedCommandCount == 0
        || LastRecordedCommandCount >= static_cast<uint16>(Math::Clamp(CVarDrawCacheMinCommands.GetValue(), 0, 65535));
}

void FVisualElement::NoteWalked(int32 CommandCount) const
{
    LastRecordedCommandCount = static_cast<uint16>(Math::Clamp(CommandCount, 0, 65535));

    if (CleanPaintFrameCount < TNumericLimits<uint8>::Max())
    {
        ++CleanPaintFrameCount;
    }

    if (RecentDirtyFrameCount > 0 && CleanPaintFrameCount > RecentDirtyFrameCount)
    {
        --RecentDirtyFrameCount;
    }
}

void FVisualElement::SetOuterCornerRadius(float /*InCornerRadius*/)
{
}

int32 FVisualElement::GetContentTopInset() const
{
    return 0;
}

IntVector2 FVisualElement::PrepareDesiredSize()
{
    if (!IsDesiredSizeDirty())
    {
        return CachedDesiredSize;
    }

    ForEachChild([](FVisualElement& Child)
    {
        Child.PrepareDesiredSize();
        return EChildVisit::Continue;
    });

    ClearElementFlags(EElementFlags::DesiredSizeDirty);

    const IntVector2 DesiredSize = ComputeDesiredSize();
    if (DesiredSize != CachedDesiredSize)
    {
        CachedDesiredSize = DesiredSize;

        if (FVisualElement* Parent = GetLiveParent())
        {
            Parent->InvalidateDesiredSize();
        }
    }

    return CachedDesiredSize;
}

void FVisualElement::InvalidateDesiredSize()
{
    InvalidatePaint();
    InvalidateArrange();

    ClearElementFlags(EElementFlags::DrawCacheBlocked);

    if (IsDesiredSizeDirty())
    {
        return;
    }

    SetElementFlags(EElementFlags::DesiredSizeDirty);

    if (FVisualElement* Parent = GetLiveParent())
    {
        Parent->InvalidateDesiredSize();
    }
}

void FVisualElement::InvalidatePaint()
{
    ++GHitTestGeneration;

    if (IsPaintDirty())
    {
        return;
    }

    SetElementFlags(EElementFlags::PaintDirty);
    CleanPaintFrameCount = 0;

    if (RecentDirtyFrameCount < TNumericLimits<uint8>::Max())
    {
        ++RecentDirtyFrameCount;
    }

    if (FVisualElement* Parent = GetLiveParent())
    {
        Parent->InvalidatePaint();
    }
}

void FVisualElement::RequestContinuousPaint() const
{
    ++GHitTestGeneration;

    SetElementFlags(EElementFlags::PaintDirty);
    CleanPaintFrameCount = 0;

    for (FVisualElement* Parent = GetLiveParent(); Parent; Parent = Parent->GetLiveParent())
    {
        Parent->SetElementFlags(EElementFlags::PaintDirty);
        Parent->CleanPaintFrameCount = 0;
    }
}

void FVisualElement::SetDrawCachePolicy(EDrawCachePolicy InPolicy)
{
    if (DrawCachePolicy == InPolicy)
    {
        return;
    }

    DrawCachePolicy = InPolicy;

    if (DrawCachePolicy == EDrawCachePolicy::Never)
    {
        ReleaseDrawCache();
    }

    InvalidatePaint();
}

void FVisualElement::ReleaseDrawCache()
{
    DrawCacheBlock.Reset();
}

bool FVisualElement::HasDrawCache() const
{
    return DrawCacheBlock && DrawCacheBlock->bValid;
}

void FVisualElement::FindParentElements(FElementPath& OutRootPath)
{
    if (ParentElement.IsValid())
    {
        ParentElement->FindParentElements(OutRootPath);
    }

    OutRootPath.Add(Visibility, AsSharedPtr());
}

bool FVisualElement::HitTest(const IntVector2& ClientPosition, FElementPath& OutPath)
{
    if (!IsVisible() || !IsHitTestable())
    {
        return false;
    }

    if (!ContentRectangle.EncapsulatesPoint(ClientPosition))
    {
        if (!HasAnyElementFlags(EElementFlags::HitTestOverflow))
        {
            return false;
        }

        const int32 PathSize = OutPath.Size();
        HitTestChildren(ClientPosition, OutPath);
        return OutPath.Size() != PathSize;
    }

    OutPath.Add(Visibility, AsSharedPtr());
    HitTestChildren(ClientPosition, OutPath);
    return true;
}

void FVisualElement::HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath)
{
    ForEachChild([&ClientPosition, &OutPath](FVisualElement& Child)
    {
        return Child.HitTest(ClientPosition, OutPath) ? EChildVisit::Stop : EChildVisit::Continue;
    }, EChildOrder::FrontToBack);
}

void FVisualElement::SetVisibility(EVisibility InVisibility)
{
    if (Visibility != InVisibility)
    {
        ++GHitTestGeneration;

        Visibility = InVisibility;
        InvalidateDesiredSize();
    }
}

void FVisualElement::SetParentElement(const TWeakPtr<FVisualElement>& InParentElement)
{
    ++GHitTestGeneration;

    TSharedPtr<FVisualElement> PreviousParent = ParentElement.ToSharedPtr();
    ParentElement = InParentElement;

    if (HasAnyElementFlags(EElementFlags::HitTestOverflow))
    {
        PropagateHitTestOverflow();
    }

    InvalidateDesiredSize();

    TSharedPtr<FVisualElement> Parent = ParentElement.ToSharedPtr();
    if (Parent)
    {
        Parent->InvalidateDesiredSize();
    }

    if (PreviousParent && PreviousParent != Parent)
    {
        PreviousParent->InvalidateDesiredSize();
    }
}

void FVisualElement::SetContentRectangle(const FRectangle& InContentRectangle)
{
    const bool       bPlacedByParent = GArrangeTranslationOwner != nullptr && GArrangeTranslationOwner != this && GArrangeTranslationOwner == GetLiveParent();
    const IntVector2 ParentMove      = bPlacedByParent ? GArrangeTranslation : IntVector2(0, 0);
    const bool       bParentMoved    = ParentMove != IntVector2(0, 0);

    if (ContentRectangle == InContentRectangle)
    {
        if (bParentMoved)
        {
            InvalidatePaint();
        }

        return;
    }

    const bool bCarriedAlong = bParentMoved
        && InContentRectangle.Width == ContentRectangle.Width
        && InContentRectangle.Height == ContentRectangle.Height
        && InContentRectangle.Position - ContentRectangle.Position == ParentMove;

    ++GHitTestGeneration;

    ContentRectangle = InContentRectangle;

    if (!bCarriedAlong)
    {
        InvalidatePaint();
    }
}
