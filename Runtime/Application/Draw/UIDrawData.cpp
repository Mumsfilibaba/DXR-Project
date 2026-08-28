#include "Application/Draw/UIDrawData.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"
#include "Core/Math/Math.h"

// Below this a direction is treated as degenerate, which is what a repeated point in a polyline produces
static constexpr float GDirectionEpsilon = 1.0e-4f;

FUIDrawData::FUIDrawData()
    : Vertices()
    , Indices()
    , Batches()
    , ClipStack()
    , ScratchPoints()
{
}

FUIDrawData::~FUIDrawData() = default;

void FUIDrawData::Reset()
{
    Vertices.Clear();
    Indices.Clear();
    Batches.Clear();
    ClipStack.Clear();
    ScratchPoints.Clear();
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
            {
                ClipStack.Add(Command.Bounds);
                break;
            }

            case EDrawCommandType::ClipPop:
            {
                if (!ClipStack.IsEmpty())
                {
                    ClipStack.RemoveAt(ClipStack.LastIndex());
                }

                break;
            }
        }
    }
}

bool FUIDrawData::IsCulledByClip(const FRectangle& Bounds) const
{
    if (ClipStack.IsEmpty())
    {
        return false;
    }

    // Bounds with no extent are left to the geometry itself, which either draws nothing or, in the case
    // of text, still draws the glyphs the caller measured for a rectangle it never widened.
    if (Bounds.IsEmpty())
    {
        return false;
    }

    return ClipStack.Last().Intersect(Bounds).IsEmpty();
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

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);

    const float      HalfThickness = Command.Thickness * 0.5f;
    const int32      Inset         = static_cast<int32>(HalfThickness);
    const FRectangle PathBounds    = Command.Bounds.Deflate(FMargin(Inset, Inset, Inset, Inset));

    if (PathBounds.IsEmpty())
    {
        AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.Tint.ToColor().ToPackedRGBA());
        return;
    }

    BuildRoundedBoxOutline(PathBounds, Radius.ClampToBounds(PathBounds), ScratchPoints);
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

    GetOrOpenBatch(FUITextureHandle(Atlas));

    const float  AtlasWidth  = static_cast<float>(Atlas->GetWidth());
    const float  AtlasHeight = static_cast<float>(Atlas->GetHeight());
    const uint32 PackedColor = Command.Tint.ToColor().ToPackedRGBA();

    int32       PenX      = Command.Bounds.Position.X;
    const int32 BaselineY = Command.Bounds.Position.Y + Command.Font->GetTextBandOffset(Command.Bounds.Height) + Command.Font->GetAscent();

    for (int32 Index = 0; Index < Command.Text.Length(); ++Index)
    {
        const FGlyph& Glyph = Atlas->GetGlyph(Command.Text[Index]);
        if (!Glyph.AtlasRectangle.IsEmpty())
        {
            const FRectangle GlyphBounds(IntVector2(PenX + Glyph.BearingX, BaselineY + Glyph.BearingY),
                Glyph.AtlasRectangle.Width, Glyph.AtlasRectangle.Height);

            const Vector2 MinTexCoord(
                static_cast<float>(Glyph.AtlasRectangle.Position.X) / AtlasWidth,
                static_cast<float>(Glyph.AtlasRectangle.Position.Y) / AtlasHeight);

            const Vector2 MaxTexCoord(
                static_cast<float>(Glyph.AtlasRectangle.GetRight()) / AtlasWidth,
                static_cast<float>(Glyph.AtlasRectangle.GetBottom()) / AtlasHeight);

            AddQuad(GlyphBounds, MinTexCoord, MaxTexCoord, PackedColor);
        }

        PenX += Glyph.Advance;
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

void FUIDrawData::AddPolyline(TArrayView<const Vector2> Points, float Thickness, bool bClosed, uint32 PackedColor)
{
    const int32 PointCount = Points.Size();
    if (PointCount < 2 || Thickness <= 0.0f)
    {
        return;
    }

    if (Vertices.Size() + (PointCount * 2) > MaxVertexCount)
    {
        return;
    }

    const float  HalfThickness = Thickness * 0.5f;
    const int32  SegmentCount  = bClosed ? PointCount : PointCount - 1;
    const uint32 BaseVertex    = static_cast<uint32>(Vertices.Size());

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
        float   Scale = HalfThickness;

        const float MiterLength = Miter.GetLength();
        if (MiterLength > GDirectionEpsilon)
        {
            Miter = Miter * (1.0f / MiterLength);

            const float CosHalfAngle = Miter.DotProduct(IncomingNormal);
            if (CosHalfAngle > GDirectionEpsilon)
            {
                Scale = Math::Min(HalfThickness / CosHalfAngle, HalfThickness * MiterLimit);
            }
        }
        else
        {
            Miter = IncomingNormal;
        }

        const Vector2 Offset = Miter * Scale;

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
}

void FUIDrawData::AddConvexPolygon(TArrayView<const Vector2> Points, uint32 PackedColor)
{
    const int32 PointCount = Points.Size();
    if (PointCount < 3)
    {
        return;
    }

    if (Vertices.Size() + PointCount > MaxVertexCount)
    {
        return;
    }

    const uint32 BaseVertex = static_cast<uint32>(Vertices.Size());

    for (int32 Index = 0; Index < PointCount; ++Index)
    {
        FUIVertex& Vertex = Vertices.Emplace();
        Vertex.Position   = Points[Index];
        Vertex.TexCoord   = Vector2(0.5f, 0.5f);
        Vertex.Color      = PackedColor;
    }

    for (int32 Index = 2; Index < PointCount; ++Index)
    {
        Indices.Add(BaseVertex);
        Indices.Add(BaseVertex + static_cast<uint32>(Index - 1));
        Indices.Add(BaseVertex + static_cast<uint32>(Index));
    }

    Batches.Last().IndexCount += (PointCount - 2) * 3;
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

void FUIDrawData::BuildRoundedBoxOutline(const FRectangle& Bounds, const FCornerRadii& Radius, TArray<Vector2>& OutPoints)
{
    OutPoints.Clear();

    const float MinX = static_cast<float>(Bounds.Position.X);
    const float MinY = static_cast<float>(Bounds.Position.Y);
    const float MaxX = static_cast<float>(Bounds.GetRight());
    const float MaxY = static_cast<float>(Bounds.GetBottom());

    const float CornerRadius[4] = 
    {
        Radius.TopLeft,
        Radius.TopRight,
        Radius.BottomRight,
        Radius.BottomLeft
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

void FUIDrawData::AddRoundedBox(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor)
{
    BuildRoundedBoxOutline(Bounds, Radius, ScratchPoints);

    const int32 OutlineCount = ScratchPoints.Size();
    if (OutlineCount < 3)
    {
        return;
    }

    if (Vertices.Size() + OutlineCount + 1 > MaxVertexCount)
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
    Center.TexCoord   = Vector2(0.5f, 0.5f);
    Center.Color      = PackedColor;

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        const Vector2& Position = ScratchPoints[OutlineIndex];

        FUIVertex& Vertex = Vertices.Emplace();
        Vertex.Position   = Position;
        Vertex.TexCoord   = Vector2((Position.X - MinX) / (MaxX - MinX), (Position.Y - MinY) / (MaxY - MinY));
        Vertex.Color      = PackedColor;
    }

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        Indices.Add(CenterVertex);
        Indices.Add(CenterVertex + 1 + static_cast<uint32>(OutlineIndex));
        Indices.Add(CenterVertex + 1 + static_cast<uint32>((OutlineIndex + 1) % OutlineCount));
    }

    Batches.Last().IndexCount += OutlineCount * 3;
}

FUIDrawBatch& FUIDrawData::GetOrOpenBatch(const FUITextureHandle& Texture)
{
    const bool       bIsClipped       = !ClipStack.IsEmpty();
    const FRectangle ScissorRectangle = bIsClipped ? ClipStack.Last() : FRectangle();

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
