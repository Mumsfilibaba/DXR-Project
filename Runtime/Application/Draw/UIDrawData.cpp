#include "Application/Draw/UIDrawData.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"
#include "Core/Math/Math.h"

constexpr float DIRECTION_EPSILON = 1.0e-4f;

constexpr uint32 PACKED_ALPHA_MASK = 0xff000000u;
constexpr uint32 PACKED_COLOR_MASK = 0x00ffffffu;

static uint32 PackColorWithAlphaScale(uint32 PackedColor, float Scale)
{
    const float  Alpha  = static_cast<float>((PackedColor & PACKED_ALPHA_MASK) >> 24);
    const uint32 Scaled = static_cast<uint32>(Math::Clamp(Math::RoundToInt(Alpha * Scale), 0, 255));

    return (PackedColor & PACKED_COLOR_MASK) | (Scaled << 24);
}

FUIDrawData::FUIDrawData()
    : Vertices()
    , Indices()
    , Batches()
    , ScratchPoints()
    , ScratchOffsets()
    , ActiveClipRectangle()
    , bHasActiveClip(false)
    , bAntiAliasingEnabled(true)
{
}

FUIDrawData::~FUIDrawData() = default;

void FUIDrawData::Reset()
{
    Vertices.Clear();
    Indices.Clear();
    Batches.Clear();
    ScratchPoints.Clear();
    ScratchOffsets.Clear();

    ActiveClipRectangle = FRectangle();
    bHasActiveClip      = false;
}

void FUIDrawData::SetAntiAliasingEnabled(bool bEnabled)
{
    bAntiAliasingEnabled = bEnabled;
}

void FUIDrawData::BuildFromCommandList(const FDrawCommandList& CommandList)
{
    Reset();

    const TArray<FDrawCommand>& Commands = CommandList.GetCommands();
    if (Commands.IsEmpty())
    {
        return;
    }

    TArray<int32> SortedIndices;
    SortedIndices.Reserve(Commands.Size());

    for (int32 Index = 0; Index < Commands.Size(); ++Index)
    {
        SortedIndices.Add(Index);
    }

    SortedIndices.SortWithPredicate([&Commands](int32 First, int32 Second)
    {
        if (Commands[First].LayerId != Commands[Second].LayerId)
        {
            return Commands[First].LayerId < Commands[Second].LayerId;
        }

        return First < Second;
    });

    for (int32 SortedIndex = 0; SortedIndex < SortedIndices.Size(); ++SortedIndex)
    {
        const FDrawCommand& Command = Commands[SortedIndices[SortedIndex]];

        ActiveClipRectangle = Command.ClipRectangle;
        bHasActiveClip      = Command.bIsClipped;

        switch (Command.Type)
        {
            case EDrawCommandType::Box:
            case EDrawCommandType::Line:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddBox(Command);
                }

                break;
            }

            case EDrawCommandType::BoxOutline:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddBoxOutline(Command);
                }

                break;
            }

            case EDrawCommandType::Text:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddText(Command);
                }

                break;
            }

            case EDrawCommandType::Image:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddImage(Command);
                }

                break;
            }

            case EDrawCommandType::RoundedBottomBar:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddRoundedBottomBar(Command);
                }

                break;
            }

            case EDrawCommandType::RoundedAccentRing:
            {
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddRoundedAccentRing(Command);
                }

                break;
            }

            case EDrawCommandType::Polyline:
            {
                const TArrayView<const Vector2> Points = CommandList.GetCommandPoints(Command);
                if (!IsCulledByClip(ComputePointBounds(Points, Command.Thickness)))
                {
                    GetOrOpenBatch(FUITextureHandle());
                    AddPolyline(Points, Command.Thickness, Command.bIsClosed, Command.Tint.ToColor().ToPackedRGBA());
                }

                break;
            }

            case EDrawCommandType::ConvexPolygon:
            {
                const TArrayView<const Vector2> Points = CommandList.GetCommandPoints(Command);
                if (!IsCulledByClip(ComputePointBounds(Points, 0.0f)))
                {
                    GetOrOpenBatch(FUITextureHandle());
                    AddConvexPolygon(Points, Command.Tint.ToColor().ToPackedRGBA());
                }

                break;
            }

            case EDrawCommandType::ClipPush:
            case EDrawCommandType::ClipPop:
            {
                break;
            }
        }
    }
}

bool FUIDrawData::IsCulledByClip(const FRectangle& Bounds) const
{
    if (!bHasActiveClip)
    {
        return false;
    }

    if (Bounds.IsEmpty())
    {
        return false;
    }

    return ActiveClipRectangle.Intersect(Bounds).IsEmpty();
}

FRectangle FUIDrawData::ComputePointBounds(TArrayView<const Vector2> Points, float Thickness)
{
    if (Points.IsEmpty())
    {
        return FRectangle();
    }

    float MinX = Points[0].X;
    float MinY = Points[0].Y;
    float MaxX = Points[0].X;
    float MaxY = Points[0].Y;

    for (const Vector2& Point : Points)
    {
        MinX = Math::Min(MinX, Point.X);
        MinY = Math::Min(MinY, Point.Y);
        MaxX = Math::Max(MaxX, Point.X);
        MaxY = Math::Max(MaxY, Point.Y);
    }

    const float Spill  = Thickness * MiterLimit * 0.5f;
    const int32 Left   = Math::FloorToInt(MinX - Spill);
    const int32 Top    = Math::FloorToInt(MinY - Spill);
    const int32 Right  = Math::CeilToInt(MaxX + Spill);
    const int32 Bottom = Math::CeilToInt(MaxY + Spill);

    return FRectangle(IntVector2(Left, Top), Math::Max(Right - Left, 1), Math::Max(Bottom - Top, 1));
}

