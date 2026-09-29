#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/DrawCache.h"
#include "Application/Text/FontAtlas.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"

static FORCEINLINE void ReserveAtLeast(TArray<Vector2>& Array, int32 RequiredCapacity)
{
    if (RequiredCapacity > Array.Capacity())
    {
        Array.Reserve(Math::Max(RequiredCapacity, Array.Capacity() * 2));
    }
}

template<typename ElementType>
static FORCEINLINE void CopyPoolSlice(const TArray<ElementType>& Source, int32 Base, TArray<ElementType>& Destination)
{
    const int32 Count = Source.Size() - Base;
    Destination.ResizeUninitialized(Math::Max(Count, 0));

    if (Count > 0)
    {
        Memory::Memcpy(Destination.Data(), Source.Data() + Base, Count * static_cast<int32>(sizeof(ElementType)));
    }
}

template<typename ElementType>
static FORCEINLINE void AppendPoolSlice(const TArray<ElementType>& Source, TArray<ElementType>& Destination)
{
    if (Source.IsEmpty())
    {
        return;
    }

    const int32 Base = Destination.Size();
    Destination.ResizeUninitialized(Base + Source.Size());
    Memory::Memcpy(Destination.Data() + Base, Source.Data(), Source.Size() * static_cast<int32>(sizeof(ElementType)));
}

FDrawCommandList::FDrawCommandList()
    : Commands()
    , Points()
    , TextPool()
    , Brushes()
    , ClipRects()
    , ScratchPoints()
    , ClipStack()
    , ReplayedSpans()
    , EmptyClipRectangle()
    , UnmatchedPopCount(0)
    , OpenDrawCacheCount(0)
    , DrawCacheBlockCounter(0)
    , MinClipDepthSinceDrawCache(0)
    , ReplayedCommandCount(0)
    , bDrawCacheSuppressed(false)
{
}

FDrawCommandList::~FDrawCommandList() = default;

FDrawCommand& FDrawCommandList::EmplaceCommand(EDrawCommandType Type, int32 LayerId)
{
    Commands.AddUninitialized();
    FDrawCommand& Command = Commands.Last();
    Memory::Memzero(&Command, sizeof(FDrawCommand));

    Command.Type    = Type;
    Command.LayerId = LayerId;

    if (!ClipStack.IsEmpty())
    {
        Command.Flags  = EDrawCommandFlags::Clipped;
        Command.ClipId = ClipStack.Last();
    }

    return Command;
}

void FDrawCommandList::AddBox(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, const FCornerRadii& CornerRadius)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Box, LayerId);
    Command.Bounds       = Bounds;
    Command.PackedColor  = Tint.ToPackedRGBA();
    Command.CornerRadius = CornerRadius;
}

void FDrawCommandList::AddBoxOutline(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, float Thickness, const FCornerRadii& CornerRadius)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::BoxOutline, LayerId);
    Command.Bounds       = Bounds;
    Command.PackedColor  = Tint.ToPackedRGBA();
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
}

void FDrawCommandList::AddText(int32 LayerId, const FRectangle& Bounds, const StringView& InText, const IFontFace* Font, const FFloatColor& Tint)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Text, LayerId);
    Command.Bounds      = Bounds;
    Command.PackedColor = Tint.ToPackedRGBA();
    Command.Font        = Font;
    StoreText(Command, InText);
}

void FDrawCommandList::AddLine(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Line, LayerId);
    Command.Bounds      = Bounds;
    Command.PackedColor = Tint.ToPackedRGBA();
}

void FDrawCommandList::AddLine(int32 LayerId, const Vector2& Start, const Vector2& End, const FFloatColor& Tint, float Thickness)
{
    const Vector2 LinePoints[2] = { Start, End };
    AddPolyline(LayerId, TArrayView<const Vector2>(LinePoints, 2), Tint, Thickness, false);
}

