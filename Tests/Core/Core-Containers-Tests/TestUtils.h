#pragma once
#include <initializer_list>

#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/Containers/String.h>
#include <Core/Templates/CString.h>
#include <Core/Templates/Utility.h>
#include <Core/Templates/TypeHash.h>
#include <Core/Math/Random.h>

#include "TestCommon/TestMacros.h"

// ------------------------------------------------------------------------------------------------
// Helper macros for tests (reporting routed through the engine logger)
// ------------------------------------------------------------------------------------------------

#define MAKE_STRING(...) #__VA_ARGS__
#define MAKE_VAR(...) __VA_ARGS__

// TEST_CHECK is provided by TestCommon/TestMacros.h

#define TEST_CHECK_ARRAY(Array, ...) \
    if (Array.IsEmpty()) \
    { \
        LOG_ERROR("[TEST FAILED] '%s' is empty (%s:%d)", MAKE_STRING(Array), __FILE__, __LINE__); \
        return false; \
    } \
    else \
    { \
        std::initializer_list InitList = MAKE_VAR(__VA_ARGS__); \
 \
        int32 Index = 0; \
        for (auto Element : InitList) \
        { \
            if ((Index >= static_cast<int32>(Array.Size())) || (Array[Index] != Element)) \
            { \
                LOG_ERROR("[TEST FAILED] '%s[%d]' mismatch (%s:%d)", MAKE_STRING(Array), \
                    Index, __FILE__, __LINE__); \
                return false; \
            } \
 \
            Index++; \
        } \
    }

#define TEST_CHECK_STRING_N(String, Value, NumChars) \
    { \
        const bool bResult = (String.Length() == NumChars) && \
            TCString<decltype(String)::CharType>::Strncmp(String.Data(), Value, NumChars) == 0; \
        if (!bResult) \
        { \
            LOG_ERROR("[TEST FAILED] String '%s' mismatch (%s:%d)", #String, __FILE__, __LINE__); \
            return false; \
        } \
    }

#define TEST_CHECK_STRING(String, Value) TEST_CHECK_STRING_N(String, Value, TCString<decltype(String)::CharType>::Strlen(Value))

#define SUCCESS() \
    return true

struct FInstancedCounters
{
    int32 Live       = 0;
    int32 Ctor       = 0;
    int32 ValueCtor  = 0;
    int32 CopyCtor   = 0;
    int32 MoveCtor   = 0;
    int32 CopyAssign = 0;
    int32 MoveAssign = 0;
    int32 Dtor       = 0;
};

class FInstanced
{
public:
    static FInstancedCounters& Counters()
    {
        static FInstancedCounters GCounters;
        return GCounters;
    }

    static void  Reset()
    {
        Counters() = FInstancedCounters();
    }

    static int32 LiveCount()
    {
        return Counters().Live;
    }

public:
    FInstanced()
        : Id(0)
        , Payload("0")
    {
        Counters().Live++;
        Counters().Ctor++;
    }

    explicit FInstanced(int32 InId)
        : Id(InId)
        , Payload(String::CreateFormatted("%d", InId))
    {
        Counters().Live++;
        Counters().ValueCtor++;
    }

    FInstanced(const FInstanced& Other)
        : Id(Other.Id)
        , Payload(Other.Payload)
    {
        Counters().Live++;
        Counters().CopyCtor++;
    }

    FInstanced(FInstanced&& Other) noexcept
        : Id(Other.Id)
        , Payload(::Move(Other.Payload))
    {
        Other.Id = -1;
        Counters().Live++;
        Counters().MoveCtor++;
    }

    FInstanced& operator=(const FInstanced& Other)
    {
        Id      = Other.Id;
        Payload = Other.Payload;
        Counters().CopyAssign++;
        return *this;
    }

    FInstanced& operator=(FInstanced&& Other) noexcept
    {
        Id           = Other.Id;
        Payload      = ::Move(Other.Payload);
        Other.Id     = -1;
        Counters().MoveAssign++;
        return *this;
    }

    ~FInstanced()
    {
        Counters().Live--;
        Counters().Dtor++;
    }

    int32 GetId() const
    {
        return Id;
    }

    // The payload must always mirror the id; a torn copy/move breaks this invariant.
    bool IsPayloadValid() const
    {
        return Payload == String::CreateFormatted("%d", Id);
    }

    bool operator==(const FInstanced& Other) const
    {
        return Id == Other.Id;
    }

    bool operator!=(const FInstanced& Other) const
    {
        return Id != Other.Id;
    }

    bool operator<(const FInstanced& Other) const
    {
        return Id < Other.Id;
    }

private:
    int32  Id;
    String Payload;
};

template<>
struct THash<FInstanced>
{
    static uint64 GetHash(const FInstanced& Value)
    {
        return static_cast<uint64>(Value.GetId());
    }
};

// ------------------------------------------------------------------------------------------------
// Reallocation stress sweep.
// ------------------------------------------------------------------------------------------------

namespace Stress
{
    inline constexpr int32 SweepSizesArray[] =
    {
        0, 1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33,
        63, 64, 65, 100, 101, 104, 127, 128, 129,
        255, 256, 257, 511, 512, 513, 1000, 1024, 1025, 2049
    };

    inline constexpr int32 SweepSizesCount = static_cast<int32>(sizeof(SweepSizesArray) / sizeof(SweepSizesArray[0]));

    // Number of randomized passes per swept size. Scaled down for slow Debug + leak-checked builds.
    inline constexpr uint32 DefaultSeedCount =
    #if defined(DEBUG_BUILD)
        3u;
    #else
        8u;
    #endif
}

#define STRESS_SWEEP(SizeVar, SeedVar, NumSeeds) \
    for (int32 _StressIdx = 0; _StressIdx < Stress::SweepSizesCount; ++_StressIdx) \
        for (uint32 SeedVar = 1u; SeedVar <= static_cast<uint32>(NumSeeds); ++SeedVar) \
            for (int32 SizeVar = Stress::SweepSizesArray[_StressIdx], _StressOnce = 0; \
                 _StressOnce < 1; ++_StressOnce)
