#include "Application/Draw/UIDrawData.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Text/FontAtlas.h"
#include "Application/Text/IFontFace.h"
#include "Core/Algorithms/Algorithm.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/FrameProfiler.h"

constexpr float DIRECTION_EPSILON = 1.0e-4f;

constexpr uint32 PACKED_ALPHA_MASK = 0xff000000u;
constexpr uint32 PACKED_COLOR_MASK = 0x00ffffffu;

template<typename ElementType>
static FORCEINLINE void ReserveForAppend(TArray<ElementType>& Array, int32 AppendCount)
{
    const int32 RequiredCapacity = Array.Size() + AppendCount;
    if (RequiredCapacity > Array.Capacity())
    {
        Array.Reserve(Math::Max(RequiredCapacity, Array.Capacity() * 2));
    }
}

static uint64 HashBytes(const void* Data, int32 ByteCount)
{
    const uint8* Bytes = static_cast<const uint8*>(Data);

    constexpr uint64 Prime1 = 11400714785074694791ull;
    constexpr uint64 Prime2 = 14029467366897019727ull;
    constexpr uint64 Prime3 = 1609587929392839161ull;

    const auto RotateLeft = [](uint64 Value, uint32 Bits)
    {
        return (Value << Bits) | (Value >> (64u - Bits));
    };

    uint64 Lane0  = Prime1;
    uint64 Lane1  = Prime2;
    uint64 Lane2  = Prime3;
    uint64 Lane3  = Prime1 ^ Prime2;
    int32  Offset = 0;

    for (; Offset + 32 <= ByteCount; Offset += 32)
    {
        uint64 Words[4];
        Memory::Memcpy(Words, Bytes + Offset, sizeof(Words));

        Lane0 = (Lane0 ^ Words[0]) * Prime1;
        Lane1 = (Lane1 ^ Words[1]) * Prime2;
        Lane2 = (Lane2 ^ Words[2]) * Prime3;
        Lane3 = (Lane3 ^ Words[3]) * Prime1;
    }

    uint64 Hash = Lane0 ^ RotateLeft(Lane1, 13) ^ RotateLeft(Lane2, 29) ^ RotateLeft(Lane3, 47);
    for (; Offset + 8 <= ByteCount; Offset += 8)
    {
        uint64 Word;
        Memory::Memcpy(&Word, Bytes + Offset, sizeof(Word));
        Hash = RotateLeft(Hash ^ Word, 27) * Prime1 + Prime3;
    }

    for (; Offset < ByteCount; ++Offset)
    {
        Hash = RotateLeft(Hash ^ Bytes[Offset], 11) * Prime2;
    }

    Hash ^= static_cast<uint64>(ByteCount);
    Hash ^= Hash >> 33;
    Hash *= 0xff51afd7ed558ccdull;
    Hash ^= Hash >> 33;
    Hash *= 0xc4ceb9fe1a85ec53ull;
    return Hash ^ (Hash >> 33);
}

static uint32 PackColorWithAlphaScale(uint32 PackedColor, float Scale)
{
    const float  Alpha  = static_cast<float>((PackedColor & PACKED_ALPHA_MASK) >> 24);
    const uint32 Scaled = static_cast<uint32>(Math::Clamp(Math::RoundToInt(Alpha * Scale), 0, 255));

    return (PackedColor & PACKED_COLOR_MASK) | (Scaled << 24);
}

FUIDrawData::FUIDrawData()
    : Vertices()
    , Indices()
    , ShapeInstances()
    , ShapeVertices()
    , ShapeIndices()
    , bShapeCompatibilityDirty(false)
    , TextGlyphInstances()
    , Batches()
    , ScratchPoints()
    , ScratchOffsets()
    , SortedCommandIndices()
    , SortScratchIndices()
    , TextGeometryCache()
    , TextCommandOrdinal(0)
    , SourceCommandList(nullptr)
    , ActiveClipId(0)
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
    ShapeVertices.Clear();
    ShapeIndices.Clear();
    ShapeInstances.Clear();
    TextGlyphInstances.Clear();
    Batches.Clear();
    ScratchPoints.Clear();
    ScratchOffsets.Clear();
    
    bShapeCompatibilityDirty = false;
    TextCommandOrdinal       = 0;
    SourceCommandList        = nullptr;
    ActiveClipId             = 0;
    ActiveClipRectangle      = FRectangle();
    bHasActiveClip           = false;
}