void FUIDrawData::AddBox(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty())
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle());

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);
    if (!Radius.IsZero())
    {
        AddRoundedBox(Command.Bounds, Radius, Command.Tint.ToColor().ToPackedRGBA());
        return;
    }

    AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.Tint.ToColor().ToPackedRGBA());
}

void FUIDrawData::AddBoxOutline(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty() || Command.Thickness <= 0.0f)
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle());

    const float HalfThickness = Command.Thickness * 0.5f;

    if (Command.Thickness >= static_cast<float>(Math::Min(Command.Bounds.Width, Command.Bounds.Height)))
    {
        AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.Tint.ToColor().ToPackedRGBA());
        return;
    }

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);

    BuildRoundedBoxOutline(Command.Bounds, Radius, ScratchPoints, HalfThickness);
    AddPolyline(ScratchPoints, Command.Thickness, true, Command.Tint.ToColor().ToPackedRGBA());
}

void FUIDrawData::AddText(const FDrawCommand& Command)
{
    if (!Command.Font || Command.Text.IsEmpty())
    {
        return;
    }

    const FFontAtlas* Atlas = Command.Font->GetAtlas();
    if (!Atlas || !Atlas->IsValid())
    {
        return;
    }

    const FShapedRun& ShapedRun = Command.Font->ShapeText(StringView(Command.Text.Data(), Command.Text.Length()));
    GetOrOpenBatch(FUITextureHandle(Atlas));

    const float  AtlasWidth  = static_cast<float>(Atlas->GetWidth());
    const float  AtlasHeight = static_cast<float>(Atlas->GetHeight());
    const uint32 PackedColor = Command.Tint.ToColor().ToPackedRGBA();

    const int32 PenX      = Command.Bounds.Position.X;
    const int32 BaselineY = Command.Bounds.Position.Y + Command.Font->GetTextBandOffset(Command.Bounds.Height) + Command.Font->GetAscent();

    for (const FShapedGlyph& Shaped : ShapedRun.Glyphs)
    {
        if (!Shaped.Glyph || Shaped.Glyph->AtlasRectangle.IsEmpty())
        {
            continue;
        }

        const FGlyph& Glyph = *Shaped.Glyph;

        const FRectangle GlyphBounds(IntVector2(PenX + Shaped.Offset + Glyph.BearingX, BaselineY + Glyph.BearingY),
            Glyph.AtlasRectangle.Width, Glyph.AtlasRectangle.Height);

        const Vector2 MinTexCoord(
            static_cast<float>(Glyph.AtlasRectangle.Position.X) / AtlasWidth,
            static_cast<float>(Glyph.AtlasRectangle.Position.Y) / AtlasHeight);

        const Vector2 MaxTexCoord(
            static_cast<float>(Glyph.AtlasRectangle.GetRight()) / AtlasWidth,
            static_cast<float>(Glyph.AtlasRectangle.GetBottom()) / AtlasHeight);

        AddQuad(GlyphBounds, MinTexCoord, MaxTexCoord, PackedColor);
    }
}

void FUIDrawData::AddImage(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty())
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle(Command.Brush.Texture));

    const uint32   PackedColor = Command.Tint.ToColor().ToPackedRGBA();
    const FUIBrush Brush       = Command.Brush;

    if (!Brush.IsNineSlice())
    {
        const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);
        if (!Radius.IsZero())
        {
            AddRoundedBox(Command.Bounds, Radius, PackedColor, Brush.MinTexCoord, Brush.MaxTexCoord);
            return;
        }

        AddQuad(Command.Bounds, Brush.MinTexCoord, Brush.MaxTexCoord, PackedColor);
        return;
    }

    const FMargin& Margin = Brush.Margin;

    const int32 LeftWidth    = Math::Min(Margin.Left, Command.Bounds.Width);
    const int32 RightWidth   = Math::Min(Margin.Right, Command.Bounds.Width - LeftWidth);
    const int32 TopHeight    = Math::Min(Margin.Top, Command.Bounds.Height);
    const int32 BottomHeight = Math::Min(Margin.Bottom, Command.Bounds.Height - TopHeight);

    const int32 ColumnX[4] =
    {
        Command.Bounds.Position.X,
        Command.Bounds.Position.X + LeftWidth,
        Command.Bounds.GetRight() - RightWidth,
        Command.Bounds.GetRight(),
    };

    const int32 RowY[4] =
    {
        Command.Bounds.Position.Y,
        Command.Bounds.Position.Y + TopHeight,
        Command.Bounds.GetBottom() - BottomHeight,
        Command.Bounds.GetBottom(),
    };

    const float TexWidth   = Brush.MaxTexCoord.X - Brush.MinTexCoord.X;
    const float TexHeight  = Brush.MaxTexCoord.Y - Brush.MinTexCoord.Y;
    const float DestWidth  = static_cast<float>(Math::Max(Command.Bounds.Width, 1));
    const float DestHeight = static_cast<float>(Math::Max(Command.Bounds.Height, 1));

    const float ColumnU[4] =
    {
        Brush.MinTexCoord.X,
        Brush.MinTexCoord.X + (TexWidth * (static_cast<float>(LeftWidth) / DestWidth)),
        Brush.MaxTexCoord.X - (TexWidth * (static_cast<float>(RightWidth) / DestWidth)),
        Brush.MaxTexCoord.X,
    };

    const float RowV[4] =
    {
        Brush.MinTexCoord.Y,
        Brush.MinTexCoord.Y + (TexHeight * (static_cast<float>(TopHeight) / DestHeight)),
        Brush.MaxTexCoord.Y - (TexHeight * (static_cast<float>(BottomHeight) / DestHeight)),
        Brush.MaxTexCoord.Y,
    };

    for (int32 Row = 0; Row < 3; ++Row)
    {
        for (int32 Column = 0; Column < 3; ++Column)
        {
            const int32 PatchWidth  = ColumnX[Column + 1] - ColumnX[Column];
            const int32 PatchHeight = RowY[Row + 1] - RowY[Row];

            if (PatchWidth <= 0 || PatchHeight <= 0)
            {
                continue;
            }

            AddQuad(FRectangle(IntVector2(ColumnX[Column], RowY[Row]), PatchWidth, PatchHeight), Vector2(ColumnU[Column], RowV[Row]),
                Vector2(ColumnU[Column + 1], RowV[Row + 1]), PackedColor);
        }
    }
}

