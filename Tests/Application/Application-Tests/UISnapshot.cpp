#include "UISnapshot.h"

#include "Application/Draw/DrawCommandList.h"
#include "Application/Draw/UIDrawData.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Text/FontAtlas.h"
#include "Core/Math/Math.h"
#include "RHI/RHITexture.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Image/PngWriter.h"
#include "Core/Platform/PlatformFile.h"

static FORCEINLINE float EdgeFunction(const Vector2& A, const Vector2& B, const Vector2& C)
{
    return ((C.X - A.X) * (B.Y - A.Y)) - ((C.Y - A.Y) * (B.X - A.X));
}

static FORCEINLINE void UnpackRGBA(uint32 Packed, float& OutR, float& OutG, float& OutB, float& OutA)
{
    OutR = static_cast<float>((Packed >> 0)  & 0xFF) / 255.0f;
    OutG = static_cast<float>((Packed >> 8)  & 0xFF) / 255.0f;
    OutB = static_cast<float>((Packed >> 16) & 0xFF) / 255.0f;
    OutA = static_cast<float>((Packed >> 24) & 0xFF) / 255.0f;
}

static FORCEINLINE uint32 PackRGBA(float R, float G, float B, float A)
{
    const uint32 Red   = static_cast<uint32>(Math::Clamp(R, 0.0f, 1.0f) * 255.0f + 0.5f);
    const uint32 Green = static_cast<uint32>(Math::Clamp(G, 0.0f, 1.0f) * 255.0f + 0.5f);
    const uint32 Blue  = static_cast<uint32>(Math::Clamp(B, 0.0f, 1.0f) * 255.0f + 0.5f);
    const uint32 Alpha = static_cast<uint32>(Math::Clamp(A, 0.0f, 1.0f) * 255.0f + 0.5f);

    return Red | (Green << 8) | (Blue << 16) | (Alpha << 24);
}

static FORCEINLINE float SmoothCoverage(float Distance)
{
    const float T = Math::Clamp((Distance + 0.5f) / 1.0f, 0.0f, 1.0f);
    return 1.0f - (T * T * (3.0f - (2.0f * T)));
}

static void RasterizeShapeBatches(const FUIDrawData& DrawData, FSnapshotImage& OutImage);

static float SampleAtlasAlpha(const FFontAtlas& Atlas, float U, float V)
{
    const int32 AtlasWidth  = Atlas.GetWidth();
    const int32 AtlasHeight = Atlas.GetHeight();
    if (AtlasWidth <= 0 || AtlasHeight <= 0)
    {
        return 1.0f;
    }

    const int32 X = Math::Clamp(static_cast<int32>(U * AtlasWidth), 0, AtlasWidth - 1);
    const int32 Y = Math::Clamp(static_cast<int32>(V * AtlasHeight), 0, AtlasHeight - 1);
    return static_cast<float>(Atlas.GetPixels()[((Y * AtlasWidth) + X) * 4 + 3]) / 255.0f;
}

void FSnapshotImage::Initialize(int32 InWidth, int32 InHeight, uint32 ClearColor)
{
    Width  = Math::Max(0, InWidth);
    Height = Math::Max(0, InHeight);

    Pixels.Clear();
    Pixels.Reserve(Width * Height);

    for (int32 Index = 0; Index < Width * Height; ++Index)
    {
        Pixels.Add(ClearColor);
    }
}

uint32 FSnapshotImage::GetPixel(int32 X, int32 Y) const
{
    if (X < 0 || Y < 0 || X >= Width || Y >= Height)
    {
        return 0;
    }

    return Pixels[(Y * Width) + X];
}