void FDrawCommandList::AddPolyline(int32 LayerId, TArrayView<const Vector2> InPoints, const FFloatColor& Tint, float Thickness, bool bClosed)
{
    if (InPoints.Size() < 2 || Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Polyline, LayerId);
    Command.PackedColor = Tint.ToPackedRGBA();
    Command.Thickness   = Thickness;
    if (bClosed)
    {
        Command.Flags |= EDrawCommandFlags::Closed;
    }

    StorePoints(Command, InPoints);
}

void FDrawCommandList::AddConvexPolygon(int32 LayerId, TArrayView<const Vector2> InPoints, const FFloatColor& Tint)
{
    if (InPoints.Size() < 3)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::ConvexPolygon, LayerId);
    Command.PackedColor = Tint.ToPackedRGBA();

    StorePoints(Command, InPoints);
}

void FDrawCommandList::AddPanelChrome(
    int32               LayerId,
    const FRectangle&   Bounds,
    const FCornerRadii& CornerRadius,
    float               Thickness,
    const FFloatColor&  BorderTint,
    const FFloatColor&  BackdropTint)
{
    if (Bounds.IsEmpty())
    {
        return;
    }

    const FCornerRadii Clamped = CornerRadius.ClampToBounds(Bounds);

    if (Clamped.GetLargest() > 0.0f && BackdropTint.A > 0.0f)
    {
        FDrawCommand& Command = EmplaceCommand(EDrawCommandType::CornerWedges, LayerId);
        Command.Bounds       = Bounds;
        Command.PackedColor  = BackdropTint.ToPackedRGBA();
        Command.CornerRadius = Clamped;
    }

    AddBoxOutline(LayerId, Bounds, BorderTint, Thickness, Clamped);
}

void FDrawCommandList::AddTriangle(int32 LayerId, const Vector2& A, const Vector2& B, const Vector2& C, const FFloatColor& Tint)
{
    const Vector2 Corners[3] = { A, B, C };
    AddConvexPolygon(LayerId, TArrayView<const Vector2>(Corners, 3), Tint);
}

void FDrawCommandList::AddCircle(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Constants::TwoPI, Segments);

    BuildArcPoints(Center, Radius, 0.0f, Math::Constants::TwoPI - (Math::Constants::TwoPI / static_cast<float>(SegmentCount)), SegmentCount - 1);
    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, true);
}

void FDrawCommandList::AddCircleFilled(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Constants::TwoPI, Segments);

    BuildArcPoints(Center, Radius, 0.0f, Math::Constants::TwoPI - (Math::Constants::TwoPI / static_cast<float>(SegmentCount)), SegmentCount - 1);
    AddConvexPolygon(LayerId, ScratchPoints, Tint);
}

void FDrawCommandList::AddArc(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Abs(EndAngle - StartAngle), Segments);

    BuildArcPoints(Center, Radius, StartAngle, EndAngle, SegmentCount);
    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, false);
}

void FDrawCommandList::AddArcFilled(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, int32 Segments)
{
    if (Radius <= 0.0f)
    {
        return;
    }

    const int32 SegmentCount = ResolveCircleSegments(Radius, Math::Abs(EndAngle - StartAngle), Segments);

    BuildArcPoints(Center, Radius, StartAngle, EndAngle, SegmentCount);

    ScratchPoints.Insert(0, Center);
    AddConvexPolygon(LayerId, ScratchPoints, Tint);
}

void FDrawCommandList::AddBezier(int32 LayerId, const Vector2& P0, const Vector2& P1, const Vector2& P2, const Vector2& P3, const FFloatColor& Tint, float Thickness, int32 Segments)
{
    int32 SegmentCount = Segments;
    if (SegmentCount <= 0)
    {
        const float ControlLength = (P1 - P0).GetLength() + (P2 - P1).GetLength() + (P3 - P2).GetLength();
        SegmentCount = Math::Clamp(static_cast<int32>(ControlLength * 0.25f), MinBezierSegments, MaxBezierSegments);
    }

    ScratchPoints.Clear();
    ReserveAtLeast(ScratchPoints, SegmentCount + 1);

    for (int32 Index = 0; Index <= SegmentCount; ++Index)
    {
        const float T        = static_cast<float>(Index) / static_cast<float>(SegmentCount);
        const float OneMinus = 1.0f - T;

        const float W0 = OneMinus * OneMinus * OneMinus;
        const float W1 = 3.0f * OneMinus * OneMinus * T;
        const float W2 = 3.0f * OneMinus * T * T;
        const float W3 = T * T * T;

        ScratchPoints.Add(Vector2((P0.X * W0) + (P1.X * W1) + (P2.X * W2) + (P3.X * W3), (P0.Y * W0) + (P1.Y * W1) + (P2.Y * W2) + (P3.Y * W3)));
    }

    AddPolyline(LayerId, ScratchPoints, Tint, Thickness, false);
}