void FUIDrawData::AddRoundedBottomBar(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty() || Command.Thickness <= 0.0f)
    {
        return;
    }

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);

    const float Left        = static_cast<float>(Command.Bounds.Position.X);
    const float Right       = static_cast<float>(Command.Bounds.GetRight());
    const float Bottom      = static_cast<float>(Command.Bounds.GetBottom());
    const float Thickness   = Math::Min(Command.Thickness, static_cast<float>(Command.Bounds.Height));
    const float Top         = Bottom - Thickness;
    const float LeftCenter  = Left + Radius.BottomLeft;
    const float RightCenter = Right - Radius.BottomRight;

    const float LeftTip = (Thickness < Radius.BottomLeft)
        ? LeftCenter - Math::Sqrt(Thickness * ((2.0f * Radius.BottomLeft) - Thickness))
        : Left;

    const float RightTip = (Thickness < Radius.BottomRight)
        ? RightCenter + Math::Sqrt(Thickness * ((2.0f * Radius.BottomRight) - Thickness))
        : Right;

    const float BarWidth = RightTip - LeftTip;
    if (BarWidth <= 0.0f)
    {
        return;
    }

    const int32 ColumnCount  = Math::Clamp(Math::CeilToInt(BarWidth * 0.5f), MinBarColumns, MaxBarColumns);
    const int32 RowsPerColumn = bAntiAliasingEnabled ? 3 : 2;

    if (Vertices.Size() + ((ColumnCount + 1) * RowsPerColumn) > MaxVertexCount)
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle());

    const uint32 BaseVertex = static_cast<uint32>(Vertices.Size());
    const float  FadeWidth  = Math::Min(Command.FadeWidth, BarWidth * 0.5f);
    const float  HalfFringe = FringeWidth * 0.5f;

    for (int32 ColumnIndex = 0; ColumnIndex <= ColumnCount; ++ColumnIndex)
    {
        const float X = LeftTip + (BarWidth * (static_cast<float>(ColumnIndex) / static_cast<float>(ColumnCount)));

        float ColumnBottom = Bottom;
        float CosineSlope  = 1.0f;

        if (X < LeftCenter)
        {
            const float Offset = LeftCenter - X;
            const float Height = Math::Sqrt(Math::Max((Radius.BottomLeft * Radius.BottomLeft) - (Offset * Offset), 0.0f));

            ColumnBottom = (Bottom - Radius.BottomLeft) + Height;
            CosineSlope  = Radius.BottomLeft > 0.0f ? (Height / Radius.BottomLeft) : 1.0f;
        }
        else if (X > RightCenter)
        {
            const float Offset = X - RightCenter;
            const float Height = Math::Sqrt(Math::Max((Radius.BottomRight * Radius.BottomRight) - (Offset * Offset), 0.0f));

            ColumnBottom = (Bottom - Radius.BottomRight) + Height;
            CosineSlope  = Radius.BottomRight > 0.0f ? (Height / Radius.BottomRight) : 1.0f;
        }

        const float EdgeDistance = Math::Min(X - LeftTip, RightTip - X);

        FFloatColor ColumnTint = Command.Tint;
        ColumnTint.A *= (FadeWidth > 0.0f) ? Math::Clamp(EdgeDistance / FadeWidth, 0.0f, 1.0f) : 1.0f;

        const uint32 PackedColor   = ColumnTint.ToColor().ToPackedRGBA();
        const float  ClampedBottom = Math::Max(ColumnBottom, Top);

        if (!bAntiAliasingEnabled)
        {
            EmplaceVertex(Vector2(X, Top), PackedColor);
            EmplaceVertex(Vector2(X, ClampedBottom), PackedColor);
            continue;
        }

        const float Drop = HalfFringe / Math::Max(CosineSlope, 0.1f);
        EmplaceVertex(Vector2(X, Top), PackedColor);
        EmplaceVertex(Vector2(X, Math::Max(ClampedBottom - Drop, Top)), PackedColor);
        EmplaceVertex(Vector2(X, ClampedBottom + Drop), PackedColor & PACKED_COLOR_MASK);
    }

    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        const uint32 LeadingTop = BaseVertex + static_cast<uint32>(ColumnIndex * RowsPerColumn);
        const uint32 NextTop    = LeadingTop + static_cast<uint32>(RowsPerColumn);

        Indices.Add(LeadingTop);
        Indices.Add(NextTop);
        Indices.Add(NextTop + 1);
        Indices.Add(LeadingTop);
        Indices.Add(NextTop + 1);
        Indices.Add(LeadingTop + 1);

        if (bAntiAliasingEnabled)
        {
            Indices.Add(LeadingTop + 1);
            Indices.Add(NextTop + 1);
            Indices.Add(NextTop + 2);
            Indices.Add(LeadingTop + 1);
            Indices.Add(NextTop + 2);
            Indices.Add(LeadingTop + 2);
        }
    }

    Batches.Last().IndexCount += ColumnCount * (bAntiAliasingEnabled ? 12 : 6);
}

