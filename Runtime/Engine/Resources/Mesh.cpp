#include "Mesh.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHICommandList.h"

bool FMesh::Init(const FMeshData& Data)
{
    const bool bRTOn = RHISupportsRayTracing();

    VertexCount = static_cast<uint32>(Data.Vertices.GetSize());
    IndexCount  = static_cast<uint32>(Data.Indices.GetSize());

    const EBufferUsageFlags BufferFlags = bRTOn ? (EBufferUsageFlags::ShaderResource | EBufferUsageFlags::Default) : EBufferUsageFlags::Default;

    FRHIBufferDesc VBDesc(
        VertexCount * sizeof(FVertex),
        sizeof(FVertex),
        BufferFlags | EBufferUsageFlags::VertexBuffer);

    VertexBuffer = RHICreateBuffer(VBDesc, EResourceAccess::VertexAndConstantBuffer, Data.Vertices.GetData());
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;
    
    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = ((IndexCount < TNumericLimits<uint16>::Max()) && !bRTOn) ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(Data.Indices.GetSize());

        for (uint32 Index : Data.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.GetData();
    }
    else
    {
        InitialIndicies = Data.Indices.GetData();
    }

    FRHIBufferDesc IBDesc(
        IndexCount * GetStrideFromIndexFormat(IndexFormat),
        GetStrideFromIndexFormat(IndexFormat),
        BufferFlags | EBufferUsageFlags::IndexBuffer);

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
        FRHIRayTracingGeometryDesc GeometryInitializer(
            VertexBuffer.Get(),
            VertexCount,
            IndexBuffer.Get(),
            IndexCount,
            IndexFormat,
            EAccelerationStructureBuildFlags::None);

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