void FDrawCommandList::AddImage(int32 LayerId, const FRectangle& Bounds, const FUIBrush& Brush, const FFloatColor& Tint, const FCornerRadii& CornerRadius)
{
    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::Image, LayerId);
    Command.Bounds       = Bounds;
    Command.PackedColor  = Tint.ToPackedRGBA();
    Command.CornerRadius = CornerRadius;
    Command.PayloadOffset = Brushes.Size();
    Command.PayloadCount  = 1;
    Brushes.Add(Brush);
}

void FDrawCommandList::AddRoundedBottomBar(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness, const FFloatColor& Tint, float FadeWidth)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::RoundedBottomBar, LayerId);
    Command.Bounds       = Bounds;
    Command.PackedColor  = Tint.ToPackedRGBA();
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
    Command.Fade[0]      = Math::Max(FadeWidth, 0.0f);
}

void FDrawCommandList::AddRoundedAccentRing(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness,
    const FFloatColor& Tint, float FadeFraction, float TrailAlpha)
{
    if (Thickness <= 0.0f)
    {
        return;
    }

    FDrawCommand& Command = EmplaceCommand(EDrawCommandType::RoundedAccentRing, LayerId);
    Command.Bounds       = Bounds;
    Command.PackedColor  = Tint.ToPackedRGBA();
    Command.CornerRadius = CornerRadius;
    Command.Thickness    = Thickness;
    Command.Fade[0]      = Math::Clamp(FadeFraction, 0.0f, 1.0f);
    Command.Fade[1]      = Math::Clamp(TrailAlpha, 0.0f, 1.0f);
}

void FDrawCommandList::PushClip(int32 /*LayerId*/, const FRectangle& ClipRectangle)
{
    const FRectangle Resolved = ClipStack.IsEmpty() ? ClipRectangle : ClipRects[ClipStack.Last() - 1].Intersect(ClipRectangle);
    ClipRects.Add(Resolved);
    ClipStack.Add(static_cast<uint16>(ClipRects.Size()));
}

void FDrawCommandList::PopClip(int32 /*LayerId*/)
{
    if (ClipStack.IsEmpty())
    {
        UnmatchedPopCount++;
    }
    else
    {
        ClipStack.RemoveAt(ClipStack.LastIndex());
        MinClipDepthSinceDrawCache = Math::Min(MinClipDepthSinceDrawCache, ClipStack.Size());
    }
}

void FDrawCommandList::Reset()
{
    Commands.Clear();
    Points.Clear();
    TextPool.Clear();
    Brushes.Clear();
    ClipRects.Clear();
    ScratchPoints.Clear();
    ClipStack.Clear();
    ReplayedSpans.Clear();

    UnmatchedPopCount          = 0;
    OpenDrawCacheCount         = 0;
    DrawCacheBlockCounter      = 0;
    MinClipDepthSinceDrawCache = 0;
    ReplayedCommandCount       = 0;
}

FDrawCommandList::FDrawCacheMarker FDrawCommandList::BeginDrawCache() const
{
    FDrawCacheMarker Marker;
    Marker.CommandBase  = Commands.Size();
    Marker.PointBase    = Points.Size();
    Marker.TextBase     = TextPool.Size();
    Marker.BrushBase    = Brushes.Size();
    Marker.ClipBase     = ClipRects.Size();
    Marker.ClipDepth    = ClipStack.Size();
    Marker.BlockCounter = DrawCacheBlockCounter;

    if (OpenDrawCacheCount == 0)
    {
        MinClipDepthSinceDrawCache = ClipStack.Size();
    }

    ++OpenDrawCacheCount;
    return Marker;
}