void FUIDrawData::AddRoundedAccentRing(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty() || Command.Thickness <= 0.0f)
    {
        return;
    }

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);

    const float Left      = static_cast<float>(Command.Bounds.Position.X);
    const float Right     = static_cast<float>(Command.Bounds.GetRight());
    const float Top       = static_cast<float>(Command.Bounds.Position.Y);
    const float Bottom    = static_cast<float>(Command.Bounds.GetBottom());
    const float Thickness = Math::Min(Command.Thickness,
        static_cast<float>(Math::Min(Command.Bounds.Width, Command.Bounds.Height)) * 0.5f);

    const float CornerRadius[4] =
    {
        Math::Max(Radius.TopLeft, 0.0f),
        Math::Max(Radius.TopRight, 0.0f),
        Math::Max(Radius.BottomRight, 0.0f),
        Math::Max(Radius.BottomLeft, 0.0f),
    };

    const Vector2 CornerCenter[4] =
    {
        Vector2(Left + CornerRadius[0], Top + CornerRadius[0]),
        Vector2(Right - CornerRadius[1], Top + CornerRadius[1]),
        Vector2(Right - CornerRadius[2], Bottom - CornerRadius[2]),
        Vector2(Left + CornerRadius[3], Bottom - CornerRadius[3]),
    };

    const Vector2 SquareCorner[4] =
    {
        Vector2(Left, Top),
        Vector2(Right, Top),
        Vector2(Right, Bottom),
        Vector2(Left, Bottom),
    };

    int32 CornerSegments[4];
    int32 SampleCount = 0;

    for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        CornerSegments[CornerIndex] = (CornerRadius[CornerIndex] > 0.0f)
            ? Math::Clamp(static_cast<int32>(CornerRadius[CornerIndex] * 1.5f), MinCornerSegments, MaxCornerSegments)
            : 0;

        SampleCount += CornerSegments[CornerIndex] + 1;
    }

    const int32 RowsPerSample = bAntiAliasingEnabled ? 3 : 2;
    if (Vertices.Size() + (SampleCount * RowsPerSample) > MaxVertexCount)
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle());

    const uint32 BaseVertex   = static_cast<uint32>(Vertices.Size());
    const float  HalfFringe   = FringeWidth * 0.5f;
    const float  FadeFraction = Math::Clamp(Command.FadeFraction, 0.0f, 1.0f);
    const float  TrailAlpha   = Math::Clamp(Command.TrailAlpha, 0.0f, 1.0f);

    for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        const bool  bIsTopCorner = CornerIndex < 2;
        const int32 Segments     = CornerSegments[CornerIndex];
        const float ThisRadius   = CornerRadius[CornerIndex];
        const float StartAngle   = Math::Constants::PI + (static_cast<float>(CornerIndex) * Math::Constants::HalfPI);

        for (int32 StepIndex = 0; StepIndex <= Segments; ++StepIndex)
        {
            const float Sweep = (Segments > 0)
                ? static_cast<float>(StepIndex) / static_cast<float>(Segments)
                : 0.0f;

            const float Along = bIsTopCorner
                ? ((Segments == 0) ? 1.0f : ((CornerIndex == 0) ? Sweep : 1.0f - Sweep))
                : 0.0f;

            Vector2 Outer;
            Vector2 Normal;
            float   Reach = Thickness;

            if (ThisRadius > 0.0f)
            {
                const float Angle = StartAngle + (Math::Constants::HalfPI * Sweep);

                Normal = Vector2(Math::Cos(Angle), Math::Sin(Angle));
                Outer  = Vector2(CornerCenter[CornerIndex].X + (ThisRadius * Normal.X),
                    CornerCenter[CornerIndex].Y + (ThisRadius * Normal.Y));
            }
            else
            {
                const float Diagonal = Math::Constants::PI + (static_cast<float>(CornerIndex) * Math::Constants::HalfPI)
                    + (Math::Constants::HalfPI * 0.5f);

                Normal = Vector2(Math::Cos(Diagonal), Math::Sin(Diagonal));
                Outer  = SquareCorner[CornerIndex];
                Reach  = Thickness * Math::Constants::Sqrt2;
            }

            const float Strength = (FadeFraction > 0.0f)
                ? Math::Clamp(Along / FadeFraction, 0.0f, 1.0f)
                : ((Along >= 1.0f) ? 1.0f : 0.0f);

            FFloatColor SampleTint = Command.Tint;
            SampleTint.A *= TrailAlpha + ((1.0f - TrailAlpha) * Strength);

            const uint32  PackedColor = SampleTint.ToColor().ToPackedRGBA();
            const Vector2 Inner(Outer.X - (Normal.X * Reach), Outer.Y - (Normal.Y * Reach));

            if (!bAntiAliasingEnabled)
            {
                EmplaceVertex(Inner, PackedColor);
                EmplaceVertex(Outer, PackedColor);
                continue;
            }

            EmplaceVertex(Inner, PackedColor);
            EmplaceVertex(Vector2(Outer.X - (Normal.X * HalfFringe), Outer.Y - (Normal.Y * HalfFringe)), PackedColor);
            EmplaceVertex(Vector2(Outer.X + (Normal.X * HalfFringe), Outer.Y + (Normal.Y * HalfFringe)),
                PackedColor & PACKED_COLOR_MASK);
        }
    }

    for (int32 Segment = 0; Segment < SampleCount; ++Segment)
    {
        const uint32 LeadingInner = BaseVertex + static_cast<uint32>(Segment * RowsPerSample);
        const uint32 NextInner    = BaseVertex + static_cast<uint32>(((Segment + 1) % SampleCount) * RowsPerSample);

        Indices.Add(LeadingInner);
        Indices.Add(NextInner);
        Indices.Add(NextInner + 1);
        Indices.Add(LeadingInner);
        Indices.Add(NextInner + 1);
        Indices.Add(LeadingInner + 1);

        if (bAntiAliasingEnabled)
        {
            Indices.Add(LeadingInner + 1);
            Indices.Add(NextInner + 1);
            Indices.Add(NextInner + 2);
            Indices.Add(LeadingInner + 1);
            Indices.Add(NextInner + 2);
            Indices.Add(LeadingInner + 2);
        }
    }

    Batches.Last().IndexCount += SampleCount * (bAntiAliasingEnabled ? 12 : 6);
}

