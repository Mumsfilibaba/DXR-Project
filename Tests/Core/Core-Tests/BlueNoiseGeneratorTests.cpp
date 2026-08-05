#include "BlueNoiseGeneratorTests.h"

#include <Core/Containers/Array.h>
#include <Core/Math/BlueNoiseGenerator.h>

#include "TestCommon/TestMacros.h"

static float ComputeHistogramDeviation(const TArray<float>& Values, int32 Channels, int32 Channel, int32 NumBuckets)
{
    TArray<int32> Buckets;
    Buckets.Resize(NumBuckets);
    Buckets.Fill(0);

    int32 Total = 0;
    for (int32 Index = Channel; Index < Values.Size(); Index += Channels)
    {
        const int32 Bucket = Math::Clamp(static_cast<int32>(Values[Index] * static_cast<float>(NumBuckets)), 0, NumBuckets - 1);
        Buckets[Bucket]++;
        Total++;
    }

    const float Expected = static_cast<float>(Total) / static_cast<float>(NumBuckets);

    float MaxDeviation = 0.0f;
    for (int32 Bucket = 0; Bucket < NumBuckets; ++Bucket)
    {
        MaxDeviation = Math::Max(MaxDeviation, Math::Abs(static_cast<float>(Buckets[Bucket]) - Expected) / Expected);
    }

    return MaxDeviation;
}

