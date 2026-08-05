#include "Core/Math/BlueNoiseGenerator.h"
#include "Core/Math/Random.h"
#include "Core/Misc/Asserts.h"
#include "Core/Tasks/ParallelFor.h"

static FORCEINLINE uint64 MixSeed(uint32 Seed)
{
    uint64 State = static_cast<uint64>(Seed) + 0x9E3779B97F4A7C15ull;
    State = (State ^ (State >> 30)) * 0xBF58476D1CE4E5B9ull;
    State = (State ^ (State >> 27)) * 0x94D049BB133111EBull;
    State =  State ^ (State >> 31);
    return (State != 0) ? State : 0x9E3779B97F4A7C15ull;
}

static FORCEINLINE int32 WrapOffset(int32 Offset, int32 Extent)
{
    if (Offset > (Extent / 2))
    {
        Offset -= Extent;
    }

    return Offset;
}

static FORCEINLINE int32 WrapIndex(int32 Index, int32 Extent)
{
    Index %= Extent;
    return (Index < 0) ? (Index + Extent) : Index;
}

static FORCEINLINE int32 WrapSmallOffset(int32 Index, int32 Extent)
{
    if (Index < 0)
    {
        return Index + Extent;
    }

    return (Index >= Extent) ? (Index - Extent) : Index;
}

/**
 * @brief The energy kernel shared by both algorithms, in the separable spatiotemporal form.
 *
 * Wolfe et al. 2022: Energy is returned only between samples that share a slice or share a
 * pixel. This is deliberately not a 3D distance. A true 3D kernel produces a mask that is
 * neither good spatial blue noise per slice nor good temporal blue noise per pixel. Depth == 1
 * degenerates to the ordinary 2D kernel, so the same code covers both cases.
 */
class FEnergyKernel
{
public:
    FEnergyKernel(int32 InWidth, int32 InHeight, int32 InDepth, float Sigma, float SigmaScale)
        : Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
        , SliceSize(InWidth * InHeight)
    {
        const float Denominator = SigmaScale * Sigma * Sigma;

        // Tabulated over raw toroidal offsets so a splat is a straight indexed walk.
        Spatial.Resize(SliceSize);

        for (int32 OffsetY = 0; OffsetY < Height; ++OffsetY)
        {
            const int32 DeltaY = WrapOffset(OffsetY, Height);
            for (int32 OffsetX = 0; OffsetX < Width; ++OffsetX)
            {
                const int32 DeltaX  = WrapOffset(OffsetX, Width);
                const float SqrDist = static_cast<float>((DeltaX * DeltaX) + (DeltaY * DeltaY));

                Spatial[(OffsetY * Width) + OffsetX] = Math::Exp(-SqrDist / Denominator);
            }
        }

        Temporal.Resize(Math::Max(Depth, 1));
        for (int32 OffsetZ = 0; OffsetZ < Depth; ++OffsetZ)
        {
            const int32 DeltaZ  = WrapOffset(OffsetZ, Depth);
            const float SqrDist = static_cast<float>(DeltaZ * DeltaZ);

            Temporal[OffsetZ] = Math::Exp(-SqrDist / Denominator);
        }

        // Anything below this contributes less than a float epsilon to a realistic sum, so the
        // swap algorithm can skip it. Void-and-cluster still walks the full toroidal domain.
        SpatialRadius  = 1;
        TemporalRadius = 1;

        while ((SpatialRadius < (Width / 2)) && (SpatialRadius < (Height / 2)) && (Math::Exp(-static_cast<float>(SpatialRadius * SpatialRadius) / Denominator) > 1.0e-6f))
        {
            ++SpatialRadius;
        }

        while ((TemporalRadius < (Depth / 2)) && (Math::Exp(-static_cast<float>(TemporalRadius * TemporalRadius) / Denominator) > 1.0e-6f))
        {
            ++TemporalRadius;
        }
    }

    FORCEINLINE float GetSpatial(int32 OffsetY, int32 OffsetX) const
    {
        return Spatial[(OffsetY * Width) + OffsetX];
    }

    FORCEINLINE const float* GetSpatialRow(int32 OffsetY) const
    {
        return Spatial.Data() + (OffsetY * Width);
    }

