#pragma once
#include "Float.h"
#include "Vector3.h"

#include "Core/Utilities/HashUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FR10G10B10A2

struct FR10G10B10A2
{
    FR10G10B10A2()
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    { }

    FR10G10B10A2(uint8 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    { }

    FR10G10B10A2(float InR, float InG, float InB)
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    {
        FVector3 Vector(InR, InG, InB);
        Vector.Normalize();

        R = uint32(NMath::Round(Vector.x * float(NMath::MaxNum<10>())));
        G = uint32(NMath::Round(Vector.y * float(NMath::MaxNum<10>())));
        B = uint32(NMath::Round(Vector.z * float(NMath::MaxNum<10>())));
    }

    uint32 EncodeAsInteger() const { return *reinterpret_cast<const uint32*>(this); }

    uint64 GetHash() const 
    {
        THash<uint32> Hasher;
        return static_cast<uint64>(Hasher(EncodeAsInteger()));
    }

    bool operator==(FR10G10B10A2 RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(FR10G10B10A2 RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint32 A : 2;
    uint32 R : 10;
    uint32 G : 10;
    uint32 B : 10;
};

static_assert(sizeof(FR10G10B10A2) == sizeof(uint32), "FR10G10B10A2 is assumed to have the same size as a uint32");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRG16F

struct FRG16F
{
    FRG16F()
        : R(0)
        , G(0)
    { }

    FRG16F(uint16 InR, uint16 InG)
        : R(InR)
        , G(InG)
    { }

    FRG16F(float InR, float InG)
        : R(FFloat16(InR).Encoded)
        , G(FFloat16(InG).Encoded)
    { }

    uint32 EncodeAsInteger() const { return *reinterpret_cast<const uint32*>(this); }

    uint64 GetHash() const
    {
        THash<uint32> Hasher;
        return static_cast<uint64>(Hasher(EncodeAsInteger()));
    }

    bool operator==(FRG16F RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(FRG16F RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint16 R;
    uint16 G;
};

static_assert(sizeof(FRG16F) == sizeof(uint32), "FRG16F is assumed to have the same size as a uint32");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRGBA16F

struct FRGBA16F
{
    FRGBA16F()
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    { }

    FRGBA16F(uint16 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    { }

    FRGBA16F(float InA, float InR, float InG, float InB)
        : A(FFloat16(InA).Encoded)
        , R(FFloat16(InR).Encoded)
        , G(FFloat16(InG).Encoded)
        , B(FFloat16(InB).Encoded)
    { }

    uint64 EncodeAsInteger() const { return *reinterpret_cast<const uint64*>(this); }

    uint64 GetHash() const
    {
        THash<uint64> Hasher;
        return static_cast<uint64>(Hasher(EncodeAsInteger()));
    }

    bool operator==(FRGBA16F RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(FRGBA16F RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint16 A;
    uint16 R;
    uint16 G;
    uint16 B;
};

static_assert(sizeof(FRGBA16F) == sizeof(uint64), "FRGBA16F is assumed to have the same size as a uint64");