void FDrawCommandList::AbandonDrawCache() const
{
    CHECK(OpenDrawCacheCount > 0);
    --OpenDrawCacheCount;
}

void FDrawCommandList::BlockDrawCache()
{
    ++DrawCacheBlockCounter;
}

bool FDrawCommandList::CaptureDrawCache(
    const FDrawCacheMarker& Marker,
    const FDrawGeometry& AllottedGeometry,
    int32                BaseLayerId,
    int32                MaxLayerId,
    FDrawCacheBlock& OutBlock) const
{
    CHECK(OpenDrawCacheCount > 0);
    --OpenDrawCacheCount;

    if (DrawCacheBlockCounter != Marker.BlockCounter)
    {
        return false;
    }

    if (ClipStack.Size() != Marker.ClipDepth || MinClipDepthSinceDrawCache < Marker.ClipDepth)
    {
        return false;
    }

    const int32 CommandCount = Commands.Size() - Marker.CommandBase;
    if (CommandCount <= 0)
    {
        return false;
    }

    const int64 PreviousByteSize = OutBlock.GetByteSize();

    OutBlock.Commands.ResizeUninitialized(CommandCount);
    Memory::Memcpy(OutBlock.Commands.Data(), Commands.Data() + Marker.CommandBase,
        CommandCount * static_cast<int32>(sizeof(FDrawCommand)));

    CopyPoolSlice(Points, Marker.PointBase, OutBlock.Points);
    CopyPoolSlice(TextPool, Marker.TextBase, OutBlock.TextPool);
    CopyPoolSlice(Brushes, Marker.BrushBase, OutBlock.Brushes);
    CopyPoolSlice(ClipRects, Marker.ClipBase, OutBlock.ClipRects);

    int32 TextCommandCount = 0;

    for (FDrawCommand& Command : OutBlock.Commands)
    {
        switch (Command.Type)
        {
            case EDrawCommandType::Polyline:
            case EDrawCommandType::ConvexPolygon:
            {
                Command.PayloadOffset -= Marker.PointBase;
                break;
            }

            case EDrawCommandType::Text:
            {
                Command.PayloadOffset -= Marker.TextBase;
                ++TextCommandCount;
                break;
            }

            case EDrawCommandType::Image:
            {
                Command.PayloadOffset -= Marker.BrushBase;
                break;
            }

            default:
            {
                break;
            }
        }

        if (Command.PayloadOffset < 0)
        {
            OutBlock.Reset();
            return false;
        }

        if (Command.IsClipped())
        {
            const int32 ClipIndex = static_cast<int32>(Command.ClipId) - 1;
            Command.ClipId = (ClipIndex >= Marker.ClipBase)
                ? static_cast<uint16>(ClipIndex - Marker.ClipBase + 1)
                : InheritedClipId;
        }
    }

    OutBlock.CapturedRectangle     = AllottedGeometry.Bounds;
    OutBlock.CapturedScale         = AllottedGeometry.Scale;
    OutBlock.CapturedClipRectangle = GetCurrentClipRectangle();
    OutBlock.CapturedEpoch         = DrawCacheEpoch::Get();
    OutBlock.BaseLayerId           = BaseLayerId;
    OutBlock.MaxLayerId            = MaxLayerId;
    OutBlock.CapturedClipDepth     = Marker.ClipDepth;
    OutBlock.TextCommandCount      = TextCommandCount;
    OutBlock.LastUsedFrame         = DrawCacheRegistry::GetCurrentFrame();
    OutBlock.bValid                = true;
    OutBlock.bGeometryValid        = false;
    OutBlock.bHasCapturedClip      = Marker.ClipDepth > 0;

    OutBlock.AtlasDependencies.Clear();
    for (const FDrawCommand& Command : OutBlock.Commands)
    {
        if (Command.Type != EDrawCommandType::Text || !Command.Font)
        {
            continue;
        }

        const FFontAtlas* Atlas = Command.Font->GetAtlas();
        if (!Atlas)
        {
            continue;
        }

        bool bAlreadyTracked = false;
        for (const FDrawCacheAtlasDependency& Dependency : OutBlock.AtlasDependencies)
        {
            if (Dependency.Atlas == Atlas)
            {
                bAlreadyTracked = true;
                break;
            }
        }

        if (!bAlreadyTracked)
        {
            OutBlock.AtlasDependencies.Emplace(Atlas, Atlas->GetRevision());
        }
    }

    DrawCacheRegistry::NotifySizeChanged(OutBlock.GetByteSize() - PreviousByteSize);
    return true;
}