void FUIDrawData::EmplaceVertex(const Vector2& Position, uint32 PackedColor)
{
    FUIVertex& Vertex = Vertices.Emplace();
    Vertex.Position   = Position;
    Vertex.TexCoord   = Vector2(0.5f, 0.5f);
    Vertex.Color      = PackedColor;
}

void FUIDrawData::EmplaceFillVertex(const Vector2& Position, const FRectangle& Bounds, uint32 PackedColor,
    const Vector2& MinTexCoord, const Vector2& MaxTexCoord)
{
    const float MinX   = static_cast<float>(Bounds.Position.X);
    const float MinY   = static_cast<float>(Bounds.Position.Y);
    const float MaxX   = static_cast<float>(Bounds.GetRight());
    const float MaxY   = static_cast<float>(Bounds.GetBottom());
    const float AlphaX = (Position.X - MinX) / (MaxX - MinX);
    const float AlphaY = (Position.Y - MinY) / (MaxY - MinY);

    FUIVertex& Vertex = Vertices.Emplace();
    Vertex.Position   = Position;
    Vertex.TexCoord   = Vector2(
        MinTexCoord.X + (AlphaX * (MaxTexCoord.X - MinTexCoord.X)),
        MinTexCoord.Y + (AlphaY * (MaxTexCoord.Y - MinTexCoord.Y)));
    Vertex.Color      = PackedColor;
}

void FUIDrawData::BuildMiterOffsets(TArrayView<const Vector2> Points, bool bClosed, TArray<Vector2>& OutOffsets)
{
    OutOffsets.Clear();

    const int32 PointCount = Points.Size();
    OutOffsets.Reserve(PointCount);

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const bool bHasPrevious = bClosed || Index > 0;
        const bool bHasNext     = bClosed || Index < PointCount - 1;

        Vector2 IncomingNormal;
        Vector2 OutgoingNormal;

        if (bHasPrevious)
        {
            const Vector2 Previous  = Points[(Index - 1 + PointCount) % PointCount];
            const Vector2 Direction = (Points[Index] - Previous).GetNormalized();
            IncomingNormal          = Vector2(-Direction.Y, Direction.X);
        }

        if (bHasNext)
        {
            const Vector2 Next      = Points[(Index + 1) % PointCount];
            const Vector2 Direction = (Next - Points[Index]).GetNormalized();
            OutgoingNormal          = Vector2(-Direction.Y, Direction.X);
        }

        if (!bHasPrevious)
        {
            IncomingNormal = OutgoingNormal;
        }

        if (!bHasNext)
        {
            OutgoingNormal = IncomingNormal;
        }

        Vector2 Miter = IncomingNormal + OutgoingNormal;
        float   Scale = 1.0f;

        const float MiterLength = Miter.GetLength();
        if (MiterLength > DIRECTION_EPSILON)
        {
            Miter = Miter * (1.0f / MiterLength);

            const float CosHalfAngle = Miter.DotProduct(IncomingNormal);
            if (CosHalfAngle > DIRECTION_EPSILON)
            {
                Scale = Math::Min(1.0f / CosHalfAngle, MiterLimit);
            }
        }
        else
        {
            Miter = IncomingNormal;
        }

        OutOffsets.Add(Miter * Scale);
    }
}

