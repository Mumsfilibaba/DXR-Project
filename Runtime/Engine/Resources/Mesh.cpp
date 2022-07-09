#include "Mesh.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHICommandList.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMesh

bool FMesh::Init(const FMeshData& Data)
{
    const bool bRTOn = RHISupportsRayTracing();

    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bRTOn ? (EBufferUsageFlags::AllowSRV | EBufferUsageFlags::Default) : EBufferUsageFlags::Default;

    FRHIBufferDataInitializer InitialData(Data.Vertices.Data(), Data.Vertices.SizeInBytes());
    
    FRHIVertexBufferInitializer VBInitializer(BufferFlags, VertexCount, sizeof(FVertex), EResourceAccess::VertexAndConstantBuffer, &InitialData);
    VertexBuffer = RHICreateVertexBuffer(VBInitializer);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    TArray<uint16> NewIndicies;

    const EIndexFormat IndexFormat = (IndexCount < UINT16_MAX) && (!bRTOn) ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(Data.Indices.Size());

        for (uint32 Index : Data.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialData = FRHIBufferDataInitializer(NewIndicies.Data(), NewIndicies.SizeInBytes());
    }
    else
    {
        InitialData = FRHIBufferDataInitializer(Data.Indices.Data(), Data.Indices.SizeInBytes());
    }

    FRHIIndexBufferInitializer IndexBufferInitializer(BufferFlags, IndexFormat, IndexCount, EResourceAccess::IndexBuffer, &InitialData);
    IndexBuffer = RHICreateIndexBuffer(IndexBufferInitializer);
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
        FRHIRayTracingGeometryInitializer GeometryInitializer(VertexBuffer.Get(), IndexBuffer.Get(), EAccelerationStructureBuildFlags::None);
        RTGeometry = RHICreateRayTracingGeometry(GeometryInitializer);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        FRHIBufferSRVInitializer SRVInitializer(VertexBuffer.Get(), 0, VertexCount);
        VertexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexBufferSRV)
        {
            return false;
        }

        SRVInitializer = FRHIBufferSRVInitializer(IndexBuffer.Get(), 0, IndexCount, EBufferSRVFormat::Uint32);
        IndexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    CreateBoundingBox(Data);
    return true;
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CmdList)
{
    CmdList.BuildRayTracingGeometry(RTGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), true);
    return true;
}

TSharedPtr<FMesh> FMesh::Make(const FMeshData& Data)
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