int32 FDrawCommandList::AppendDrawCache(FDrawCacheBlock& Block)
{
    CHECK(Block.bValid);

    const int32 CommandBase = Commands.Size();
    const int32 PointBase   = Points.Size();
    const int32 TextBase    = TextPool.Size();
    const int32 BrushBase   = Brushes.Size();
    const int32 ClipBase    = ClipRects.Size();

    AppendPoolSlice(Block.Points, Points);
    AppendPoolSlice(Block.TextPool, TextPool);
    AppendPoolSlice(Block.Brushes, Brushes);
    AppendPoolSlice(Block.ClipRects, ClipRects);

    const uint16 InheritedId = ClipStack.IsEmpty() ? 0 : ClipStack.Last();

    Commands.ResizeUninitialized(CommandBase + Block.Commands.Size());
    FDrawCommand* Output = Commands.Data() + CommandBase;
    Memory::Memcpy(Output, Block.Commands.Data(), Block.Commands.Size() * static_cast<int32>(sizeof(FDrawCommand)));

    for (int32 Index = 0; Index < Block.Commands.Size(); ++Index)
    {
        FDrawCommand& Command = Output[Index];

        switch (Command.Type)
        {
            case EDrawCommandType::Polyline:
            case EDrawCommandType::ConvexPolygon:
            {
                Command.PayloadOffset += PointBase;
                break;
            }

            case EDrawCommandType::Text:
            {
                Command.PayloadOffset += TextBase;
                break;
            }

            case EDrawCommandType::Image:
            {
                Command.PayloadOffset += BrushBase;
                break;
            }

            default:
            {
                break;
            }
        }

        if (Command.IsClipped())
        {
            Command.ClipId = (Command.ClipId == InheritedClipId)
                ? InheritedId
                : static_cast<uint16>(Command.ClipId + ClipBase);
        }
    }

    FReplayedSpan& Span = ReplayedSpans.Emplace();
    Span.Block        = &Block;
    Span.CommandIndex = CommandBase;

    ReplayedCommandCount += Block.Commands.Size();
    return Block.MaxLayerId;
}

FDrawCacheBlock* FDrawCommandList::FindReplayedSpanContaining(int32 CommandIndex, int32& OutSpanStart) const
{
    int32 Low  = 0;
    int32 High = ReplayedSpans.Size() - 1;

    while (Low <= High)
    {
        const int32 Middle = Low + ((High - Low) / 2);
        if (ReplayedSpans[Middle].CommandIndex <= CommandIndex)
        {
            Low = Middle + 1;
        }
        else
        {
            High = Middle - 1;
        }
    }

    if (High < 0)
    {
        return nullptr;
    }

    const FReplayedSpan& Span = ReplayedSpans[High];
    if (CommandIndex >= Span.CommandIndex + Span.Block->GetCommandCount())
    {
        return nullptr;
    }

    OutSpanStart = Span.CommandIndex;
    return Span.Block;
}

bool FDrawCommandList::WasFullyReplayed() const
{
    return !Commands.IsEmpty() && ReplayedCommandCount == Commands.Size();
}