float FUIDrawData::ComputeWindingSign(TArrayView<const Vector2> Points)
{
    const int32 PointCount = Points.Size();

    float DoubleArea = 0.0f;
    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const Vector2& Current = Points[Index];
        const Vector2& Next    = Points[(Index + 1) % PointCount];

        DoubleArea += (Current.X * Next.Y) - (Next.X * Current.Y);
    }

    return DoubleArea > 0.0f ? -1.0f : 1.0f;
}

void FUIDrawData::AddPolyline(TArrayView<const Vector2> Points, float Thickness, bool bClosed, uint32 PackedColor)
{
    const int32 PointCount = Points.Size();
    if (PointCount < 2 || Thickness <= 0.0f)
    {
        return;
    }

    const int32 RowsPerPoint = bAntiAliasingEnabled ? 4 : 2;
    if (Vertices.Size() + (PointCount * RowsPerPoint) > MaxVertexCount)
    {
        return;
    }

    const float  HalfThickness = Thickness * 0.5f;
    const int32  SegmentCount  = bClosed ? PointCount : PointCount - 1;
    const uint32 BaseVertex    = static_cast<uint32>(Vertices.Size());

    BuildMiterOffsets(Points, bClosed, ScratchOffsets);

    if (!bAntiAliasingEnabled)
    {
        for (int32 Index = 0; Index < PointCount; ++Index)
        {
            const Vector2 Offset = ScratchOffsets[Index] * HalfThickness;

            FUIVertex& Outer = Vertices.Emplace();
            Outer.Position   = Points[Index] + Offset;
            Outer.TexCoord   = Vector2(0.5f, 0.5f);
            Outer.Color      = PackedColor;

            FUIVertex& Inner = Vertices.Emplace();
            Inner.Position   = Points[Index] - Offset;
            Inner.TexCoord   = Vector2(0.5f, 0.5f);
            Inner.Color      = PackedColor;
        }

        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            const uint32 CurrentOuter = BaseVertex + static_cast<uint32>(Segment * 2);
            const uint32 CurrentInner = CurrentOuter + 1;
            const uint32 NextOuter    = BaseVertex + static_cast<uint32>(((Segment + 1) % PointCount) * 2);
            const uint32 NextInner    = NextOuter + 1;

            Indices.Add(CurrentOuter);
            Indices.Add(NextOuter);
            Indices.Add(NextInner);
            Indices.Add(CurrentOuter);
            Indices.Add(NextInner);
            Indices.Add(CurrentInner);
        }

        Batches.Last().IndexCount += SegmentCount * 6;
        return;
    }

    const float HalfFringe  = FringeWidth * 0.5f;
    const bool  bIsHairline = Thickness <= FringeWidth;
    const float CoreExtent  = bIsHairline ? 0.0f : (HalfThickness - HalfFringe);
    const float EdgeExtent  = bIsHairline ? FringeWidth : (HalfThickness + HalfFringe);

    const uint32 CoreColor  = bIsHairline
        ? PackColorWithAlphaScale(PackedColor, Thickness / FringeWidth)
        : PackedColor;

    const uint32 ClearColor = PackedColor & PACKED_COLOR_MASK;

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const Vector2& Miter = ScratchOffsets[Index];

        const Vector2 CoreOffset = Miter * CoreExtent;
        const Vector2 EdgeOffset = Miter * EdgeExtent;

        EmplaceVertex(Points[Index] + EdgeOffset, ClearColor);
        EmplaceVertex(Points[Index] + CoreOffset, CoreColor);
        EmplaceVertex(Points[Index] - CoreOffset, CoreColor);
        EmplaceVertex(Points[Index] - EdgeOffset, ClearColor);
    }

    for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
    {
        const uint32 Current = BaseVertex + static_cast<uint32>(Segment * 4);
        const uint32 Next    = BaseVertex + static_cast<uint32>(((Segment + 1) % PointCount) * 4);

        for (uint32 Row = 0; Row < 3; ++Row)
        {
            Indices.Add(Current + Row);
            Indices.Add(Next + Row);
            Indices.Add(Next + Row + 1);
            Indices.Add(Current + Row);
            Indices.Add(Next + Row + 1);
            Indices.Add(Current + Row + 1);
        }
    }

    Batches.Last().IndexCount += SegmentCount * 18;
}

