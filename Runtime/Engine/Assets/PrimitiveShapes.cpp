#include "Core/Math/Math.h"
#include "Engine/Assets/MeshFactory.h"
#include "Engine/Assets/PrimitiveShapes.h"

static constexpr float MinSize = 0.01f;
static constexpr float MaxSize = 1000.0f;

static constexpr FPrimitiveShapeDesc GShapes[PrimitiveShapes::NumShapes] =
{
    { TEXT("Cube"), 3, {
        { TEXT("Width"),  EShapeParameterType::Float, 1.0f, MinSize, MaxSize, 10.0f, TEXT("m") },
        { TEXT("Height"), EShapeParameterType::Float, 1.0f, MinSize, MaxSize, 10.0f, TEXT("m") },
        { TEXT("Depth"),  EShapeParameterType::Float, 1.0f, MinSize, MaxSize, 10.0f, TEXT("m") } } },
    { TEXT("Sphere"), 2, {
        { TEXT("Subdivisions"), EShapeParameterType::Integer, 3.0f, 0.0f, 7.0f, 7.0f, nullptr },
        { TEXT("Radius"),       EShapeParameterType::Float,   0.5f, MinSize, MaxSize, 5.0f, TEXT("m") } } },
    { TEXT("Plane"), 2, {
        { TEXT("Columns"), EShapeParameterType::Integer, 10.0f, 1.0f, 512.0f, 64.0f, nullptr },
        { TEXT("Rows"),    EShapeParameterType::Integer, 10.0f, 1.0f, 512.0f, 64.0f, nullptr } } },
    { TEXT("Cylinder"), 3, {
        { TEXT("Sides"),  EShapeParameterType::Integer, 16.0f, 3.0f, 256.0f, 64.0f, nullptr },
        { TEXT("Radius"), EShapeParameterType::Float,   0.5f, MinSize, MaxSize, 5.0f, TEXT("m") },
        { TEXT("Height"), EShapeParameterType::Float,   2.0f, MinSize, MaxSize, 10.0f, TEXT("m") } } },
    { TEXT("Capsule"), 4, {
        { TEXT("Sides"),  EShapeParameterType::Integer, 32.0f, 3.0f, 256.0f, 64.0f, nullptr },
        { TEXT("Rings"),  EShapeParameterType::Integer, 8.0f, 1.0f, 128.0f, 32.0f, nullptr },
        { TEXT("Radius"), EShapeParameterType::Float,   0.5f, MinSize, MaxSize, 5.0f, TEXT("m") },
        { TEXT("Height"), EShapeParameterType::Float,   2.0f, MinSize, MaxSize, 10.0f, TEXT("m") } } },
    { TEXT("Cone"), 3, {
        { TEXT("Sides"),  EShapeParameterType::Integer, 16.0f, 3.0f, 256.0f, 64.0f, nullptr },
        { TEXT("Radius"), EShapeParameterType::Float,   0.5f, MinSize, MaxSize, 5.0f, TEXT("m") },
        { TEXT("Height"), EShapeParameterType::Float,   1.0f, MinSize, MaxSize, 10.0f, TEXT("m") } } },
    { TEXT("Torus"), 4, {
        { TEXT("Ring radius"),   EShapeParameterType::Float,   1.0f, MinSize, MaxSize, 5.0f, TEXT("m") },
        { TEXT("Tube radius"),   EShapeParameterType::Float,   0.3f, MinSize, MaxSize, 2.0f, TEXT("m") },
        { TEXT("Ring segments"), EShapeParameterType::Integer, 32.0f, 3.0f, 512.0f, 128.0f, nullptr },
        { TEXT("Tube segments"), EShapeParameterType::Integer, 16.0f, 3.0f, 256.0f, 64.0f, nullptr } } },
    { TEXT("Pyramid"), 3, {
        { TEXT("Width"),  EShapeParameterType::Float, 2.0f, MinSize, MaxSize, 10.0f, TEXT("m") },
        { TEXT("Depth"),  EShapeParameterType::Float, 2.0f, MinSize, MaxSize, 10.0f, TEXT("m") },
        { TEXT("Height"), EShapeParameterType::Float, 2.0f, MinSize, MaxSize, 10.0f, TEXT("m") } } },
    { TEXT("Teapot"), 1, {
        { TEXT("Tessellation"), EShapeParameterType::Integer, 10.0f, 1.0f, 64.0f, 32.0f, nullptr } } },
};

static uint32 AsCount(float Value)
{
    return static_cast<uint32>(Value);
}

const FPrimitiveShapeDesc& PrimitiveShapes::GetDesc(EPrimitiveShape Type)
{
    const int32 Index = static_cast<int32>(Type);
    CHECK(Index >= 0 && Index < NumShapes);
    return GShapes[Index];
}

