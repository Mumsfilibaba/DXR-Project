#pragma once
#include "Core/Math/Math.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Function.h"

enum class EBlueNoiseAlgorithm
{
    /** @brief Ulichney 1993 void-and-cluster. Scalar only. */
    VoidAndCluster,

    /** @brief Georgiev-Fajardo 2016 swap/annealing. Required for vector-valued masks. */
    VectorSwap,
};

using FBlueNoiseProgressCallback   = TFunction<void(float /*Fraction*/, const CHAR* /*Phase*/)>;
using FBlueNoiseCheckpointCallback = TFunction<void(const TArray<float>& /*Values*/, int64 /*Iteration*/)>;

struct FBlueNoiseCallbacks
{
    FBlueNoiseProgressCallback   Progress;
    FBlueNoiseCheckpointCallback Checkpoint;
};

struct FBlueNoiseParams
{
    EBlueNoiseAlgorithm Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;

    int32 Width  = 128;
    int32 Height = 128;

    /** @brief Number of temporal slices. 1 is a plain 2D mask; > 1 selects the spatiotemporal energy. */
    int32 Depth = 1;

    /** @brief Values per pixel. Must be 1 for VoidAndCluster. */
    int32 Channels = 1;

    uint32 Seed = 0;

    /** @brief Spatial sigma. Zero selects the documented default for the algorithm and dimensionality. */
    float SigmaSpatial = 0.0f;

    /** @brief Value-space sigma for the swap algorithm. Unused by void-and-cluster. */
    float SigmaValue = 1.0f;

    /** @brief Swap iterations. Zero selects a count scaled to the domain size. */
    int64 NumIterations = 0;

    /** @brief Resume state for the swap algorithm. Empty starts from a fresh stratified shuffle. */
    TArray<float> InitialValues;

    /** @brief Iteration to resume the annealing schedule from, so a resumed run does not reheat. */
    int64 StartIteration = 0;

    /** @brief Swap iterations between checkpoint callbacks. Zero disables checkpointing. */
    int64 CheckpointInterval = 0;

    /** @brief Use the naive O(N*M) energy evaluation instead of the separable incremental update. */
    bool bUseReferenceEnergy = false;

    int64 GetValueCount() const
    {
        return static_cast<int64>(Width) * Height * Depth * Channels;
    }

    int64 GetPixelCount() const
    {
        return static_cast<int64>(Width) * Height * Depth;
    }
};

struct CORE_API FBlueNoiseGenerator
{
    /** @brief Runs whichever algorithm Params selects. */
    static TArray<float> Generate(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks = FBlueNoiseCallbacks());

    /** @brief Ulichney 1993 void-and-cluster. Produces a rank permutation of the domain, returned as rank / N [0, 1). Requires Channels == 1. */
    static TArray<float> GenerateVoidAndCluster(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks = FBlueNoiseCallbacks());

    /** @brief Georgiev-Fajardo 2016 swap algorithm with simulated annealing. */
    static TArray<float> GenerateVectorBlueNoise(const FBlueNoiseParams& Params, FBlueNoiseCallbacks Callbacks = FBlueNoiseCallbacks());

    /** @brief Uniform white noise of the same shape. The negative control for the spectral tests. */
    static TArray<float> GenerateWhiteNoise(const FBlueNoiseParams& Params);

    /** @brief Radially averaged power spectrum of one slice of one channel. */
    static TArray<float> ComputeRadialPowerSpectrum(const TArray<float>& Values, const FBlueNoiseParams& Params, int32 Slice, int32 Channel, int32 NumBins);

    /** @brief Fraction of total spectral energy sitting below LowFrequencyFraction of Nyquist. */
    static float ComputeLowFrequencyEnergy(const TArray<float>& Spectrum, float LowFrequencyFraction);
};
