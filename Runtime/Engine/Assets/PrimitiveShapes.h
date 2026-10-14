#pragma once
#include "Core/Containers/String.h"
#include "Core/Templates/TypeHash.h"
#include "AssetCore/MeshData.h"

enum class EPrimitiveShape : uint8
{
    Cube = 0,
    Sphere,
    Plane,
    Cylinder,
    Capsule,
    Cone,
    Torus,
    Pyramid,
    Teapot,
    Count,
};

enum class EShapeParameterType : uint8
{
    Integer,
    Float,
};

struct FShapeParameter
{
    const CHAR_T*       Name;
    EShapeParameterType Type;
    float               Default;
    float               Min;
    float               Max;

    /** @brief The highest value its slider reaches, from Min; larger values up to Max can still be typed */
    float               SliderMax;

    /** @brief The unit the value is shown with, such as "m", or nullptr for a count */
    const CHAR_T*       Unit;
};

struct FPrimitiveShapeDesc
{
    static constexpr int32 MaxParameters = 4;

    const CHAR_T*   Name;
    int32           NumParameters;
    FShapeParameter Parameters[MaxParameters];
};

struct FPrimitiveShape
{
    NODISCARD bool operator==(const FPrimitiveShape& Other) const
    {
        for (int32 Index = 0; Index < FPrimitiveShapeDesc::MaxParameters; ++Index)
        {
            if (Values[Index] != Other.Values[Index])
            {
                return false;
            }
        }

        return Type == Other.Type;
    }

    NODISCARD bool operator!=(const FPrimitiveShape& Other) const
    {
        return !(*this == Other);
    }

    EPrimitiveShape Type = EPrimitiveShape::Cube;
    float           Values[FPrimitiveShapeDesc::MaxParameters] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct ENGINE_API PrimitiveShapes
{
    static constexpr int32 NumShapes = static_cast<int32>(EPrimitiveShape::Count);

    NODISCARD static const FPrimitiveShapeDesc& GetDesc(EPrimitiveShape Type);

    /** @return Returns the shape whose name is Name, such as "Sphere", or false */
    NODISCARD static bool FindByName(const FString& Name, EPrimitiveShape& OutType);

    /** @return Returns the shape with every parameter at its default */
    NODISCARD static FPrimitiveShape MakeDefault(EPrimitiveShape Type);

    /** @return Returns Shape with every value within its parameter's range, and integer parameters rounded */
    NODISCARD static FPrimitiveShape Clamp(const FPrimitiveShape& Shape);

    /** @return Returns roughly how many triangles the shape builds, to tell a quick build from a slow one */
    NODISCARD static int64 EstimateTriangles(const FPrimitiveShape& Shape);

    /** @brief Build the shape's mesh with MeshFactory; it is a pure function, safe on any thread */
    NODISCARD static FMeshData BuildMeshData(const FPrimitiveShape& Shape);

    /** @return Returns a label for the shape, such as "Sphere, 5 subdivisions, radius 0.5 m" */
    NODISCARD static FString Describe(const FPrimitiveShape& Shape);

    NODISCARD static uint64 GetHash(const FPrimitiveShape& Shape);
};

template<>
struct THash<FPrimitiveShape>
{
    static uint64 GetHash(const FPrimitiveShape& Shape)
    {
        return PrimitiveShapes::GetHash(Shape);
    }
};