void RasterizeDrawData(const FUIDrawData& DrawData, FSnapshotImage& OutImage)
{
    if (!OutImage.IsValid())
    {
        return;
    }

    const TArray<FUIVertex>&    Vertices = DrawData.GetVertices();
    const TArray<uint32>&       Indices  = DrawData.GetIndices();
    const TArray<FUIDrawBatch>& Batches  = DrawData.GetBatches();

    for (const FUIDrawBatch& Batch : Batches)
    {
        if (Batch.Kind == EUIDrawBatchKind::Shape || Batch.Texture.Texture != nullptr)
        {
            continue;
        }

        const FFontAtlas* Atlas = Batch.Texture.Atlas;

        int32 ScissorLeft   = 0;
        int32 ScissorTop    = 0;
        int32 ScissorRight  = OutImage.Width;
        int32 ScissorBottom = OutImage.Height;

        if (Batch.bIsClipped)
        {
            ScissorLeft   = Math::Max(ScissorLeft,   Batch.ScissorRectangle.Position.X);
            ScissorTop    = Math::Max(ScissorTop,    Batch.ScissorRectangle.Position.Y);
            ScissorRight  = Math::Min(ScissorRight,  Batch.ScissorRectangle.GetRight());
            ScissorBottom = Math::Min(ScissorBottom, Batch.ScissorRectangle.GetBottom());
        }

        if (ScissorLeft >= ScissorRight || ScissorTop >= ScissorBottom)
        {
            continue;
        }

        for (int32 Offset = 0; Offset + 2 < Batch.IndexCount; Offset += 3)
        {
            const int32 BaseIndex = Batch.IndexOffset + Offset;
            if (BaseIndex + 2 >= Indices.Size())
            {
                break;
            }

            const FUIVertex& V0 = Vertices[static_cast<int32>(Indices[BaseIndex + 0])];
            const FUIVertex& V1 = Vertices[static_cast<int32>(Indices[BaseIndex + 1])];
            const FUIVertex& V2 = Vertices[static_cast<int32>(Indices[BaseIndex + 2])];

            const float Area = EdgeFunction(V0.Position, V1.Position, V2.Position);
            if (Math::Abs(Area) < 1.0e-6f)
            {
                continue;
            }

            const float MinXf = Math::Min(V0.Position.X, Math::Min(V1.Position.X, V2.Position.X));
            const float MaxXf = Math::Max(V0.Position.X, Math::Max(V1.Position.X, V2.Position.X));
            const float MinYf = Math::Min(V0.Position.Y, Math::Min(V1.Position.Y, V2.Position.Y));
            const float MaxYf = Math::Max(V0.Position.Y, Math::Max(V1.Position.Y, V2.Position.Y));

            const int32 MinX = Math::Max(ScissorLeft,   static_cast<int32>(Math::Floor(MinXf)));
            const int32 MaxX = Math::Min(ScissorRight,  static_cast<int32>(Math::Ceil(MaxXf)));
            const int32 MinY = Math::Max(ScissorTop,    static_cast<int32>(Math::Floor(MinYf)));
            const int32 MaxY = Math::Min(ScissorBottom, static_cast<int32>(Math::Ceil(MaxYf)));

            const float InverseArea = 1.0f / Area;

            for (int32 Y = MinY; Y < MaxY; ++Y)
            {
                for (int32 X = MinX; X < MaxX; ++X)
                {
                    const Vector2 Sample(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f);

                    float W0 = EdgeFunction(V1.Position, V2.Position, Sample) * InverseArea;
                    float W1 = EdgeFunction(V2.Position, V0.Position, Sample) * InverseArea;
                    float W2 = EdgeFunction(V0.Position, V1.Position, Sample) * InverseArea;

                    if (W0 < 0.0f || W1 < 0.0f || W2 < 0.0f)
                    {
                        continue;
                    }

                    float R0, G0, B0, A0;
                    float R1, G1, B1, A1;
                    float R2, G2, B2, A2;
                    UnpackRGBA(V0.Color, R0, G0, B0, A0);
                    UnpackRGBA(V1.Color, R1, G1, B1, A1);
                    UnpackRGBA(V2.Color, R2, G2, B2, A2);

                    float SourceR = (R0 * W0) + (R1 * W1) + (R2 * W2);
                    float SourceG = (G0 * W0) + (G1 * W1) + (G2 * W2);
                    float SourceB = (B0 * W0) + (B1 * W1) + (B2 * W2);
                    float SourceA = (A0 * W0) + (A1 * W1) + (A2 * W2);

                    if (Atlas)
                    {
                        const float U = (V0.TexCoord.X * W0) + (V1.TexCoord.X * W1) + (V2.TexCoord.X * W2);
                        const float V = (V0.TexCoord.Y * W0) + (V1.TexCoord.Y * W1) + (V2.TexCoord.Y * W2);
                        SourceA *= SampleAtlasAlpha(*Atlas, U, V);
                    }

                    if (SourceA <= 0.0f)
                    {
                        continue;
                    }

                    const int32 PixelIndex = (Y * OutImage.Width) + X;

                    float DestR, DestG, DestB, DestA;
                    UnpackRGBA(OutImage.Pixels[PixelIndex], DestR, DestG, DestB, DestA);

                    const float InverseSourceA = 1.0f - SourceA;
                    OutImage.Pixels[PixelIndex] = PackRGBA(
                        (SourceR * SourceA) + (DestR * InverseSourceA),
                        (SourceG * SourceA) + (DestG * InverseSourceA),
                        (SourceB * SourceA) + (DestB * InverseSourceA),
                        SourceA + (DestA * InverseSourceA));
                }
            }
        }
    }

    RasterizeShapeBatches(DrawData, OutImage);
}