    FORCEINLINE float GetTemporal(int32 OffsetZ) const
    {
        return Temporal[OffsetZ];
    }

    int32 GetSpatialRadius() const  { return SpatialRadius; }
    int32 GetTemporalRadius() const { return TemporalRadius; }

private:
    int32 Width;
    int32 Height;
    int32 Depth;
    int32 SliceSize;
    int32 SpatialRadius;
    int32 TemporalRadius;

    TArray<float> Spatial;
    TArray<float> Temporal;
};

class FVoidAndClusterState
{
public:
    FVoidAndClusterState(int32 InWidth, int32 InHeight, int32 InDepth, const FEnergyKernel& InKernel, bool bInUseReference)
        : Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
        , SliceSize(InWidth * InHeight)
        , Count(InWidth * InHeight * InDepth)
        , Kernel(InKernel)
        , bUseReference(bInUseReference)
    {
        Pattern.Resize(Count);
        Pattern.Fill(0);
        Energy.Resize(Count);
        Energy.Fill(0.0f);
        DirtySlices.Resize(Depth);
        DirtySlices.Fill(1);
        SliceMin.Resize(Depth);
        SliceMax.Resize(Depth);
    }

    bool IsSet(int32 Index) const { return Pattern[Index] != 0; }

    void SetSample(int32 Index, bool bSet)
    {
        Pattern[Index] = bSet ? 1 : 0;
        Splat(Index, bSet ? 1.0f : -1.0f);
    }

    int32 FindTightestCluster()
    {
        RefreshSummaries();

        int32 Best      = -1;
        float BestValue = -Math::Constants::Infinity;

        for (int32 Slice = 0; Slice < Depth; ++Slice)
        {
            if ((SliceMax[Slice].Index >= 0) && (SliceMax[Slice].Value > BestValue))
            {
                BestValue = SliceMax[Slice].Value;
                Best      = SliceMax[Slice].Index;
            }
        }

        return Best;
    }

    int32 FindLargestVoid()
    {
        RefreshSummaries();

        int32 Best      = -1;
        float BestValue = Math::Constants::Infinity;

        for (int32 Slice = 0; Slice < Depth; ++Slice)
        {
            if ((SliceMin[Slice].Index >= 0) && (SliceMin[Slice].Value < BestValue))
            {
                BestValue = SliceMin[Slice].Value;
                Best      = SliceMin[Slice].Index;
            }
        }

        return Best;
    }

    const TArray<uint8>& GetPattern() const { return Pattern; }
    const TArray<float>& GetEnergy() const  { return Energy; }

    void SetPattern(const TArray<uint8>& InPattern)
    {
        Pattern = InPattern;
        RebuildEnergy();
    }
    
    void RebuildEnergy()
    {
        Energy.Fill(0.0f);

        for (int32 Index = 0; Index < Count; ++Index)
        {
            if (Pattern[Index] != 0)
            {
                Splat(Index, 1.0f);
            }
        }

        DirtySlices.Fill(1);
    }

private:
    struct FExtremum
    {
        float Value = 0.0f;
        int32 Index = -1;
    };

    void Splat(int32 Index, float Sign)
    {
        if (bUseReference)
        {
            SplatReference(Index, Sign);
        }
        else
        {
            SplatSeparable(Index, Sign);
        }
    }

    void SplatReference(int32 Index, float Sign)
    {
        const int32 SourceZ = Index / SliceSize;
        const int32 SourceY = (Index % SliceSize) / Width;
        const int32 SourceX = Index % Width;

        for (int32 Z = 0; Z < Depth; ++Z)
        {
            for (int32 Y = 0; Y < Height; ++Y)
            {
                for (int32 X = 0; X < Width; ++X)
                {
                    const bool bSameSlice = (Z == SourceZ);
                    const bool bSamePixel = (X == SourceX) && (Y == SourceY);

                    if (!bSameSlice && !bSamePixel)
                    {
                        continue;
                    }

                    const int32 OffsetX = WrapIndex(X - SourceX, Width);
                    const int32 OffsetY = WrapIndex(Y - SourceY, Height);
                    const int32 OffsetZ = WrapIndex(Z - SourceZ, Depth);

                    const float Contribution = bSameSlice ? Kernel.GetSpatial(OffsetY, OffsetX) : Kernel.GetTemporal(OffsetZ);
                    Energy[(Z * SliceSize) + (Y * Width) + X] += Sign * Contribution;
                }
            }
        }

        DirtySlices.Fill(1);
    }