const TArray<FUIShapeVertex>& FUIDrawData::GetShapeVertices() const
{
    BuildShapeCompatibilityGeometry();
    return ShapeVertices;
}

const TArray<uint32>& FUIDrawData::GetShapeIndices() const
{
    BuildShapeCompatibilityGeometry();
    return ShapeIndices;
}

void FUIDrawData::BuildShapeCompatibilityGeometry() const
{
    if (!bShapeCompatibilityDirty)
    {
        return;
    }

    ShapeVertices.ResizeUninitialized(ShapeInstances.Size() * 4);
    ShapeIndices.ResizeUninitialized(ShapeInstances.Size() * 6);

    static const Vector2 Corners[4] =
    {
        Vector2(0.0f, 0.0f),
        Vector2(1.0f, 0.0f),
        Vector2(1.0f, 1.0f),
        Vector2(0.0f, 1.0f),
    };

    for (int32 InstanceIndex = 0; InstanceIndex < ShapeInstances.Size(); ++InstanceIndex)
    {
        const FUIShapeInstance& Instance = ShapeInstances[InstanceIndex];
        
        FUIShapeVertex* Quad = ShapeVertices.Data() + (InstanceIndex * 4);
        for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
        {
            Quad[CornerIndex].Position  = Instance.Position + (Instance.DrawSize * Corners[CornerIndex]);
            Quad[CornerIndex].Color     = Instance.Color;
            Quad[CornerIndex].LocalPos  = Instance.LocalOrigin + (Instance.DrawSize * Corners[CornerIndex]);
            Quad[CornerIndex].RectSize  = Instance.RectSize;
            Quad[CornerIndex].RadiusTL  = Instance.RadiusTL;
            Quad[CornerIndex].RadiusTR  = Instance.RadiusTR;
            Quad[CornerIndex].RadiusBR  = Instance.RadiusBR;
            Quad[CornerIndex].RadiusBL  = Instance.RadiusBL;
            Quad[CornerIndex].Thickness = Instance.Thickness;
            Quad[CornerIndex].ShapeKind = Instance.ShapeKind;
        }

        const uint32 BaseVertex = static_cast<uint32>(InstanceIndex * 4);

        uint32* IndicesOut = ShapeIndices.Data() + (InstanceIndex * 6);
        IndicesOut[0] = BaseVertex + 0;
        IndicesOut[1] = BaseVertex + 1;
        IndicesOut[2] = BaseVertex + 2;
        IndicesOut[3] = BaseVertex + 0;
        IndicesOut[4] = BaseVertex + 2;
        IndicesOut[5] = BaseVertex + 3;
    }

    bShapeCompatibilityDirty = false;
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

    SourceCommandList = &CommandList;

    ReserveForAppend(Vertices, Commands.Size() * 4);
    ReserveForAppend(Indices, Commands.Size() * 6);
    ReserveForAppend(ShapeInstances, Commands.Size());
    ReserveForAppend(TextGlyphInstances, Commands.Size() * 4);

    {
        SortedCommandIndices.ResizeUninitialized(Commands.Size());
        for (int32 Index = 0; Index < Commands.Size(); ++Index)
        {
            SortedCommandIndices[Index] = Index;
        }

        if (!Algorithm::IsSorted(Commands, [](const FDrawCommand& Left, const FDrawCommand& Right)
        {
            return Left.LayerId < Right.LayerId;
        }))
        {
            Algorithm::IntegerSortBy(SortedCommandIndices, [&](int32 CommandIndex)
            {
                return Commands[CommandIndex].LayerId;
            }, SortScratchIndices);
        }
    }

    {
        for (int32 SortedIndex = 0; SortedIndex < SortedCommandIndices.Size(); ++SortedIndex)
        {
            const FDrawCommand& Command = Commands[SortedCommandIndices[SortedIndex]];

            ApplyCommandClip(Command, CommandList);

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
                        AddPolyline(Points, Command.Thickness, Command.IsClosed(), Command.PackedColor);
                    }

                    break;
                }

                case EDrawCommandType::ConvexPolygon:
                {
                    const TArrayView<const Vector2> Points = CommandList.GetCommandPoints(Command);
                    if (!IsCulledByClip(ComputePointBounds(Points, 0.0f)))
                    {
                        GetOrOpenBatch(FUITextureHandle());
                        AddConvexPolygon(Points, Command.PackedColor);
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

    SourceCommandList = nullptr;
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

void FUIDrawData::ApplyCommandClip(const FDrawCommand& Command, const FDrawCommandList& CommandList)
{
    if (Command.ClipId == ActiveClipId && Command.IsClipped() == bHasActiveClip)
    {
        return;
    }

    ActiveClipId        = Command.ClipId;
    bHasActiveClip      = Command.IsClipped();
    ActiveClipRectangle = bHasActiveClip ? CommandList.GetCommandClipRectangle(Command) : FRectangle();
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

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);
    if (!Radius.IsZero())
    {
        AddSdfRoundedQuad(Command.Bounds, Radius, Command.PackedColor, 0.0f, ShapeKindFill);
        return;
    }

    GetOrOpenBatch(FUITextureHandle());
    AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.PackedColor);
}

void FUIDrawData::AddBoxOutline(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty() || Command.Thickness <= 0.0f)
    {
        return;
    }

    if (Command.Thickness >= static_cast<float>(Math::Min(Command.Bounds.Width, Command.Bounds.Height)))
    {
        GetOrOpenBatch(FUITextureHandle());
        AddQuad(Command.Bounds, Vector2(0.0f, 0.0f), Vector2(1.0f, 1.0f), Command.PackedColor);
        return;
    }

    const FCornerRadii Radius = Command.CornerRadius.ClampToBounds(Command.Bounds);
    AddSdfRoundedQuad(Command.Bounds, Radius, Command.PackedColor, Command.Thickness, ShapeKindStroke);
}