static float SignedDistanceToRoundedBox(const Vector2& P, const Vector2& HalfSize, float RadiusTL, float RadiusTR, float RadiusBR, float RadiusBL)
{
    const float BottomRadius = (P.X > 0.0f) ? RadiusBR : RadiusBL;
    const float TopRadius    = (P.X > 0.0f) ? RadiusTR : RadiusTL;
    const float Radius       = (P.Y > 0.0f) ? BottomRadius : TopRadius;

    const Vector2 Q(Math::Abs(P.X) - HalfSize.X + Radius, Math::Abs(P.Y) - HalfSize.Y + Radius);
    return Math::Min(Math::Max(Q.X, Q.Y), 0.0f) + Vector2(Math::Max(Q.X, 0.0f), Math::Max(Q.Y, 0.0f)).GetLength() - Radius;
}

static void RasterizeShapeBatches(const FUIDrawData& DrawData, FSnapshotImage& OutImage)
{
    const TArray<FUIShapeVertex>& Vertices = DrawData.GetShapeVertices();
    const TArray<uint32>&         Indices  = DrawData.GetShapeIndices();

    for (const FUIDrawBatch& Batch : DrawData.GetBatches())
    {
        if (Batch.Kind != EUIDrawBatchKind::Shape)
        {
            continue;
        }

        int32 ScissorLeft   = 0;
        int32 ScissorTop    = 0;
        int32 ScissorRight  = OutImage.Width;
        int32 ScissorBottom = OutImage.Height;

        if (Batch.bIsClipped)
        {
            ScissorLeft   = Math::Max(ScissorLeft,   Batch.ScissorRectangle.Position.X);
            ScissorTop    = Math::Max(ScissorTop,    Batch.ScissorRectangle.Position.Y);
            ScissorRight  = Math::Min(ScissorRight,  Batch.ScissorRectangle.GetRight());
            ScissorBottom = Math::Min(ScissorBottom, Batch.ScissorRectangle.GetBottom());
        }

        for (int32 Offset = 0; Offset + 2 < Batch.IndexCount; Offset += 3)
        {
            const int32 BaseIndex = Batch.IndexOffset + Offset;
            if (BaseIndex + 2 >= Indices.Size())
            {
                break;
            }

            const FUIShapeVertex& V0 = Vertices[static_cast<int32>(Indices[BaseIndex + 0])];
            const FUIShapeVertex& V1 = Vertices[static_cast<int32>(Indices[BaseIndex + 1])];
            const FUIShapeVertex& V2 = Vertices[static_cast<int32>(Indices[BaseIndex + 2])];

            const float Area = EdgeFunction(V0.Position, V1.Position, V2.Position);
            if (Math::Abs(Area) < 1.0e-6f)
            {
                continue;
            }

            const float InverseArea = 1.0f / Area;
            const float MinXf = Math::Min(V0.Position.X, Math::Min(V1.Position.X, V2.Position.X));
            const float MaxXf = Math::Max(V0.Position.X, Math::Max(V1.Position.X, V2.Position.X));
            const float MinYf = Math::Min(V0.Position.Y, Math::Min(V1.Position.Y, V2.Position.Y));
            const float MaxYf = Math::Max(V0.Position.Y, Math::Max(V1.Position.Y, V2.Position.Y));

            const int32 MinX = Math::Max(ScissorLeft,   static_cast<int32>(Math::Floor(MinXf)));
            const int32 MaxX = Math::Min(ScissorRight,  static_cast<int32>(Math::Ceil(MaxXf)));
            const int32 MinY = Math::Max(ScissorTop,    static_cast<int32>(Math::Floor(MinYf)));
            const int32 MaxY = Math::Min(ScissorBottom, static_cast<int32>(Math::Ceil(MaxYf)));

            for (int32 Y = MinY; Y < MaxY; ++Y)
            {
                for (int32 X = MinX; X < MaxX; ++X)
                {
                    const Vector2 Sample(static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f);
                    float W0 = EdgeFunction(V1.Position, V2.Position, Sample) * InverseArea;
                    float W1 = EdgeFunction(V2.Position, V0.Position, Sample) * InverseArea;
                    float W2 = EdgeFunction(V0.Position, V1.Position, Sample) * InverseArea;

                    if (W0 < 0.0f || W1 < 0.0f || W2 < 0.0f)
                    {
                        continue;
                    }

                    const Vector2 Local(
                        (V0.LocalPos.X * W0) + (V1.LocalPos.X * W1) + (V2.LocalPos.X * W2),
                        (V0.LocalPos.Y * W0) + (V1.LocalPos.Y * W1) + (V2.LocalPos.Y * W2));

                    float Distance = 0.0f;
                    if (V0.ShapeKind > 1.5f)
                    {
                        Distance = (Local - Vector2(V0.RadiusTL, V0.RadiusTR)).GetLength() - V0.RadiusBR;
                    }
                    else
                    {
                        const Vector2 Centered(Local.X - (V0.RectSize.X * 0.5f), Local.Y - (V0.RectSize.Y * 0.5f));
                        Distance = SignedDistanceToRoundedBox(Centered, Vector2(V0.RectSize.X * 0.5f, V0.RectSize.Y * 0.5f),
                            V0.RadiusTL, V0.RadiusTR, V0.RadiusBR, V0.RadiusBL);

                        if (V0.ShapeKind > 0.5f)
                        {
                            Distance = Math::Abs(Distance) - (V0.Thickness * 0.5f);
                        }
                    }

                    const float Coverage = SmoothCoverage(Distance);

                    float R, G, B, A;
                    UnpackRGBA(V0.Color, R, G, B, A);

                    A *= Coverage;

                    if (A <= 0.0f)
                    {
                        continue;
                    }

                    const int32 PixelIndex = (Y * OutImage.Width) + X;

                    float DestR, DestG, DestB, DestA;
                    UnpackRGBA(OutImage.Pixels[PixelIndex], DestR, DestG, DestB, DestA);

                    const float InverseSourceA = 1.0f - A;
                    OutImage.Pixels[PixelIndex] = PackRGBA(
                        (R * A) + (DestR * InverseSourceA),
                        (G * A) + (DestG * InverseSourceA),
                        (B * A) + (DestB * InverseSourceA),
                        A + (DestA * InverseSourceA));
                }
            }
        }
    }
}