    void SplatSeparable(int32 Index, float Sign)
    {
        const int32 SourceZ = Index / SliceSize;
        const int32 SourceY = (Index % SliceSize) / Width;
        const int32 SourceX = Index % Width;

        float* SliceEnergy = Energy.Data() + (SourceZ * SliceSize);

        const int32 SplitX = Width - SourceX;
        const int32 SplitY = Height - SourceY;

        for (int32 OffsetY = 0; OffsetY < Height; ++OffsetY)
        {
            const int32  TargetY   = (OffsetY < SplitY) ? (SourceY + OffsetY) : (OffsetY - SplitY);
            float*       RowStart  = SliceEnergy + (TargetY * Width);
            const float* KernelRow = Kernel.GetSpatialRow(OffsetY);

            for (int32 OffsetX = 0; OffsetX < SplitX; ++OffsetX)
            {
                RowStart[SourceX + OffsetX] += Sign * KernelRow[OffsetX];
            }

            for (int32 OffsetX = SplitX; OffsetX < Width; ++OffsetX)
            {
                RowStart[OffsetX - SplitX] += Sign * KernelRow[OffsetX];
            }
        }

        DirtySlices[SourceZ] = 1;

        for (int32 OffsetZ = 1; OffsetZ < Depth; ++OffsetZ)
        {
            const int32 TargetZ     = WrapIndex(SourceZ + OffsetZ, Depth);
            const int32 TargetIndex = (TargetZ * SliceSize) + (SourceY * Width) + SourceX;

            Energy[TargetIndex] += Sign * Kernel.GetTemporal(OffsetZ);

            InvalidateSlice(TargetZ, TargetIndex);
        }
    }

    void InvalidateSlice(int32 Slice, int32 ChangedIndex)
    {
        if (DirtySlices[Slice] != 0)
        {
            return;
        }

        const float Value = Energy[ChangedIndex];
        if (Pattern[ChangedIndex] != 0)
        {
            if ((SliceMax[Slice].Index == ChangedIndex) || (Value > SliceMax[Slice].Value))
            {
                DirtySlices[Slice] = 1;
            }
        }
        else
        {
            if ((SliceMin[Slice].Index == ChangedIndex) || (Value < SliceMin[Slice].Value))
            {
                DirtySlices[Slice] = 1;
            }
        }
    }

    void RefreshSummaries()
    {
        for (int32 Slice = 0; Slice < Depth; ++Slice)
        {
            if (DirtySlices[Slice] == 0)
            {
                continue;
            }

            FExtremum Min;
            FExtremum Max;
            Min.Value = Math::Constants::Infinity;
            Max.Value = -Math::Constants::Infinity;

            const int32 Base = Slice * SliceSize;
            for (int32 Local = 0; Local < SliceSize; ++Local)
            {
                const int32 Index = Base + Local;
                const float Value = Energy[Index];

                if (Pattern[Index] != 0)
                {
                    if (Value > Max.Value)
                    {
                        Max.Value = Value;
                        Max.Index = Index;
                    }
                }
                else if (Value < Min.Value)
                {
                    Min.Value = Value;
                    Min.Index = Index;
                }
            }

            SliceMin[Slice]    = Min;
            SliceMax[Slice]    = Max;
            DirtySlices[Slice] = 0;
        }
    }

    int32 Width;
    int32 Height;
    int32 Depth;
    int32 SliceSize;
    int32 Count;

    const FEnergyKernel& Kernel;
    bool                 bUseReference;

    TArray<uint8>     Pattern;
    TArray<float>     Energy;
    TArray<uint8>     DirtySlices;
    TArray<FExtremum> SliceMin;
    TArray<FExtremum> SliceMax;
};