void FUIDrawData::AddConvexPolygon(TArrayView<const Vector2> Points, uint32 PackedColor)
{
    const int32 PointCount = Points.Size();
    if (PointCount < 3)
    {
        return;
    }

    const int32 RingCount = bAntiAliasingEnabled ? 2 : 1;
    if (Vertices.Size() + (PointCount * RingCount) > MaxVertexCount)
    {
        return;
    }

    const uint32 BaseVertex = static_cast<uint32>(Vertices.Size());

    if (!bAntiAliasingEnabled)
    {
        for (int32 Index = 0; Index < PointCount; ++Index)
        {
            EmplaceVertex(Points[Index], PackedColor);
        }

        for (int32 Index = 2; Index < PointCount; ++Index)
        {
            Indices.Add(BaseVertex);
            Indices.Add(BaseVertex + static_cast<uint32>(Index - 1));
            Indices.Add(BaseVertex + static_cast<uint32>(Index));
        }

        Batches.Last().IndexCount += (PointCount - 2) * 3;
        return;
    }

    const uint32 ClearColor = PackedColor & PACKED_COLOR_MASK;
    const float  HalfFringe = FringeWidth * 0.5f;
    const float  Winding    = ComputeWindingSign(Points);

    BuildMiterOffsets(Points, true, ScratchOffsets);

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const Vector2 Outward = ScratchOffsets[Index] * (Winding * HalfFringe);

        EmplaceVertex(Points[Index] - Outward, PackedColor);
        EmplaceVertex(Points[Index] + Outward, ClearColor);
    }

    for (int32 Index = 2; Index < PointCount; ++Index)
    {
        Indices.Add(BaseVertex);
        Indices.Add(BaseVertex + static_cast<uint32>((Index - 1) * 2));
        Indices.Add(BaseVertex + static_cast<uint32>(Index * 2));
    }

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        const uint32 Inner     = BaseVertex + static_cast<uint32>(Index * 2);
        const uint32 Outer     = Inner + 1;
        const uint32 NextInner = BaseVertex + static_cast<uint32>(((Index + 1) % PointCount) * 2);
        const uint32 NextOuter = NextInner + 1;

        Indices.Add(Inner);
        Indices.Add(Outer);
        Indices.Add(NextOuter);
        Indices.Add(Inner);
        Indices.Add(NextOuter);
        Indices.Add(NextInner);
    }

    Batches.Last().IndexCount += ((PointCount - 2) * 3) + (PointCount * 6);
}

void FUIDrawData::AddQuad(const FRectangle& Bounds, const Vector2& MinTexCoord, const Vector2& MaxTexCoord, uint32 PackedColor)
{
    if (Vertices.Size() + 4 > MaxVertexCount)
    {
        return;
    }

    const uint32 BaseVertex = static_cast<uint32>(Vertices.Size());

    const float MinX = static_cast<float>(Bounds.Position.X);
    const float MinY = static_cast<float>(Bounds.Position.Y);
    const float MaxX = static_cast<float>(Bounds.GetRight());
    const float MaxY = static_cast<float>(Bounds.GetBottom());

    FUIVertex& TopLeft = Vertices.Emplace();
    TopLeft.Position   = Vector2(MinX, MinY);
    TopLeft.TexCoord   = Vector2(MinTexCoord.X, MinTexCoord.Y);
    TopLeft.Color      = PackedColor;

    FUIVertex& TopRight = Vertices.Emplace();
    TopRight.Position   = Vector2(MaxX, MinY);
    TopRight.TexCoord   = Vector2(MaxTexCoord.X, MinTexCoord.Y);
    TopRight.Color      = PackedColor;

    FUIVertex& BottomRight = Vertices.Emplace();
    BottomRight.Position   = Vector2(MaxX, MaxY);
    BottomRight.TexCoord   = Vector2(MaxTexCoord.X, MaxTexCoord.Y);
    BottomRight.Color      = PackedColor;

    FUIVertex& BottomLeft = Vertices.Emplace();
    BottomLeft.Position   = Vector2(MinX, MaxY);
    BottomLeft.TexCoord   = Vector2(MinTexCoord.X, MaxTexCoord.Y);
    BottomLeft.Color      = PackedColor;

    Indices.Add(BaseVertex + 0);
    Indices.Add(BaseVertex + 1);
    Indices.Add(BaseVertex + 2);
    Indices.Add(BaseVertex + 0);
    Indices.Add(BaseVertex + 2);
    Indices.Add(BaseVertex + 3);

    Batches.Last().IndexCount += 6;
}

void FUIDrawData::BuildRoundedBoxOutline(const FRectangle& Bounds, const FCornerRadii& Radius, TArray<Vector2>& OutPoints, float Inset)
{
    OutPoints.Clear();

    const float MinX = static_cast<float>(Bounds.Position.X) + Inset;
    const float MinY = static_cast<float>(Bounds.Position.Y) + Inset;
    const float MaxX = static_cast<float>(Bounds.GetRight()) - Inset;
    const float MaxY = static_cast<float>(Bounds.GetBottom()) - Inset;

    const float CornerRadius[4] = 
    {
        Math::Max(Radius.TopLeft - Inset, 0.0f),
        Math::Max(Radius.TopRight - Inset, 0.0f),
        Math::Max(Radius.BottomRight - Inset, 0.0f),
        Math::Max(Radius.BottomLeft - Inset, 0.0f)
    };

    const Vector2 CornerPoint[4] =
    {
        Vector2(MinX, MinY),
        Vector2(MaxX, MinY),
        Vector2(MaxX, MaxY),
        Vector2(MinX, MaxY),
    };

    const Vector2 ArcCenterOffset[4] =
    {
        Vector2(1.0f, 1.0f),
        Vector2(-1.0f, 1.0f),
        Vector2(-1.0f, -1.0f),
        Vector2(1.0f, -1.0f),
    };

    for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        const float ThisRadius = CornerRadius[CornerIndex];

        if (ThisRadius <= 0.0f)
        {
            OutPoints.Add(CornerPoint[CornerIndex]);
            continue;
        }

        const Vector2 ArcCenter(
            CornerPoint[CornerIndex].X + (ArcCenterOffset[CornerIndex].X * ThisRadius),
            CornerPoint[CornerIndex].Y + (ArcCenterOffset[CornerIndex].Y * ThisRadius));

        const int32 SegmentsPerCorner = Math::Clamp(static_cast<int32>(ThisRadius * 1.5f), MinCornerSegments, MaxCornerSegments);
        const int32 PointsPerCorner   = SegmentsPerCorner + 1;

        const float StartAngle = Math::Constants::PI + (static_cast<float>(CornerIndex) * Math::Constants::HalfPI);
        const float AngleStep  = Math::Constants::HalfPI / static_cast<float>(SegmentsPerCorner);

        for (int32 PointIndex = 0; PointIndex < PointsPerCorner; ++PointIndex)
        {
            const float Angle = StartAngle + (AngleStep * static_cast<float>(PointIndex));
            OutPoints.Add(Vector2(ArcCenter.X + (ThisRadius * Math::Cos(Angle)), ArcCenter.Y + (ThisRadius * Math::Sin(Angle))));
        }
    }
}