bool WriteSnapshotPng(const FSnapshotImage& Image, const String& Filename)
{
    if (!Image.IsValid())
    {
        LOG_ERROR("[UISnapshot]: Cannot write '%s' from an image of %dx%d", *Filename, Image.Width, Image.Height);
        return false;
    }

    const FImageView View(reinterpret_cast<const uint8*>(Image.Pixels.Data()), Image.Width, Image.Height);

    FPngWriter Writer;
    if (!Writer.WriteToFile(Filename, View))
    {
        return false;
    }

    LOG_INFO("[UISnapshot]: Wrote %dx%d snapshot to '%s'", Image.Width, Image.Height, *Filename);
    return true;
}

bool SnapshotElementToPng(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, uint32 ClearColor, const String& Filename)
{
    if (!Element || Size.X <= 0 || Size.Y <= 0)
    {
        return false;
    }

    const FRectangle Bounds(IntVector2(0, 0), Size.X, Size.Y);

    Element->PrepareDesiredSize();
    Element->Tick(Bounds);

    FDrawCommandList CommandList;
    Element->OnDraw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), CommandList, 0);

    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(CommandList);

    FSnapshotImage Image;
    Image.Initialize(Size.X, Size.Y, ClearColor);

    RasterizeDrawData(DrawData, Image);

    return WriteSnapshotPng(Image, Filename);
}

bool EnsureSnapshotDirectory(const String& Directory)
{
    String Partial;
    for (int32 Index = 0; Index < Directory.Length(); ++Index)
    {
        const CHAR Character = Directory[Index];
        Partial.Append(Character);

        if (Character == '/' || Character == '\\')
        {
            FPlatformFile::CreateDirectory(*Partial);
        }
    }

    return FPlatformFile::CreateDirectory(*Directory);
}

String GetSnapshotDirectory(const String& SubDirectory)
{
    return String("Screenshots/") + SubDirectory;
}