void FUIDrawData::AddText(const FDrawCommand& Command)
{
    if (!Command.Font || !SourceCommandList)
    {
        return;
    }

    const StringView Text = SourceCommandList->GetCommandText(Command);
    if (Text.IsEmpty())
    {
        return;
    }

    const FFontAtlas* Atlas = Command.Font->GetAtlas();
    if (!Atlas || !Atlas->IsValid())
    {
        return;
    }

    const uint64 AtlasRevision = Atlas->GetRevision();
    const int32 CacheIndex = TextCommandOrdinal++;
    if (CacheIndex >= TextGeometryCache.Size())
    {
        TextGeometryCache.Emplace();
    }

    FTextGeometryCacheEntry& Cached = TextGeometryCache[CacheIndex];
    if (Cached.bValid && Cached.Font == Command.Font && Cached.Bounds == Command.Bounds
        && Cached.AtlasRevision == AtlasRevision && Cached.PackedColor == Command.PackedColor
        && Cached.Text.Equals(Text.Data(), Text.Length()))
    {
        GetOrOpenBatch(FUITextureHandle(Atlas), EUIDrawBatchKind::Text);
        
        const int32 InstanceStart = TextGlyphInstances.Size();
        ReserveForAppend(TextGlyphInstances, Cached.Instances.Size());
        
        TextGlyphInstances.ResizeUninitialized(InstanceStart + Cached.Instances.Size());
        Memory::Memcpy(TextGlyphInstances.Data() + InstanceStart, Cached.Instances.Data(),
            Cached.Instances.Size() * static_cast<int32>(sizeof(FUITextGlyphInstance)));

        Batches.Last().IndexCount += Cached.Instances.Size();
        return;
    }

    const FShapedRun& ShapedRun = Command.Font->ShapeText(Text);
    GetOrOpenBatch(FUITextureHandle(Atlas), EUIDrawBatchKind::Text);

    const int32  InstanceStart = TextGlyphInstances.Size();
    const float  AtlasWidth    = static_cast<float>(Atlas->GetWidth());
    const float  AtlasHeight   = static_cast<float>(Atlas->GetHeight());
    const uint32 PackedColor   = Command.PackedColor;

    const int32 PenX      = Command.Bounds.Position.X;
    const int32 BaselineY = Command.Bounds.Position.Y + Command.Font->GetTextBandOffset(Command.Bounds.Height) + Command.Font->GetAscent();

    const int32 MaxInstanceCount = ShapedRun.Glyphs.Size();
    ReserveForAppend(TextGlyphInstances, MaxInstanceCount);

    TextGlyphInstances.ResizeUninitialized(InstanceStart + MaxInstanceCount);
    FUITextGlyphInstance* InstanceOutput = TextGlyphInstances.Data() + InstanceStart;
    
    int32 InstanceCount = 0;
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

        FUITextGlyphInstance& Instance = InstanceOutput[InstanceCount++];
        Instance.Position    = Vector2(static_cast<float>(GlyphBounds.Position.X), static_cast<float>(GlyphBounds.Position.Y));
        Instance.Size        = Vector2(static_cast<float>(GlyphBounds.Width), static_cast<float>(GlyphBounds.Height));
        Instance.MinTexCoord = MinTexCoord;
        Instance.MaxTexCoord = MaxTexCoord;
        Instance.Color       = PackedColor;
    }

    TextGlyphInstances.ResizeUninitialized(InstanceStart + InstanceCount);
    Batches.Last().IndexCount += InstanceCount;

    if (InstanceCount <= 0)
    {
        return;
    }

    if (Atlas->GetRevision() != AtlasRevision)
    {
        Cached.bValid = false;
        return;
    }

    Cached.AtlasRevision = Atlas->GetRevision();
    Cached.Text          = String(Text.Data(), Text.Length());
    Cached.Font          = Command.Font;
    Cached.Bounds        = Command.Bounds;
    Cached.PackedColor   = Command.PackedColor;
    Cached.bValid        = true;

    ReserveForAppend(Cached.Instances, InstanceCount);
    Cached.Instances.ResizeUninitialized(InstanceCount);

    Memory::Memcpy(Cached.Instances.Data(), TextGlyphInstances.Data() + InstanceStart,
        InstanceCount * static_cast<int32>(sizeof(FUITextGlyphInstance)));
}