static FORCEINLINE void ReportProgress(FBlueNoiseCallbacks& Callbacks, float Fraction, const CHAR* Phase)
{
    if (Callbacks.Progress)
    {
        Callbacks.Progress(Math::Clamp(Fraction, 0.0f, 1.0f), Phase);
    }
}

static float ResolveSigmaSpatial(const FBlueNoiseParams& Params)
{
    if (Params.SigmaSpatial > 0.0f)
    {
        return Params.SigmaSpatial;
    }

    const bool bSpatiotemporal = (Params.Depth > 1);
    if (Params.Algorithm == EBlueNoiseAlgorithm::VoidAndCluster)
    {
        return bSpatiotemporal ? 1.9f : 1.5f;
    }

    return bSpatiotemporal ? 1.9f : 2.1f;
}

TArray<float> FBlueNoiseGenerator::Generate(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks)
{
    if (Params.Algorithm == EBlueNoiseAlgorithm::VoidAndCluster)
    {
        return GenerateVoidAndCluster(Params, Move(Callbacks));
    }

    return GenerateVectorBlueNoise(Params, Move(Callbacks));
}

TArray<float> FBlueNoiseGenerator::GenerateVoidAndCluster(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks)
{
    CHECK(Params.Channels == 1);
    CHECK(Params.Width > 0 && Params.Height > 0 && Params.Depth > 0);

    const int32 Width  = Params.Width;
    const int32 Height = Params.Height;
    const int32 Depth  = Params.Depth;
    const int32 Count  = Width * Height * Depth;

    // Ulichney's kernel is exp(-d^2 / 2*sigma^2).
    const FEnergyKernel Kernel(Width, Height, Depth, ResolveSigmaSpatial(Params), 2.0f);
    FVoidAndClusterState State(Width, Height, Depth, Kernel, Params.bUseReferenceEnergy);

    // ---------------------------------------------------------------------------------------------
    // Initial binary pattern at 10 percent density.
    // ---------------------------------------------------------------------------------------------

    const int32 NumInitial = Math::Max(1, Count / 10);

    FRandom Random;
    Random.SetSeed(MixSeed(Params.Seed));

    for (int32 Placed = 0; Placed < NumInitial; ++Placed)
    {
        int32 Index = static_cast<int32>(Random.RandInt(0, Count - 1));
        while (State.IsSet(Index))
        {
            Index = (Index + 1) % Count;
        }

        State.SetSample(Index, true);
    }

    ReportProgress(Callbacks, 0.0f, "Prototype");

    const int32 MaxRelaxIterations = NumInitial * 4;
    for (int32 Iteration = 0; Iteration < MaxRelaxIterations; ++Iteration)
    {
        const int32 Cluster = State.FindTightestCluster();
        if (Cluster < 0)
        {
            break;
        }

        State.SetSample(Cluster, false);

        const int32 Void = State.FindLargestVoid();
        if ((Void < 0) || (Void == Cluster))
        {
            State.SetSample(Cluster, true);
            break;
        }

        State.SetSample(Void, true);
    }

    const TArray<uint8> Prototype = State.GetPattern();

    TArray<int32> Rank;
    Rank.Resize(Count);
    Rank.Fill(-1);

    // ---------------------------------------------------------------------------------------------
    // Phase 1: Strip the prototype back to nothing, tightest cluster first. The last sample removed
    // is the most isolated, so it takes rank 0.
    // ---------------------------------------------------------------------------------------------

    ReportProgress(Callbacks, 0.0f, "Phase1");

    for (int32 CurrentRank = NumInitial - 1; CurrentRank >= 0; --CurrentRank)
    {
        const int32 Cluster = State.FindTightestCluster();
        if (Cluster < 0)
        {
            break;
        }

        State.SetSample(Cluster, false);
        Rank[Cluster] = CurrentRank;

        if (((CurrentRank & 0x3ff) == 0))
        {
            ReportProgress(Callbacks, 1.0f - (static_cast<float>(CurrentRank) / static_cast<float>(NumInitial)), "Phase1");
        }
    }

    // ---------------------------------------------------------------------------------------------
    // Phase 2: Refill from the prototype, largest void first.
    // ---------------------------------------------------------------------------------------------

    State.SetPattern(Prototype);

    ReportProgress(Callbacks, 0.0f, "Phase2");

    for (int32 CurrentRank = NumInitial; CurrentRank < Count; ++CurrentRank)
    {
        const int32 Void = State.FindLargestVoid();
        if (Void < 0)
        {
            break;
        }

        State.SetSample(Void, true);
        Rank[Void] = CurrentRank;

        if (((CurrentRank & 0x3ff) == 0))
        {
            ReportProgress(Callbacks, static_cast<float>(CurrentRank - NumInitial) / static_cast<float>(Count - NumInitial), "Phase2");
        }
    }

    // ---------------------------------------------------------------------------------------------
    // The rank permutation maps onto [0, 1) directly.
    // ---------------------------------------------------------------------------------------------

    TArray<float> Result;
    Result.Resize(Count);

    const float InvCount = 1.0f / static_cast<float>(Count);
    for (int32 Index = 0; Index < Count; ++Index)
    {
        Result[Index] = static_cast<float>(Math::Max(Rank[Index], 0)) * InvCount;
    }

    ReportProgress(Callbacks, 1.0f, "Done");
    return Result;
}