void FUIDrawData::AddRoundedBox(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor,
    const Vector2& MinTexCoord, const Vector2& MaxTexCoord)
{
    BuildRoundedBoxOutline(Bounds, Radius, ScratchPoints);

    const int32 OutlineCount = ScratchPoints.Size();
    if (OutlineCount < 3)
    {
        return;
    }

    const int32 RingCount = bAntiAliasingEnabled ? 2 : 1;
    if (Vertices.Size() + (OutlineCount * RingCount) + 1 > MaxVertexCount)
    {
        return;
    }

    const float MinX = static_cast<float>(Bounds.Position.X);
    const float MinY = static_cast<float>(Bounds.Position.Y);
    const float MaxX = static_cast<float>(Bounds.GetRight());
    const float MaxY = static_cast<float>(Bounds.GetBottom());

    const uint32 CenterVertex = static_cast<uint32>(Vertices.Size());

    FUIVertex& Center = Vertices.Emplace();
    Center.Position   = Vector2((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
    Center.TexCoord   = (MinTexCoord + MaxTexCoord) * 0.5f;
    Center.Color      = PackedColor;

    if (!bAntiAliasingEnabled)
    {
        for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
        {
            EmplaceFillVertex(ScratchPoints[OutlineIndex], Bounds, PackedColor, MinTexCoord, MaxTexCoord);
        }

        for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
        {
            Indices.Add(CenterVertex);
            Indices.Add(CenterVertex + 1 + static_cast<uint32>(OutlineIndex));
            Indices.Add(CenterVertex + 1 + static_cast<uint32>((OutlineIndex + 1) % OutlineCount));
        }

        Batches.Last().IndexCount += OutlineCount * 3;
        return;
    }

    const uint32 ClearColor = PackedColor & PACKED_COLOR_MASK;
    const float  HalfFringe = FringeWidth * 0.5f;
    const float  Winding    = ComputeWindingSign(ScratchPoints);

    BuildMiterOffsets(ScratchPoints, true, ScratchOffsets);

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        const Vector2& Position = ScratchPoints[OutlineIndex];
        const Vector2  Outward  = ScratchOffsets[OutlineIndex] * (Winding * HalfFringe);

        EmplaceFillVertex(Position - Outward, Bounds, PackedColor, MinTexCoord, MaxTexCoord);
        EmplaceFillVertex(Position + Outward, Bounds, ClearColor, MinTexCoord, MaxTexCoord);
    }

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        const uint32 Inner     = CenterVertex + 1 + static_cast<uint32>(OutlineIndex * 2);
        const uint32 Outer     = Inner + 1;
        const uint32 NextInner = CenterVertex + 1 + static_cast<uint32>(((OutlineIndex + 1) % OutlineCount) * 2);
        const uint32 NextOuter = NextInner + 1;

        Indices.Add(CenterVertex);
        Indices.Add(Inner);
        Indices.Add(NextInner);

        Indices.Add(Inner);
        Indices.Add(Outer);
        Indices.Add(NextOuter);
        Indices.Add(Inner);
        Indices.Add(NextOuter);
        Indices.Add(NextInner);
    }

    Batches.Last().IndexCount += OutlineCount * 9;
}

FUIDrawBatch& FUIDrawData::GetOrOpenBatch(const FUITextureHandle& Texture)
{
    const bool       bIsClipped       = bHasActiveClip;
    const FRectangle ScissorRectangle = bIsClipped ? ActiveClipRectangle : FRectangle();

    if (!Batches.IsEmpty())
    {
        FUIDrawBatch& OpenBatch = Batches.Last();
        if (OpenBatch.Texture == Texture && OpenBatch.bIsClipped == bIsClipped && OpenBatch.ScissorRectangle == ScissorRectangle)
        {
            return OpenBatch;
        }

        if (OpenBatch.IndexCount == 0)
        {
            OpenBatch.Texture          = Texture;
            OpenBatch.ScissorRectangle = ScissorRectangle;
            OpenBatch.bIsClipped       = bIsClipped;
            return OpenBatch;
        }
    }

    FUIDrawBatch& NewBatch = Batches.Emplace();
    NewBatch.ScissorRectangle = ScissorRectangle;
    NewBatch.Texture          = Texture;
    NewBatch.IndexOffset      = Indices.Size();
    NewBatch.IndexCount       = 0;
    NewBatch.bIsClipped       = bIsClipped;

    return NewBatch;
}
