#include "Mesh.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"

FMesh::FMesh()
    : VertexBuffer(nullptr)
    , PosOnlyVertexBuffer(nullptr)
    , VertexBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RTGeometry(nullptr)
    , VertexCount(0)
    , IndexFormat(EIndexFormat::Unknown)
    , IndexCount(0)
    , BoundingBox()
{
}

bool FMesh::Init(const FMeshData& Data)
{
    const bool bRTOn = RHISupportsRayTracing();

    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bRTOn ? EBufferUsageFlags::ShaderResource | EBufferUsageFlags::Default : EBufferUsageFlags::Default;

    FRHIBufferDesc VBDesc(VertexCount * sizeof(FVertex), sizeof(FVertex), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, Data.Vertices.Data());
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    // Create VertexBuffer with only positions
    TArray<FVector3> ShadowVertices(VertexCount);
    for (int32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        ShadowVertices[Index] = Vertex.Position;
    }

    VBDesc = FRHIBufferDesc(VertexCount * sizeof(FVector3), sizeof(FVector3), BufferFlags | EBufferUsageFlags::VertexBuffer);
    PosOnlyVertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, ShadowVertices.Data());
    if (!PosOnlyVertexBuffer)
    {
        return false;
    }
    else
    {
        PosOnlyVertexBuffer->SetName("Position Only VertexBuffer");
    }

    // Create VertexBuffer with only positions and texcoords
    TArray<FVertexMasked> MaskedVertices(VertexCount);
    for (int32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = Data.Vertices[Index];
        MaskedVertices[Index].Position = Vertex.Position;
        MaskedVertices[Index].TexCoord = Vertex.TexCoord;
    }

    VBDesc = FRHIBufferDesc(VertexCount * sizeof(FVertexMasked), sizeof(FVertexMasked), BufferFlags | EBufferUsageFlags::VertexBuffer);
    MaskedVertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, MaskedVertices.Data());
    if (!MaskedVertexBuffer)
    {
        return false;
    }
    else
    {
        MaskedVertexBuffer->SetName("Masked VertexBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;
    
    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = IndexCount < TNumericLimits<uint16>::Max() && !bRTOn ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(Data.Indices.Size());

        for (uint32 Index : Data.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.Data();
    }
    else
    {
        InitialIndicies = Data.Indices.Data();
    }

    FRHIBufferDesc IBDesc(IndexCount * GetStrideFromIndexFormat(IndexFormat), GetStrideFromIndexFormat(IndexFormat), BufferFlags | EBufferUsageFlags::IndexBuffer);
    IndexBuffer = RHICreateBuffer(IBDesc, EResourceAccess::IndexBuffer, InitialIndicies);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName("IndexBuffer");
    }

    if (bRTOn)
    {
        FRHIRayTracingGeometryDesc GeometryInitializer(VertexBuffer.Get(), VertexCount, IndexBuffer.Get(), IndexCount, IndexFormat, EAccelerationStructureBuildFlags::None);
        RTGeometry = RHICreateRayTracingGeometry(GeometryInitializer);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        FRHIBufferSRVDesc SRVInitializer(VertexBuffer.Get(), 0, VertexCount);
        VertexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexBufferSRV)
        {
            return false;
        }

        SRVInitializer = FRHIBufferSRVDesc(IndexBuffer.Get(), 0, IndexCount, EBufferSRVFormat::Uint32);
        IndexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    CreateBoundingBox(Data);
    return true;
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    CommandList.BuildRayTracingGeometry(
        RTGeometry.Get(),
        VertexBuffer.Get(),
        VertexCount,
        IndexBuffer.Get(),
        IndexCount,
        IndexFormat,
        true);

    return true;
}

TSharedPtr<FMesh> FMesh::Create(const FMeshData& Data)
{
    TSharedPtr<FMesh> Result = MakeShared<FMesh>();
    if (Result->Init(Data))
    {
        return Result;
    }
    else
    {
        return TSharedPtr<FMesh>();
    }
}

void FMesh::CreateBoundingBox(const FMeshData& Data)
{
    constexpr float Inf = std::numeric_limits<float>::infinity();

    FVector3 MinBounds = FVector3( Inf,  Inf,  Inf);
    FVector3 MaxBounds = FVector3(-Inf, -Inf, -Inf);

    for (const FVertex& Vertex : Data.Vertices)
    {
        MinBounds = Min(MinBounds, Vertex.Position);
        MaxBounds = Max(MaxBounds, Vertex.Position);
    }

    BoundingBox.Top    = MaxBounds;
    BoundingBox.Bottom = MinBounds;
}