TArray<float> FBlueNoiseGenerator::GenerateVectorBlueNoise(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks)
{
    CHECK(Params.Channels >= 1);
    CHECK(Params.Width > 0 && Params.Height > 0 && Params.Depth > 0);

    const int32 Width      = Params.Width;
    const int32 Height     = Params.Height;
    const int32 Depth      = Params.Depth;
    const int32 Channels   = Params.Channels;
    const int32 SliceSize  = Width * Height;
    const int32 PixelCount = SliceSize * Depth;

    // Georgiev-Fajardo's kernel is exp(-d^2 / sigma^2), without the factor of two.
    const FEnergyKernel Kernel(Width, Height, Depth, ResolveSigmaSpatial(Params), 1.0f);

    const int32  SpatialRadius  = Kernel.GetSpatialRadius();
    const int32  TemporalRadius = (Depth > 1) ? Kernel.GetTemporalRadius() : 0;
    const float* SpatialTable   = Kernel.GetSpatialRow(0);

    // The value-space falloff exponent follows the dimensionality of the domain: d/2 for a 2D mask,
    // d/3 once the mask is spatiotemporal.
    const float ValueExponent = static_cast<float>(Channels) / ((Depth > 1) ? 3.0f : 2.0f);
    const float InvSigmaValue = 1.0f / (Params.SigmaValue * Params.SigmaValue);

    TArray<float> Values;

    FRandom Random;
    Random.SetSeed(MixSeed(Params.Seed));

    if (Params.InitialValues.Size() == (PixelCount * Channels))
    {
        Values = Params.InitialValues;
    }
    else
    {
        Values.Resize(PixelCount * Channels);

        for (int32 Channel = 0; Channel < Channels; ++Channel)
        {
            for (int32 Pixel = 0; Pixel < PixelCount; ++Pixel)
            {
                Values[(Pixel * Channels) + Channel] = (static_cast<float>(Pixel) + 0.5f) / static_cast<float>(PixelCount);
            }

            for (int32 Pixel = PixelCount - 1; Pixel > 0; --Pixel)
            {
                const int32 Other = static_cast<int32>(Random.RandInt(0, Pixel));
                const float Temp  = Values[(Pixel * Channels) + Channel];

                Values[(Pixel * Channels) + Channel] = Values[(Other * Channels) + Channel];
                Values[(Other * Channels) + Channel] = Temp;
            }
        }
    }

    // |a - b|^e == (|a - b|^2)^(e/2), so the square root folds into the exponent for free.
    const float HalfExponent = ValueExponent * 0.5f;

    // A 2D mask puts the half-exponent on Channels / 4
    const bool bIsSqrt        = (Depth == 1) && (Channels == 2);
    const bool bIsQuarterRoot = (Depth == 1) && (Channels == 1);

    const auto ValueDistance = [&](int32 PixelA, int32 PixelB) -> float
    {
        float SqrDistance = 0.0f;
        for (int32 Channel = 0; Channel < Channels; ++Channel)
        {
            const float Delta = Values[(PixelA * Channels) + Channel] - Values[(PixelB * Channels) + Channel];
            SqrDistance += Delta * Delta;
        }

        if (SqrDistance <= 0.0f)
        {
            return 0.0f;
        }

        if (bIsSqrt)
        {
            return Math::Sqrt(SqrDistance);
        }

        if (bIsQuarterRoot)
        {
            return Math::Sqrt(Math::Sqrt(SqrDistance));
        }

        return Math::Pow(SqrDistance, HalfExponent);
    };

    constexpr int32 InvalidPixel = -1;

    const auto PixelEnergy = [&](int32 Pixel, int32 SwapA, int32 SwapB) -> float
    {
        const auto Remap = [SwapA, SwapB](int32 Index) -> int32
        {
            if (Index == SwapA)
            {
                return SwapB;
            }

            return (Index == SwapB) ? SwapA : Index;
        };

        const int32 PixelZ = Pixel / SliceSize;
        const int32 PixelY = (Pixel % SliceSize) / Width;
        const int32 PixelX = Pixel % Width;

        const int32 SourcePixel = Remap(Pixel);

        float Energy = 0.0f;
        for (int32 OffsetY = -SpatialRadius; OffsetY <= SpatialRadius; ++OffsetY)
        {
            const int32 NeighbourY   = WrapSmallOffset(PixelY + OffsetY, Height);
            const int32 KernelRow    = WrapSmallOffset(OffsetY, Height) * Width;
            const int32 NeighbourRow = (PixelZ * SliceSize) + (NeighbourY * Width);

            for (int32 OffsetX = -SpatialRadius; OffsetX <= SpatialRadius; ++OffsetX)
            {
                if ((OffsetX == 0) && (OffsetY == 0))
                {
                    continue;
                }

                const int32 NeighbourX = WrapSmallOffset(PixelX + OffsetX, Width);
                const int32 Neighbour  = NeighbourRow + NeighbourX;
                const float Spatial    = SpatialTable[KernelRow + WrapSmallOffset(OffsetX, Width)];

                Energy += Spatial * Math::Exp(-ValueDistance(SourcePixel, Remap(Neighbour)) * InvSigmaValue);
            }
        }

        for (int32 OffsetZ = -TemporalRadius; OffsetZ <= TemporalRadius; ++OffsetZ)
        {
            if (OffsetZ == 0)
            {
                continue;
            }

            const int32 NeighbourZ = WrapSmallOffset(PixelZ + OffsetZ, Depth);
            const int32 Neighbour  = (NeighbourZ * SliceSize) + (PixelY * Width) + PixelX;

            const float Temporal = Kernel.GetTemporal(WrapSmallOffset(OffsetZ, Depth));
            Energy += Temporal * Math::Exp(-ValueDistance(SourcePixel, Remap(Neighbour)) * InvSigmaValue);
        }

        return Energy;
    };

    // Energy change from swapping a pair, evaluated without touching Values.
    const auto SwapDelta = [&](int32 PixelA, int32 PixelB) -> float
    {
        const float Before = PixelEnergy(PixelA, InvalidPixel, InvalidPixel) + PixelEnergy(PixelB, InvalidPixel, InvalidPixel);
        const float After  = PixelEnergy(PixelA, PixelA, PixelB) + PixelEnergy(PixelB, PixelA, PixelB);
        return After - Before;
    };

    const int64 NumIterations = (Params.NumIterations > 0) ? Params.NumIterations : (static_cast<int64>(PixelCount) * 64);

    constexpr float StartTemperature = 0.5f;
    constexpr float EndTemperature   = 0.0005f;

    const float TemperatureDecay = Math::Pow(EndTemperature / StartTemperature, 1.0f / static_cast<float>(Math::Max<int64>(NumIterations, 1LL)));

    // A resumed run picks the schedule up where it left off rather than reheating from the start.
    const int64 StartIteration = Math::Clamp<int64>(Params.StartIteration, 0, NumIterations);
    float       Temperature    = StartTemperature * Math::Pow(TemperatureDecay, static_cast<float>(StartIteration));

    ReportProgress(Callbacks, static_cast<float>(StartIteration) / static_cast<float>(NumIterations), "Anneal");

    const int32 BatchSize = Math::Clamp(PixelCount / 256, 1, 4096);

    TArray<int32> BatchPixelA;
    TArray<int32> BatchPixelB;
    TArray<float> BatchAccept;
    TArray<float> BatchDelta;
    BatchPixelA.Resize(BatchSize);
    BatchPixelB.Resize(BatchSize);
    BatchAccept.Resize(BatchSize);
    BatchDelta.Resize(BatchSize);

    TArray<int64> TouchedStamp;
    TouchedStamp.Resize(PixelCount);
    TouchedStamp.Fill(-1);

    int64 Iteration  = StartIteration;
    int64 BatchIndex = 0;

    while (Iteration < NumIterations)
    {
        const int32 Count = static_cast<int32>(Math::Min<int64>(BatchSize, NumIterations - Iteration));

        for (int32 Index = 0; Index < Count; ++Index)
        {
            BatchPixelA[Index] = static_cast<int32>(Random.RandInt(0, PixelCount - 1));
            BatchPixelB[Index] = static_cast<int32>(Random.RandInt(0, PixelCount - 1));
            BatchAccept[Index] = Random.RandFloat(0.0f, 1.0f);
        }

        Tasks::ParallelForRange(Count, [&](int32 Begin, int32 End)
        {
            for (int32 Index = Begin; Index < End; ++Index)
            {
                const int32 PixelA = BatchPixelA[Index];
                const int32 PixelB = BatchPixelB[Index];
                BatchDelta[Index]  = (PixelA != PixelB) ? SwapDelta(PixelA, PixelB) : 0.0f;
            }
        }, 1);

        for (int32 Index = 0; Index < Count; ++Index, ++Iteration)
        {
            const int32 PixelA = BatchPixelA[Index];
            const int32 PixelB = BatchPixelB[Index];

            Temperature *= TemperatureDecay;

            if (PixelA == PixelB)
            {
                continue;
            }

            float Delta = BatchDelta[Index];
            if ((TouchedStamp[PixelA] == BatchIndex) || (TouchedStamp[PixelB] == BatchIndex))
            {
                Delta = SwapDelta(PixelA, PixelB);
            }

            bool bAccept = (Delta < 0.0f);
            if (!bAccept && (Temperature > 0.0f))
            {
                bAccept = BatchAccept[Index] < Math::Exp(-Delta / Temperature);
            }

            if (!bAccept)
            {
                continue;
            }

            for (int32 Channel = 0; Channel < Channels; ++Channel)
            {
                const float Temp                      = Values[(PixelA * Channels) + Channel];
                Values[(PixelA * Channels) + Channel] = Values[(PixelB * Channels) + Channel];
                Values[(PixelB * Channels) + Channel] = Temp;
            }

            TouchedStamp[PixelA] = BatchIndex;
            TouchedStamp[PixelB] = BatchIndex;
        }

        ++BatchIndex;

        ReportProgress(Callbacks, static_cast<float>(Iteration) / static_cast<float>(NumIterations), "Anneal");

        if (Callbacks.Checkpoint && (Params.CheckpointInterval > 0) && ((Iteration - StartIteration) >= Params.CheckpointInterval))
        {
            if ((Iteration % Params.CheckpointInterval) < BatchSize)
            {
                Callbacks.Checkpoint(Values, Iteration);
            }
        }
    }

    ReportProgress(Callbacks, 1.0f, "Done");
    return Values;
}

