#pragma once
#include "Core/Json/JsonSerializer.h"
#include "Core/Math/Color.h"
#include "Core/Math/Matrix3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

// Every math type serializes as a flat array of floats rather than an {"x": ..} object, because the
// writer keeps scalar arrays on one line, so a transform is one line in the file and moving an actor
// is a one-line diff.
template<int32 NUM_COMPONENTS>
struct TJsonComponentArray
{
    static void Save(FJsonValue& OutValue, const float* Components)
    {
        OutValue = FJsonValue::MakeArray();
        for (int32 Index = 0; Index < NUM_COMPONENTS; ++Index)
        {
            OutValue.Add(FJsonValue(Components[Index]));
        }
    }

    static bool Load(const FJsonValue& InValue, float* Components, FJsonArchive& Archive)
    {
        if (!InValue.IsArray())
        {
            Archive.AddTypeError(InValue, "an array");
            return false;
        }

        if (InValue.Num() != NUM_COMPONENTS)
        {
            Archive.AddError("expected %d components, found %d", NUM_COMPONENTS, InValue.Num());
            return false;
        }

        for (int32 Index = 0; Index < NUM_COMPONENTS; ++Index)
        {
            double Component = 0.0;
            if (!InValue[Index].TryGetDouble(Component))
            {
                Archive.AddError("component %d is %s rather than a number", Index, FJsonArchive::GetTypeName(InValue[Index].GetType()));
                return false;
            }

            Components[Index] = static_cast<float>(Component);
        }

        return true;
    }
};

template<>
struct TJsonSerializer<Vector2>
{
    static void Save(FJsonValue& OutValue, const Vector2& InValue)
    {
        TJsonComponentArray<2>::Save(OutValue, InValue.XY);
    }

    static bool Load(const FJsonValue& InValue, Vector2& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<2>::Load(InValue, OutValue.XY, Archive);
    }
};

template<>
struct TJsonSerializer<Vector3>
{
    static void Save(FJsonValue& OutValue, const Vector3& InValue)
    {
        TJsonComponentArray<3>::Save(OutValue, InValue.XYZ);
    }

    static bool Load(const FJsonValue& InValue, Vector3& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<3>::Load(InValue, OutValue.XYZ, Archive);
    }
};

template<>
struct TJsonSerializer<Vector4>
{
    static void Save(FJsonValue& OutValue, const Vector4& InValue)
    {
        TJsonComponentArray<4>::Save(OutValue, InValue.XYZW);
    }

    static bool Load(const FJsonValue& InValue, Vector4& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<4>::Load(InValue, OutValue.XYZW, Archive);
    }
};

template<>
struct TJsonSerializer<Quaternion>
{
    static void Save(FJsonValue& OutValue, const Quaternion& InValue)
    {
        TJsonComponentArray<4>::Save(OutValue, InValue.XYZW);
    }

    static bool Load(const FJsonValue& InValue, Quaternion& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<4>::Load(InValue, OutValue.XYZW, Archive);
    }
};

template<>
struct TJsonSerializer<Matrix3>
{
    static void Save(FJsonValue& OutValue, const Matrix3& InValue)
    {
        TJsonComponentArray<9>::Save(OutValue, &InValue.M[0][0]);
    }

    static bool Load(const FJsonValue& InValue, Matrix3& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<9>::Load(InValue, &OutValue.M[0][0], Archive);
    }
};

template<>
struct TJsonSerializer<Matrix3x4>
{
    static void Save(FJsonValue& OutValue, const Matrix3x4& InValue)
    {
        TJsonComponentArray<12>::Save(OutValue, &InValue.M[0][0]);
    }

    static bool Load(const FJsonValue& InValue, Matrix3x4& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<12>::Load(InValue, &OutValue.M[0][0], Archive);
    }
};

template<>
struct TJsonSerializer<Matrix4>
{
    static void Save(FJsonValue& OutValue, const Matrix4& InValue)
    {
        TJsonComponentArray<16>::Save(OutValue, &InValue.M[0][0]);
    }

    static bool Load(const FJsonValue& InValue, Matrix4& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<16>::Load(InValue, &OutValue.M[0][0], Archive);
    }
};

// Written as [r, g, b, a] with the channels still 0-255, so the file matches the type
template<>
struct TJsonSerializer<FColor>
{
    static void Save(FJsonValue& OutValue, const FColor& InValue)
    {
        OutValue = FJsonValue::MakeArray();
        OutValue.Add(FJsonValue(static_cast<int64>(InValue.R)));
        OutValue.Add(FJsonValue(static_cast<int64>(InValue.G)));
        OutValue.Add(FJsonValue(static_cast<int64>(InValue.B)));
        OutValue.Add(FJsonValue(static_cast<int64>(InValue.A)));
    }

    static bool Load(const FJsonValue& InValue, FColor& OutValue, FJsonArchive& Archive)
    {
        if (!InValue.IsArray())
        {
            Archive.AddTypeError(InValue, "an array");
            return false;
        }

        if (InValue.Num() != 4)
        {
            Archive.AddError("expected 4 channels, found %d", InValue.Num());
            return false;
        }

        uint8* Channels[] = { &OutValue.R, &OutValue.G, &OutValue.B, &OutValue.A };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            int64 Channel = 0;
            if (!InValue[Index].TryGetInt64(Channel))
            {
                Archive.AddError("channel %d is %s rather than a number", Index, FJsonArchive::GetTypeName(InValue[Index].GetType()));
                return false;
            }

            if ((Channel < 0) || (Channel > 255))
            {
                Archive.AddError("channel %d is %lld, which is outside 0 to 255", Index, Channel);
                return false;
            }

            *Channels[Index] = static_cast<uint8>(Channel);
        }

        return true;
    }
};

template<>
struct TJsonSerializer<FFloatColor>
{
    static void Save(FJsonValue& OutValue, const FFloatColor& InValue)
    {
        TJsonComponentArray<4>::Save(OutValue, InValue.RGBA);
    }

    static bool Load(const FJsonValue& InValue, FFloatColor& OutValue, FJsonArchive& Archive)
    {
        return TJsonComponentArray<4>::Load(InValue, OutValue.RGBA, Archive);
    }
};
