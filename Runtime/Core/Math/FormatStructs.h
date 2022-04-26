#pragma once
#include "Float.h"
#include "Vector3.h"

#include "Core/Utilities/HashUtilities.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SR10G10B10A2

struct SR10G10B10A2
{
    SR10G10B10A2()
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    { }

    SR10G10B10A2(uint8 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    { }

    SR10G10B10A2(float InR, float InG, float InB)
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    {
        CVector3 Vector(InR, InG, InB);
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

    bool operator==(SR10G10B10A2 RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(SR10G10B10A2 RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint32 A : 2;
    uint32 R : 10;
    uint32 G : 10;
    uint32 B : 10;
};

static_assert(sizeof(SR10G10B10A2) == sizeof(uint32), "SR10G10B10A2 is assumed to have the same size as a uint32");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRG16F

struct SRG16F
{
    SRG16F()
        : R(0)
        , G(0)
    { }

    SRG16F(uint16 InR, uint16 InG)
        : R(InR)
        , G(InG)
    { }

    SRG16F(float InR, float InG)
        : R(SFloat16(InR).Encoded)
        , G(SFloat16(InG).Encoded)
    { }

    uint32 EncodeAsInteger() const { return *reinterpret_cast<const uint32*>(this); }

    uint64 GetHash() const
    {
        THash<uint32> Hasher;
        return static_cast<uint64>(Hasher(EncodeAsInteger()));
    }

    bool operator==(SRG16F RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(SRG16F RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint16 R;
    uint16 G;
};

static_assert(sizeof(SRG16F) == sizeof(uint32), "SRG16F is assumed to have the same size as a uint32");

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRGBA16F

struct SRGBA16F
{
    SRGBA16F()
        : A(0)
        , R(0)
        , G(0)
        , B(0)
    { }

    SRGBA16F(uint16 InA, uint16 InR, uint16 InG, uint16 InB)
        : A(InA)
        , R(InR)
        , G(InG)
        , B(InB)
    { }

    SRGBA16F(float InA, float InR, float InG, float InB)
        : A(SFloat16(InA).Encoded)
        , R(SFloat16(InR).Encoded)
        , G(SFloat16(InG).Encoded)
        , B(SFloat16(InB).Encoded)
    { }

    uint64 EncodeAsInteger() const { return *reinterpret_cast<const uint64*>(this); }

    uint64 GetHash() const
    {
        THash<uint64> Hasher;
        return static_cast<uint64>(Hasher(EncodeAsInteger()));
    }

    bool operator==(SRGBA16F RHS) const
    {
        return EncodeAsInteger() == RHS.EncodeAsInteger();
    }

    bool operator!=(SRGBA16F RHS) const
    {
        return EncodeAsInteger() != RHS.EncodeAsInteger();
    }

    uint16 A;
    uint16 R;
    uint16 G;
    uint16 B;
};

static_assert(sizeof(SRGBA16F) == sizeof(uint64), "SRGBA16F is assumed to have the same size as a uint64");