void FUIDrawData::AddImage(const FDrawCommand& Command)
{
    if (Command.Bounds.IsEmpty() || !SourceCommandList)
    {
        return;
    }

    const FUIBrush* BrushPtr = SourceCommandList->GetCommandBrush(Command);
    if (!BrushPtr)
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle(BrushPtr->Texture));

    const uint32   PackedColor = Command.PackedColor;
    const FUIBrush Brush       = *BrushPtr;

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

        const float  EdgeDistance  = Math::Min(X - LeftTip, RightTip - X);
        const float  AlphaScale    = (FadeWidth > 0.0f) ? Math::Clamp(EdgeDistance / FadeWidth, 0.0f, 1.0f) : 1.0f;
        const uint32 PackedColor   = PackColorWithAlphaScale(Command.PackedColor, AlphaScale);
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

            const uint32 PackedColor = PackColorWithAlphaScale(Command.PackedColor, TrailAlpha + ((1.0f - TrailAlpha) * Strength));
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
    ReserveForAppend(OutOffsets, PointCount);

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

    const float  HalfThickness     = Thickness * 0.5f;
    const int32  SegmentCount      = bClosed ? PointCount : PointCount - 1;
    const uint32 BaseVertex        = static_cast<uint32>(Vertices.Size());
    const int32  VertexCount       = PointCount * RowsPerPoint;
    const int32  IndicesPerSegment = bAntiAliasingEnabled ? 18 : 6;
    const int32  IndexCount        = SegmentCount * IndicesPerSegment;

    const int32 VertexStart = Vertices.Size();
    ReserveForAppend(Vertices, VertexCount);

    Vertices.ResizeUninitialized(VertexStart + VertexCount);
    FUIVertex* OutVertices = Vertices.Data() + VertexStart;

    const int32 IndexStart = Indices.Size();
    ReserveForAppend(Indices, IndexCount);

    Indices.ResizeUninitialized(IndexStart + IndexCount);
    uint32* OutIndices = Indices.Data() + IndexStart;

    BuildMiterOffsets(Points, bClosed, ScratchOffsets);

    if (!bAntiAliasingEnabled)
    {
        for (int32 Index = 0; Index < PointCount; ++Index)
        {
            const Vector2 Offset = ScratchOffsets[Index] * HalfThickness;

            FUIVertex& Outer = OutVertices[Index * 2];
            Outer.Position   = Points[Index] + Offset;
            Outer.TexCoord   = Vector2(0.5f, 0.5f);
            Outer.Color      = PackedColor;

            FUIVertex& Inner = OutVertices[(Index * 2) + 1];
            Inner.Position   = Points[Index] - Offset;
            Inner.TexCoord   = Vector2(0.5f, 0.5f);
            Inner.Color      = PackedColor;
        }

        int32 WriteIndex = 0;
        for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
        {
            const uint32 CurrentOuter = BaseVertex + static_cast<uint32>(Segment * 2);
            const uint32 CurrentInner = CurrentOuter + 1;
            const uint32 NextOuter    = BaseVertex + static_cast<uint32>(((Segment + 1) % PointCount) * 2);
            const uint32 NextInner    = NextOuter + 1;

            OutIndices[WriteIndex++] = CurrentOuter;
            OutIndices[WriteIndex++] = NextOuter;
            OutIndices[WriteIndex++] = NextInner;
            OutIndices[WriteIndex++] = CurrentOuter;
            OutIndices[WriteIndex++] = NextInner;
            OutIndices[WriteIndex++] = CurrentInner;
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
        const Vector2& Miter      = ScratchOffsets[Index];
        const Vector2  CoreOffset = Miter * CoreExtent;
        const Vector2  EdgeOffset = Miter * EdgeExtent;

        FUIVertex* Vertex = OutVertices + (Index * 4);
        Vertex[0].Position = Points[Index] + EdgeOffset;
        Vertex[1].Position = Points[Index] + CoreOffset;
        Vertex[2].Position = Points[Index] - CoreOffset;
        Vertex[3].Position = Points[Index] - EdgeOffset;

        for (int32 Row = 0; Row < 4; ++Row)
        {
            Vertex[Row].TexCoord = Vector2(0.5f, 0.5f);
            Vertex[Row].Color = (Row == 0 || Row == 3) ? ClearColor : CoreColor;
        }
    }

    int32 WriteIndex = 0;
    for (int32 Segment = 0; Segment < SegmentCount; ++Segment)
    {
        const uint32 Current = BaseVertex + static_cast<uint32>(Segment * 4);
        const uint32 Next    = BaseVertex + static_cast<uint32>(((Segment + 1) % PointCount) * 4);

        for (uint32 Row = 0; Row < 3; ++Row)
        {
            OutIndices[WriteIndex++] = Current + Row;
            OutIndices[WriteIndex++] = Next + Row;
            OutIndices[WriteIndex++] = Next + Row + 1;
            OutIndices[WriteIndex++] = Current + Row;
            OutIndices[WriteIndex++] = Next + Row + 1;
            OutIndices[WriteIndex++] = Current + Row + 1;
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
        const int32 VertexStart = Vertices.Size();
        ReserveForAppend(Vertices, PointCount);

        Vertices.ResizeUninitialized(VertexStart + PointCount);

        FUIVertex* Dest = Vertices.Data() + VertexStart;
        for (int32 Index = 0; Index < PointCount; ++Index)
        {
            Dest[Index].Position = Points[Index];
            Dest[Index].TexCoord = Vector2(0.5f, 0.5f);
            Dest[Index].Color    = PackedColor;
        }

        const int32 FaceCount  = PointCount - 2;
        const int32 IndexStart = Indices.Size();
        ReserveForAppend(Indices, FaceCount * 3);

        Indices.ResizeUninitialized(IndexStart + (FaceCount * 3));

        uint32* IndexDest = Indices.Data() + IndexStart;
        for (int32 Index = 2; Index < PointCount; ++Index)
        {
            *IndexDest++ = BaseVertex;
            *IndexDest++ = BaseVertex + static_cast<uint32>(Index - 1);
            *IndexDest++ = BaseVertex + static_cast<uint32>(Index);
        }

        Batches.Last().IndexCount += FaceCount * 3;
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
    const int32 VertexStart = Vertices.Size();
    ReserveForAppend(Vertices, 4);

    Vertices.ResizeUninitialized(VertexStart + 4);

    const float MinX = static_cast<float>(Bounds.Position.X);
    const float MinY = static_cast<float>(Bounds.Position.Y);
    const float MaxX = static_cast<float>(Bounds.GetRight());
    const float MaxY = static_cast<float>(Bounds.GetBottom());
    
    FUIVertex* Quad = Vertices.Data() + VertexStart;
    Quad[0].Position = Vector2(MinX, MinY);
    Quad[0].TexCoord = Vector2(MinTexCoord.X, MinTexCoord.Y);
    Quad[0].Color    = PackedColor;
    Quad[1].Position = Vector2(MaxX, MinY);
    Quad[1].TexCoord = Vector2(MaxTexCoord.X, MinTexCoord.Y);
    Quad[1].Color    = PackedColor;
    Quad[2].Position = Vector2(MaxX, MaxY);
    Quad[2].TexCoord = Vector2(MaxTexCoord.X, MaxTexCoord.Y);
    Quad[2].Color    = PackedColor;
    Quad[3].Position = Vector2(MinX, MaxY);
    Quad[3].TexCoord = Vector2(MinTexCoord.X, MaxTexCoord.Y);
    Quad[3].Color    = PackedColor;

    const int32 IndexStart = Indices.Size();
    ReserveForAppend(Indices, 6);

    Indices.ResizeUninitialized(IndexStart + 6);

    uint32* QuadIndices = Indices.Data() + IndexStart;
    QuadIndices[0] = BaseVertex + 0;
    QuadIndices[1] = BaseVertex + 1;
    QuadIndices[2] = BaseVertex + 2;
    QuadIndices[3] = BaseVertex + 0;
    QuadIndices[4] = BaseVertex + 2;
    QuadIndices[5] = BaseVertex + 3;

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

void FUIDrawData::AddSdfRoundedQuad(const FRectangle& Bounds, const FCornerRadii& Radius, uint32 PackedColor, float Thickness, float ShapeKind)
{
    const float Pad   = bAntiAliasingEnabled ? (FringeWidth * 0.5f) : 0.0f;
    const float MinX  = static_cast<float>(Bounds.Position.X);
    const float MinY  = static_cast<float>(Bounds.Position.Y);
    const float MaxX  = static_cast<float>(Bounds.GetRight());
    const float MaxY  = static_cast<float>(Bounds.GetBottom());
    const float SizeX = MaxX - MinX;
    const float SizeY = MaxY - MinY;

    if (ShapeInstances.Size() >= (MaxVertexCount / 4))
    {
        return;
    }

    GetOrOpenBatch(FUITextureHandle(), EUIDrawBatchKind::Shape);

    FUIShapeInstance& Instance = ShapeInstances.Emplace();
    Instance.Position    = Vector2(MinX - Pad, MinY - Pad);
    Instance.Color       = PackedColor;
    Instance.DrawSize    = Vector2(SizeX + (Pad * 2.0f), SizeY + (Pad * 2.0f));
    Instance.LocalOrigin = Vector2(-Pad, -Pad);
    Instance.RectSize    = Vector2(SizeX, SizeY);
    Instance.RadiusTL    = Radius.TopLeft;
    Instance.RadiusTR    = Radius.TopRight;
    Instance.RadiusBR    = Radius.BottomRight;
    Instance.RadiusBL    = Radius.BottomLeft;
    Instance.Thickness   = Thickness;
    Instance.ShapeKind   = ShapeKind;

    bShapeCompatibilityDirty = true;
    Batches.Last().IndexCount += 6;
}

FUIDrawBatch& FUIDrawData::GetOrOpenBatch(const FUITextureHandle& Texture, EUIDrawBatchKind Kind)
{
    const bool       bIsClipped       = bHasActiveClip;
    const FRectangle ScissorRectangle = bIsClipped ? ActiveClipRectangle : FRectangle();
    const int32 StreamIndexOffset = (Kind == EUIDrawBatchKind::Shape)
        ? ShapeInstances.Size() * 6
        : ((Kind == EUIDrawBatchKind::Text) ? TextGlyphInstances.Size() : Indices.Size());

    if (!Batches.IsEmpty())
    {
        FUIDrawBatch& OpenBatch = Batches.Last();
        if (OpenBatch.Kind == Kind && OpenBatch.Texture == Texture && OpenBatch.bIsClipped == bIsClipped
            && OpenBatch.ScissorRectangle == ScissorRectangle)
        {
            return OpenBatch;
        }

        if (OpenBatch.IndexCount == 0)
        {
            OpenBatch.Texture          = Texture;
            OpenBatch.ScissorRectangle = ScissorRectangle;
            OpenBatch.bIsClipped       = bIsClipped;
            OpenBatch.Kind             = Kind;
            OpenBatch.IndexOffset      = StreamIndexOffset;
            return OpenBatch;
        }
    }

    FUIDrawBatch& NewBatch = Batches.Emplace();
    NewBatch.ScissorRectangle = ScissorRectangle;
    NewBatch.Texture          = Texture;
    NewBatch.IndexOffset      = StreamIndexOffset;
    NewBatch.IndexCount       = 0;
    NewBatch.bIsClipped       = bIsClipped;
    NewBatch.Kind             = Kind;

    return NewBatch;
}

uint64 FUIDrawData::ComputeGeometryHash() const
{
    const int32  VertexByteCount = Vertices.Size() * static_cast<int32>(sizeof(FUIVertex));
    const uint64 VertexHash      = VertexByteCount > 0 ? HashBytes(Vertices.Data(), VertexByteCount) : 0;

    const uint64 IndexHash = Indices.IsEmpty()
        ? 0
        : HashBytes(Indices.Data(), Indices.Size() * static_cast<int32>(sizeof(uint32)));

    const uint64 ShapeVertexHash = ShapeInstances.IsEmpty()
        ? 0
        : HashBytes(ShapeInstances.Data(), ShapeInstances.Size() * static_cast<int32>(sizeof(FUIShapeInstance)));

    const uint64 TextGlyphHash = TextGlyphInstances.IsEmpty()
        ? 0
        : HashBytes(TextGlyphInstances.Data(), TextGlyphInstances.Size() * static_cast<int32>(sizeof(FUITextGlyphInstance)));

    uint64 Hash = 14695981039346656037ull;
    Hash ^= static_cast<uint64>(Vertices.Size());
    Hash *= 1099511628211ull;
    Hash ^= static_cast<uint64>(Indices.Size());
    Hash *= 1099511628211ull;
    Hash ^= static_cast<uint64>(ShapeInstances.Size());
    Hash *= 1099511628211ull;
    Hash ^= static_cast<uint64>(TextGlyphInstances.Size());
    Hash *= 1099511628211ull;

    if (!Vertices.IsEmpty())
    {
        Hash ^= VertexHash;
        Hash *= 1099511628211ull;
    }

    if (!Indices.IsEmpty())
    {
        Hash ^= IndexHash;
        Hash *= 1099511628211ull;
    }

    if (!ShapeInstances.IsEmpty())
    {
        Hash ^= ShapeVertexHash;
        Hash *= 1099511628211ull;
    }

    if (!TextGlyphInstances.IsEmpty())
    {
        Hash ^= TextGlyphHash;
        Hash *= 1099511628211ull;
    }


    return Hash;
}