bool FDrawCommandList::FindFirstDifference(
    const FDrawCommandList& Left,
    const FDrawCommandList& Right,
    int32&                  OutIndex,
    String&                 OutReason)
{
    const int32 SharedCount = Math::Min(Left.Commands.Size(), Right.Commands.Size());

    for (int32 Index = 0; Index < SharedCount; ++Index)
    {
        const FDrawCommand& LeftCommand  = Left.Commands[Index];
        const FDrawCommand& RightCommand = Right.Commands[Index];

        OutIndex = Index;

        if (LeftCommand.Type != RightCommand.Type)
        {
            OutReason = "command type";
            return true;
        }

        if (LeftCommand.Flags != RightCommand.Flags || LeftCommand.LayerId != RightCommand.LayerId)
        {
            OutReason = "flags or layer";
            return true;
        }

        if (LeftCommand.Bounds != RightCommand.Bounds)
        {
            OutReason = "bounds";
            return true;
        }

        if (LeftCommand.PackedColor != RightCommand.PackedColor)
        {
            OutReason = "colour";
            return true;
        }

        if (LeftCommand.CornerRadius != RightCommand.CornerRadius || LeftCommand.Thickness != RightCommand.Thickness)
        {
            OutReason = "shape parameters";
            return true;
        }

        if (LeftCommand.Type == EDrawCommandType::Text)
        {
            if (LeftCommand.Font != RightCommand.Font)
            {
                OutReason = "font or payload size";
                return true;
            }
        }
        else if (LeftCommand.Fade[0] != RightCommand.Fade[0] || LeftCommand.Fade[1] != RightCommand.Fade[1])
        {
            OutReason = "shape parameters";
            return true;
        }

        if (LeftCommand.PayloadCount != RightCommand.PayloadCount)
        {
            OutReason = "font or payload size";
            return true;
        }

        if (LeftCommand.IsClipped() && Left.GetCommandClipRectangle(LeftCommand) != Right.GetCommandClipRectangle(RightCommand))
        {
            OutReason = "clip rectangle";
            return true;
        }

        switch (LeftCommand.Type)
        {
            case EDrawCommandType::Text:
            {
                const StringView LeftText  = Left.GetCommandText(LeftCommand);
                const StringView RightText = Right.GetCommandText(RightCommand);

                if (!LeftText.Equals(RightText))
                {
                    OutReason = "text";
                    return true;
                }

                break;
            }

            case EDrawCommandType::Polyline:
            case EDrawCommandType::ConvexPolygon:
            {
                const TArrayView<const Vector2> LeftPoints  = Left.GetCommandPoints(LeftCommand);
                const TArrayView<const Vector2> RightPoints = Right.GetCommandPoints(RightCommand);

                if (LeftPoints.Size() != RightPoints.Size()
                    || (LeftPoints.Size() > 0 && Memory::Memcmp(LeftPoints.Data(), RightPoints.Data(),
                        LeftPoints.Size() * static_cast<int32>(sizeof(Vector2))) != 0))
                {
                    OutReason = "points";
                    return true;
                }

                break;
            }

            case EDrawCommandType::Image:
            {
                const FUIBrush* LeftBrush  = Left.GetCommandBrush(LeftCommand);
                const FUIBrush* RightBrush = Right.GetCommandBrush(RightCommand);

                if (!LeftBrush != !RightBrush)
                {
                    OutReason = "brush presence";
                    return true;
                }

                if (LeftBrush && Memory::Memcmp(LeftBrush, RightBrush, sizeof(FUIBrush)) != 0)
                {
                    OutReason = "brush";
                    return true;
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    if (Left.Commands.Size() != Right.Commands.Size())
    {
        OutIndex  = SharedCount;
        OutReason = "command count";
        return true;
    }

    return false;
}

TArrayView<const Vector2> FDrawCommandList::GetCommandPoints(const FDrawCommand& Command) const
{
    if ((Command.Type != EDrawCommandType::Polyline && Command.Type != EDrawCommandType::ConvexPolygon) || Command.PayloadCount <= 0)
    {
        return TArrayView<const Vector2>();
    }

    return TArrayView<const Vector2>(Points.Data() + Command.PayloadOffset, Command.PayloadCount);
}

StringView FDrawCommandList::GetCommandText(const FDrawCommand& Command) const
{
    if (Command.Type != EDrawCommandType::Text || Command.PayloadCount <= 0)
    {
        return StringView();
    }

    return StringView(TextPool.Data() + Command.PayloadOffset, Command.PayloadCount);
}

const FUIBrush* FDrawCommandList::GetCommandBrush(const FDrawCommand& Command) const
{
    if (Command.Type != EDrawCommandType::Image || Command.PayloadCount <= 0)
    {
        return nullptr;
    }

    return &Brushes[Command.PayloadOffset];
}

const FRectangle& FDrawCommandList::GetCommandClipRectangle(const FDrawCommand& Command) const
{
    if (!Command.IsClipped() || Command.ClipId == 0)
    {
        return EmptyClipRectangle;
    }

    CHECK(Command.ClipId != InheritedClipId && Command.ClipId <= static_cast<uint16>(ClipRects.Size()));
    return ClipRects[Command.ClipId - 1];
}

int32 FDrawCommandList::CountCommandsOfType(EDrawCommandType Type) const
{
    int32 Count = 0;
    for (const FDrawCommand& Command : Commands)
    {
        if (Command.Type == Type)
        {
            Count++;
        }
    }

    return Count;
}

int32 FDrawCommandList::FindTextCommand(const StringView& InText) const
{
    for (int32 Index = 0; Index < Commands.Size(); Index++)
    {
        const FDrawCommand& Command = Commands[Index];
        if (Command.Type == EDrawCommandType::Text && GetCommandText(Command).Equals(InText.Data(), InText.Length()))
        {
            return Index;
        }
    }

    return InvalidIndex;
}

void FDrawCommandList::StorePoints(FDrawCommand& Command, TArrayView<const Vector2> InPoints)
{
    Command.PayloadOffset = Points.Size();
    Command.PayloadCount  = InPoints.Size();

    const int32 Start = Points.Size();
    Points.AppendUninitialized(InPoints.Size());
    Memory::Memcpy(Points.Data() + Start, InPoints.Data(), static_cast<uint32>(InPoints.Size()) * sizeof(Vector2));
}

void FDrawCommandList::StoreText(FDrawCommand& Command, StringView InText)
{
    Command.PayloadOffset = TextPool.Size();
    Command.PayloadCount  = InText.Length();

    if (InText.Length() <= 0)
    {
        return;
    }

    const int32 Start = TextPool.Size();
    TextPool.AppendUninitialized(InText.Length());
    Memory::Memcpy(TextPool.Data() + Start, InText.Data(), static_cast<uint32>(InText.Length()) * sizeof(CHAR));
}

void FDrawCommandList::BuildArcPoints(const Vector2& Center, float Radius, float StartAngle, float EndAngle, int32 Segments)
{
    ScratchPoints.Clear();

    const int32 PointCount = Math::Max(Segments, 1) + 1;
    ReserveAtLeast(ScratchPoints, PointCount);

    const float AngleStep = (EndAngle - StartAngle) / static_cast<float>(Math::Max(Segments, 1));

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const float Angle = StartAngle + (AngleStep * static_cast<float>(Index));
        ScratchPoints.Add(Vector2(Center.X + (Radius * Math::Cos(Angle)), Center.Y + (Radius * Math::Sin(Angle))));
    }
}

int32 FDrawCommandList::ResolveCircleSegments(float Radius, float AngleSweep, int32 RequestedSegments)
{
    if (RequestedSegments > 0)
    {
        return Math::Clamp(RequestedSegments, MinCircleSegments, MaxCircleSegments);
    }

    const float ArcLength = Radius * Math::Max(Math::Abs(AngleSweep), 0.0001f);
    return Math::Clamp(static_cast<int32>(ArcLength * 0.35f), MinCircleSegments, MaxCircleSegments);
}