bool PrimitiveShapes::FindByName(const FString& Name, EPrimitiveShape& OutType)
{
    for (int32 Index = 0; Index < NumShapes; ++Index)
    {
        if (Name.Equals(GShapes[Index].Name))
        {
            OutType = static_cast<EPrimitiveShape>(Index);
            return true;
        }
    }

    return false;
}

FPrimitiveShape PrimitiveShapes::MakeDefault(EPrimitiveShape Type)
{
    const FPrimitiveShapeDesc& Desc = GetDesc(Type);

    FPrimitiveShape Shape;
    Shape.Type = Type;

    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        Shape.Values[Index] = Desc.Parameters[Index].Default;
    }

    return Shape;
}

FPrimitiveShape PrimitiveShapes::Clamp(const FPrimitiveShape& Shape)
{
    const FPrimitiveShapeDesc& Desc = GetDesc(Shape.Type);

    FPrimitiveShape Result;
    Result.Type = Shape.Type;

    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        const FShapeParameter& Parameter = Desc.Parameters[Index];

        float Value = Shape.Values[Index] == Shape.Values[Index] ? Shape.Values[Index] : Parameter.Default;
        if (Parameter.Type == EShapeParameterType::Integer)
        {
            Value = Math::Round(Value);
        }

        Result.Values[Index] = Math::Clamp(Value, Parameter.Min, Parameter.Max);
    }

    return Result;
}

int64 PrimitiveShapes::EstimateTriangles(const FPrimitiveShape& Shape)
{
    const float* Values = Shape.Values;
    switch (Shape.Type)
    {
        case EPrimitiveShape::Cube:     return 12;
        case EPrimitiveShape::Sphere:   return int64(20) << (2 * AsCount(Values[0]));
        case EPrimitiveShape::Plane:    return int64(2) * AsCount(Values[0]) * AsCount(Values[1]);
        case EPrimitiveShape::Cylinder: return int64(4) * AsCount(Values[0]);
        case EPrimitiveShape::Capsule:  return int64(4) * AsCount(Values[0]) * (AsCount(Values[1]) + 1);
        case EPrimitiveShape::Cone:     return int64(2) * AsCount(Values[0]);
        case EPrimitiveShape::Torus:    return int64(2) * AsCount(Values[2]) * AsCount(Values[3]);
        case EPrimitiveShape::Pyramid:  return 6;
        case EPrimitiveShape::Teapot:   return int64(64) * AsCount(Values[0]) * AsCount(Values[0]);
        default:                        return 0;
    }
}

FMeshData PrimitiveShapes::BuildMeshData(const FPrimitiveShape& InShape)
{
    const FPrimitiveShape Shape  = Clamp(InShape);
    const float*          Values = Shape.Values;

    switch (Shape.Type)
    {
        case EPrimitiveShape::Cube:     return MeshFactory::CreateCube(Values[0], Values[1], Values[2]);
        case EPrimitiveShape::Sphere:   return MeshFactory::CreateSphere(AsCount(Values[0]), Values[1]);
        case EPrimitiveShape::Plane:    return MeshFactory::CreatePlane(AsCount(Values[0]), AsCount(Values[1]));
        case EPrimitiveShape::Cylinder: return MeshFactory::CreateCylinder(AsCount(Values[0]), Values[1], Values[2]);
        case EPrimitiveShape::Capsule:  return MeshFactory::CreateCapsule(AsCount(Values[0]), AsCount(Values[1]), Values[2], Values[3]);
        case EPrimitiveShape::Cone:     return MeshFactory::CreateCone(AsCount(Values[0]), Values[1], Values[2]);
        case EPrimitiveShape::Torus:    return MeshFactory::CreateTorus(Values[0], Values[1], AsCount(Values[2]), AsCount(Values[3]));
        case EPrimitiveShape::Pyramid:  return MeshFactory::CreatePyramid(Values[0], Values[1], Values[2]);
        case EPrimitiveShape::Teapot:   return MeshFactory::CreateTeapot(AsCount(Values[0]));
        default:                        return FMeshData();
    }
}

FString PrimitiveShapes::Describe(const FPrimitiveShape& Shape)
{
    const FPrimitiveShapeDesc& Desc = GetDesc(Shape.Type);

    FString Result = Desc.Name;
    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        const FShapeParameter& Parameter = Desc.Parameters[Index];

        Result += TEXT(", ");
        Result += FString(Parameter.Name).ToLower();
        Result += Parameter.Type == EShapeParameterType::Integer
            ? FString::Printf(TEXT(" %d"), static_cast<int32>(Shape.Values[Index]))
            : FString::Printf(TEXT(" %.2f %s"), Shape.Values[Index], Parameter.Unit ? Parameter.Unit : TEXT(""));
    }

    return Result;
}

uint64 PrimitiveShapes::GetHash(const FPrimitiveShape& Shape)
{
    uint64 Result = 0;
    HashCombine(Result, static_cast<uint8>(Shape.Type));

    for (const float Value : Shape.Values)
    {
        HashCombine(Result, Value);
    }

    return Result;
}
