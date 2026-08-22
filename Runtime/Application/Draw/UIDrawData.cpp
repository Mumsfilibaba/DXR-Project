#include "Application/Draw/UIDrawData.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"
#include "Core/Math/Math.h"

FUIDrawData::FUIDrawData()
    : Vertices()
    , Indices()
    , Batches()
    , ClipStack()
{
}

FUIDrawData::~FUIDrawData() = default;

void FUIDrawData::Reset()
{
    Vertices.Clear();
    Indices.Clear();
    Batches.Clear();
    ClipStack.Clear();
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
                // A line is handed a full rectangle by FDrawCommandList, so it needs no special case
                if (!IsCulledByClip(Command.Bounds))
                {
                    AddBox(Command);
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

            case EDrawCommandType::ClipPush:
            {
                // The list already intersected the region down the stack when it recorded the command
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

void FUIDrawData::AddBox(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty())
    {
        return;
    }

    GetOrOpenBatch(nullptr);

    const float ShortestSide = static_cast<float>(Math::Min(Command.Bounds.Width, Command.Bounds.Height));
    const float Radius       = Math::Min(Command.CornerRadius, ShortestSide * 0.5f);

    if (Radius > 0.0f)
    {
        AddRoundedBox(Command.Bounds, Radius, Command.Tint.ToColor().ToPackedRGBA());
        return;
    }

    AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.Tint.ToColor().ToPackedRGBA());
}

void FUIDrawData::AddText(const FDrawCommand& Command)
{
    if (!Command.Font || Command.Text.IsEmpty())
    {
        return;
    }

    // A face that only answers metrics lays out but draws nothing, which is what the headless tests use
    const FFontAtlas* Atlas = Command.Font->GetAtlas();
    if (!Atlas || !Atlas->IsValid())
    {
        return;
    }

    GetOrOpenBatch(Atlas);

    const float  AtlasWidth  = static_cast<float>(Atlas->GetWidth());
    const float  AtlasHeight = static_cast<float>(Atlas->GetHeight());
    const uint32 PackedColor = Command.Tint.ToColor().ToPackedRGBA();

    // The band the glyphs occupy is centred in the bounds rather than pinned to the top of them
    int32       PenX      = Command.Bounds.Position.X;
    const int32 BaselineY = Command.Bounds.Position.Y + Command.Font->GetTextBandOffset(Command.Bounds.Height) + Command.Font->GetAscent();

    for (int32 Index = 0; Index < Command.Text.Length(); ++Index)
    {
        const FGlyph& Glyph = Atlas->GetGlyph(Command.Text[Index]);

        // Whitespace packs to an empty region, so it advances the pen without emitting geometry
        if (!Glyph.AtlasRectangle.IsEmpty())
        {
            const FRectangle GlyphBounds(
                IntVector2(PenX + Glyph.BearingX, BaselineY + Glyph.BearingY),
                Glyph.AtlasRectangle.Width,
                Glyph.AtlasRectangle.Height);

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

void FUIDrawData::AddQuad(const FRectangle& Bounds, const Vector2& MinTexCoord, const Vector2& MaxTexCoord, uint32 PackedColor)
{
    // The indices are 16-bit, so the geometry is truncated rather than allowed to wrap around
    if (Vertices.Size() + 4 > MaxVertexCount)
    {
        return;
    }

    const uint16 BaseVertex = static_cast<uint16>(Vertices.Size());

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

    Indices.Add(static_cast<uint16>(BaseVertex + 0));
    Indices.Add(static_cast<uint16>(BaseVertex + 1));
    Indices.Add(static_cast<uint16>(BaseVertex + 2));
    Indices.Add(static_cast<uint16>(BaseVertex + 0));
    Indices.Add(static_cast<uint16>(BaseVertex + 2));
    Indices.Add(static_cast<uint16>(BaseVertex + 3));

    Batches.Last().IndexCount += 6;
}

void FUIDrawData::AddRoundedBox(const FRectangle& Bounds, float Radius, uint32 PackedColor)
{
    const int32 SegmentsPerCorner = Math::Clamp(static_cast<int32>(Radius * 1.5f), MinCornerSegments, MaxCornerSegments);
    const int32 PointsPerCorner   = SegmentsPerCorner + 1;
    const int32 OutlineCount      = 4 * PointsPerCorner;

    if (Vertices.Size() + OutlineCount + 1 > MaxVertexCount)
    {
        return;
    }

    const float MinX = static_cast<float>(Bounds.Position.X);
    const float MinY = static_cast<float>(Bounds.Position.Y);
    const float MaxX = static_cast<float>(Bounds.GetRight());
    const float MaxY = static_cast<float>(Bounds.GetBottom());

    const Vector2 ArcCenters[4] =
    {
        Vector2(MinX + Radius, MinY + Radius),
        Vector2(MaxX - Radius, MinY + Radius),
        Vector2(MaxX - Radius, MaxY - Radius),
        Vector2(MinX + Radius, MaxY - Radius),
    };

    const uint16 CenterVertex = static_cast<uint16>(Vertices.Size());

    FUIVertex& Center = Vertices.Emplace();
    Center.Position   = Vector2((MinX + MaxX) * 0.5f, (MinY + MaxY) * 0.5f);
    Center.TexCoord   = Vector2(0.5f, 0.5f);
    Center.Color      = PackedColor;

    for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
    {
        const float StartAngle = Math::Constants::PI + (static_cast<float>(CornerIndex) * Math::Constants::HalfPI);
        const float AngleStep  = Math::Constants::HalfPI / static_cast<float>(SegmentsPerCorner);

        for (int32 PointIndex = 0; PointIndex < PointsPerCorner; ++PointIndex)
        {
            const float Angle = StartAngle + (AngleStep * static_cast<float>(PointIndex));

            const Vector2 Position(
                ArcCenters[CornerIndex].X + (Radius * Math::Cos(Angle)),
                ArcCenters[CornerIndex].Y + (Radius * Math::Sin(Angle)));

            FUIVertex& Vertex = Vertices.Emplace();
            Vertex.Position   = Position;
            Vertex.TexCoord   = Vector2((Position.X - MinX) / (MaxX - MinX), (Position.Y - MinY) / (MaxY - MinY));
            Vertex.Color      = PackedColor;
        }
    }

    for (int32 OutlineIndex = 0; OutlineIndex < OutlineCount; ++OutlineIndex)
    {
        const uint16 CurrentVertex = static_cast<uint16>(CenterVertex + 1 + OutlineIndex);
        const uint16 NextVertex    = static_cast<uint16>(CenterVertex + 1 + ((OutlineIndex + 1) % OutlineCount));

        Indices.Add(CenterVertex);
        Indices.Add(CurrentVertex);
        Indices.Add(NextVertex);
    }

    Batches.Last().IndexCount += OutlineCount * 3;
}

FUIDrawBatch& FUIDrawData::GetOrOpenBatch(const FFontAtlas* Atlas)
{
    const bool       bIsClipped       = !ClipStack.IsEmpty();
    const FRectangle ScissorRectangle = bIsClipped ? ClipStack.Last() : FRectangle();

    if (!Batches.IsEmpty())
    {
        FUIDrawBatch& OpenBatch = Batches.Last();
        if (OpenBatch.Atlas == Atlas && OpenBatch.bIsClipped == bIsClipped && OpenBatch.ScissorRectangle == ScissorRectangle)
        {
            return OpenBatch;
        }

        // An empty batch was opened but never filled, so it is reused rather than left behind
        if (OpenBatch.IndexCount == 0)
        {
            OpenBatch.Atlas            = Atlas;
            OpenBatch.ScissorRectangle = ScissorRectangle;
            OpenBatch.bIsClipped       = bIsClipped;
            return OpenBatch;
        }
    }

    FUIDrawBatch& NewBatch = Batches.Emplace();
    NewBatch.ScissorRectangle = ScissorRectangle;
    NewBatch.Atlas            = Atlas;
    NewBatch.IndexOffset      = Indices.Size();
    NewBatch.IndexCount       = 0;
    NewBatch.bIsClipped       = bIsClipped;
    return NewBatch;
}