TArray<float> FBlueNoiseGenerator::GenerateWhiteNoise(const FBlueNoiseParams& Params)
{
    const int32 Count = static_cast<int32>(Params.GetValueCount());

    TArray<float> Result;
    Result.Resize(Count);

    FRandom Random;
    Random.SetSeed(MixSeed(Params.Seed));

    for (int32 Index = 0; Index < Count; ++Index)
    {
        Result[Index] = Random.RandFloat(0.0f, 1.0f);
    }

    return Result;
}

TArray<float> FBlueNoiseGenerator::ComputeRadialPowerSpectrum(const TArray<float>& Values, const FBlueNoiseParams& Params, int32 Slice, int32 Channel, int32 NumBins)
{
    const int32 Width     = Params.Width;
    const int32 Height    = Params.Height;
    const int32 Channels  = Math::Max(Params.Channels, 1);
    const int32 SliceBase = Slice * Width * Height;

    TArray<float> Samples;
    Samples.Resize(Width * Height);

    float Mean = 0.0f;
    for (int32 Index = 0; Index < (Width * Height); ++Index)
    {
        Samples[Index] = Values[((SliceBase + Index) * Channels) + Channel];
        Mean += Samples[Index];
    }

    Mean /= static_cast<float>(Width * Height);
    for (int32 Index = 0; Index < (Width * Height); ++Index)
    {
        Samples[Index] -= Mean;
    }

    const float MaxRadius = Math::Sqrt(static_cast<float>(((Width / 2) * (Width / 2)) + ((Height / 2) * (Height / 2))));

    TArray<float> Accumulated;
    TArray<float> Counts;
    Accumulated.Resize(NumBins);
    Accumulated.Fill(0.0f);
    Counts.Resize(NumBins);
    Counts.Fill(0.0f);

    TArray<float> Power;
    Power.Resize(Width * Height);
    Power.Fill(0.0f);

    Tasks::ParallelForRange(Height, [&](int32 Begin, int32 End)
    {
        for (int32 FreqY = Begin; FreqY < End; ++FreqY)
        {
            for (int32 FreqX = 0; FreqX < Width; ++FreqX)
            {
                float Real = 0.0f;
                float Imag = 0.0f;

                for (int32 Y = 0; Y < Height; ++Y)
                {
                    for (int32 X = 0; X < Width; ++X)
                    {
                        const float Angle = -Math::Constants::TwoPI * ((static_cast<float>(FreqX * X) / static_cast<float>(Width)) + (static_cast<float>(FreqY * Y) / static_cast<float>(Height)));
                        const float Value = Samples[(Y * Width) + X];

                        Real += Value * Math::Cos(Angle);
                        Imag += Value * Math::Sin(Angle);
                    }
                }

                Power[(FreqY * Width) + FreqX] = (Real * Real) + (Imag * Imag);
            }
        }
    });

    for (int32 FreqY = 0; FreqY < Height; ++FreqY)
    {
        const int32 SignedY = WrapOffset(FreqY, Height);
        for (int32 FreqX = 0; FreqX < Width; ++FreqX)
        {
            if ((FreqX == 0) && (FreqY == 0))
            {
                continue;
            }

            const int32 SignedX = WrapOffset(FreqX, Width);
            const float Radius  = Math::Sqrt(static_cast<float>((SignedX * SignedX) + (SignedY * SignedY)));
            const int32 Bin     = Math::Min(static_cast<int32>((Radius / MaxRadius) * static_cast<float>(NumBins)), NumBins - 1);

            Accumulated[Bin] += Power[(FreqY * Width) + FreqX];
            Counts[Bin]      += 1.0f;
        }
    }

    for (int32 Bin = 0; Bin < NumBins; ++Bin)
    {
        if (Counts[Bin] > 0.0f)
        {
            Accumulated[Bin] /= Counts[Bin];
        }
    }

    return Accumulated;
}

float FBlueNoiseGenerator::ComputeLowFrequencyEnergy(const TArray<float>& Spectrum, float LowFrequencyFraction)
{
    const int32 NumBins  = Spectrum.Size();
    const int32 CutoffBin = Math::Max(1, static_cast<int32>(static_cast<float>(NumBins) * LowFrequencyFraction));

    float LowEnergy   = 0.0f;
    float TotalEnergy = 0.0f;

    for (int32 Bin = 0; Bin < NumBins; ++Bin)
    {
        TotalEnergy += Spectrum[Bin];
        if (Bin < CutoffBin)
        {
            LowEnergy += Spectrum[Bin];
        }
    }

    return (TotalEnergy > 0.0f) ? (LowEnergy / TotalEnergy) : 0.0f;
}