bool BlueNoiseGenerator_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Separable energy matches the reference loop");
    {
        FBlueNoiseParams Params;
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Width     = 16;
        Params.Height    = 16;
        Params.Depth     = 4;
        Params.Channels  = 1;
        Params.Seed      = 1234;

        FBlueNoiseParams ReferenceParams = Params;
        ReferenceParams.bUseReferenceEnergy = true;

        const TArray<float> Fast      = FBlueNoiseGenerator::Generate(Params);
        const TArray<float> Reference = FBlueNoiseGenerator::Generate(ReferenceParams);

        TEST_EXPECT_EQ(Fast.Size(), Reference.Size());

        bool bIdentical = (Fast.Size() == Reference.Size());
        for (int32 Index = 0; bIdentical && (Index < Fast.Size()); ++Index)
        {
            bIdentical = (Fast[Index] == Reference[Index]);
        }

        TEST_EXPECT(bIdentical);
    }

    TEST_SECTION("Void-and-cluster produces a complete permutation");
    {
        FBlueNoiseParams Params;
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Width     = 32;
        Params.Height    = 32;
        Params.Depth     = 1;
        Params.Channels  = 1;
        Params.Seed      = 7;

        const TArray<float> Mask = FBlueNoiseGenerator::Generate(Params);
        TEST_EXPECT_EQ(Mask.Size(), 32 * 32);

        TArray<int32> Seen;
        Seen.Resize(Mask.Size());
        Seen.Fill(0);

        for (int32 Index = 0; Index < Mask.Size(); ++Index)
        {
            const int32 Rank = static_cast<int32>(Mask[Index] * static_cast<float>(Mask.Size()) + 0.5f);
            if ((Rank >= 0) && (Rank < Seen.Size()))
            {
                Seen[Rank]++;
            }
        }

        bool bComplete = true;
        for (int32 Rank = 0; Rank < Seen.Size(); ++Rank)
        {
            bComplete = bComplete && (Seen[Rank] == 1);
        }

        TEST_EXPECT(bComplete);
    }

    TEST_SECTION("Void-and-cluster suppresses low frequencies");
    {
        FBlueNoiseParams Params;
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Width     = 32;
        Params.Height    = 32;
        Params.Depth     = 1;
        Params.Channels  = 1;
        Params.Seed      = 99;

        const TArray<float> BlueNoise  = FBlueNoiseGenerator::Generate(Params);
        const TArray<float> WhiteNoise = FBlueNoiseGenerator::GenerateWhiteNoise(Params);

        const TArray<float> BlueSpectrum  = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(BlueNoise, Params, 0, 0, 16);
        const TArray<float> WhiteSpectrum = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(WhiteNoise, Params, 0, 0, 16);

        const float BlueLow  = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(BlueSpectrum, 0.25f);
        const float WhiteLow = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(WhiteSpectrum, 0.25f);

        LOG_INFO("Low-frequency energy fraction: blue=%.4f white=%.4f", BlueLow, WhiteLow);

        // A broken energy kernel or annealing schedule degenerates towards white noise, so requiring
        // a clear margin rather than just "less than" is what makes this diagnostic.
        TEST_EXPECT(BlueLow < (WhiteLow * 0.5f));
    }

    TEST_SECTION("Vector swap suppresses low frequencies in every channel");
    {
        FBlueNoiseParams Params;
        Params.Algorithm     = EBlueNoiseAlgorithm::VectorSwap;
        Params.Width         = 32;
        Params.Height        = 32;
        Params.Depth         = 1;
        Params.Channels      = 2;
        Params.Seed          = 4242;
        Params.NumIterations = 32 * 32 * 64;

        const TArray<float> BlueNoise  = FBlueNoiseGenerator::Generate(Params);
        const TArray<float> WhiteNoise = FBlueNoiseGenerator::GenerateWhiteNoise(Params);

        TEST_EXPECT_EQ(BlueNoise.Size(), 32 * 32 * 2);

        for (int32 Channel = 0; Channel < Params.Channels; ++Channel)
        {
            const TArray<float> BlueSpectrum  = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(BlueNoise, Params, 0, Channel, 16);
            const TArray<float> WhiteSpectrum = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(WhiteNoise, Params, 0, Channel, 16);

            const float BlueLow  = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(BlueSpectrum, 0.25f);
            const float WhiteLow = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(WhiteSpectrum, 0.25f);

            LOG_INFO("Vector channel %d low-frequency energy: blue=%.4f white=%.4f", Channel, BlueLow, WhiteLow);
            TEST_EXPECT(BlueLow < WhiteLow);
        }
    }

    TEST_SECTION("Values stay uniformly distributed");
    {
        FBlueNoiseParams Params;
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Width     = 32;
        Params.Height    = 32;
        Params.Depth     = 1;
        Params.Channels  = 1;
        Params.Seed      = 5;

        TEST_EXPECT(ComputeHistogramDeviation(FBlueNoiseGenerator::Generate(Params), 1, 0, 16) < 0.05f);

        FBlueNoiseParams VectorParams;
        VectorParams.Algorithm     = EBlueNoiseAlgorithm::VectorSwap;
        VectorParams.Width         = 32;
        VectorParams.Height        = 32;
        VectorParams.Depth         = 1;
        VectorParams.Channels      = 2;
        VectorParams.Seed          = 5;
        VectorParams.NumIterations = 32 * 32 * 8;

        const TArray<float> VectorMask = FBlueNoiseGenerator::Generate(VectorParams);
        TEST_EXPECT(ComputeHistogramDeviation(VectorMask, 2, 0, 16) < 0.05f);
        TEST_EXPECT(ComputeHistogramDeviation(VectorMask, 2, 1, 16) < 0.05f);
    }

    TEST_SECTION("Spatiotemporal slices are individually blue-noise");
    {
        FBlueNoiseParams Params;
        Params.Algorithm = EBlueNoiseAlgorithm::VoidAndCluster;
        Params.Width     = 32;
        Params.Height    = 32;
        Params.Depth     = 8;
        Params.Channels  = 1;
        Params.Seed      = 2022;

        const TArray<float> Mask = FBlueNoiseGenerator::Generate(Params);
        TEST_EXPECT_EQ(Mask.Size(), 32 * 32 * 8);

        FBlueNoiseParams WhiteParams = Params;
        const TArray<float> WhiteNoise = FBlueNoiseGenerator::GenerateWhiteNoise(WhiteParams);

        for (int32 Slice = 0; Slice < Params.Depth; ++Slice)
        {
            const TArray<float> BlueSpectrum  = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(Mask, Params, Slice, 0, 16);
            const TArray<float> WhiteSpectrum = FBlueNoiseGenerator::ComputeRadialPowerSpectrum(WhiteNoise, Params, Slice, 0, 16);

            const float BlueLow  = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(BlueSpectrum, 0.25f);
            const float WhiteLow = FBlueNoiseGenerator::ComputeLowFrequencyEnergy(WhiteSpectrum, 0.25f);

            TEST_EXPECT(BlueLow < WhiteLow);
        }
    }

    TEST_END();
}
