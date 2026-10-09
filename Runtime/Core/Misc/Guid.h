#pragma once
#include "Core/Containers/String.h"
#include "Core/Templates/TypeHash.h"

struct CORE_API FGuid
{
    /** @return Returns a new random GUID */
    NODISCARD static FGuid Create();

    /**
     * @brief Parse 32 hex digits, either packed or in the dashed 8-4-4-4-12 form, in upper or lower case
     * @return Returns false and leaves OutGuid untouched if Text is not a GUID
     */
    static bool Parse(const String& Text, FGuid& OutGuid);

public:
    constexpr FGuid() = default;

    constexpr FGuid(uint32 InA, uint32 InB, uint32 InC, uint32 InD)
        : A(InA)
        , B(InB)
        , C(InC)
        , D(InD)
    {
    }

    /** @return Returns the GUID as 32 lowercase hex digits without dashes, e.g. "6f9619ff8b86d011b42d00cf4fc964ff" */
    NODISCARD String ToString() const;

    NODISCARD constexpr bool IsValid() const
    {
        return (A | B | C | D) != 0;
    }

    void Invalidate()
    {
        A = B = C = D = 0;
    }

    NODISCARD constexpr bool operator==(const FGuid& Other) const
    {
        return (A == Other.A) && (B == Other.B) && (C == Other.C) && (D == Other.D);
    }

    NODISCARD constexpr bool operator!=(const FGuid& Other) const
    {
        return !(*this == Other);
    }

    /** @brief Orders GUIDs the same way as their strings, which keeps sorted lists of them stable on disk */
    NODISCARD constexpr bool operator<(const FGuid& Other) const
    {
        if (A != Other.A)
        {
            return A < Other.A;
        }

        if (B != Other.B)
        {
            return B < Other.B;
        }

        if (C != Other.C)
        {
            return C < Other.C;
        }

        return D < Other.D;
    }

    /** @brief The value as four big-endian words, so A holds the first eight hex digits of the string */
    uint32 A = 0;
    uint32 B = 0;
    uint32 C = 0;
    uint32 D = 0;
};

template<>
struct THash<FGuid>
{
    static uint64 GetHash(const FGuid& Value)
    {
        uint64 Result = 0;
        HashCombine(Result, Value.A);
        HashCombine(Result, Value.B);
        HashCombine(Result, Value.C);
        HashCombine(Result, Value.D);
        return Result;
    }
};
