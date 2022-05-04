#include "Mesh.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHICommandList.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMesh

bool CMesh::Init(const SMeshData& Data)
{
    const bool bRTOn = RHISupportsRayTracing();

    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bRTOn ? (EBufferUsageFlags::AllowSRV | EBufferUsageFlags::Default) : EBufferUsageFlags::Default;

    CRHIBufferDataInitializer InitialData(Data.Vertices.Data(), Data.Vertices.SizeInBytes());
    
    CRHIVertexBufferInitializer VBInitializer(BufferFlags, VertexCount, sizeof(SVertex), EResourceAccess::VertexAndConstantBuffer, &InitialData);
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

        InitialData = CRHIBufferDataInitializer(NewIndicies.Data(), NewIndicies.SizeInBytes());
    }
    else
    {
        InitialData = CRHIBufferDataInitializer(Data.Indices.Data(), Data.Indices.SizeInBytes());
    }

    CRHIIndexBufferInitializer IndexBufferInitializer(BufferFlags, IndexFormat, IndexCount, EResourceAccess::IndexBuffer, &InitialData);
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
        CRHIRayTracingGeometryInitializer GeometryInitializer(VertexBuffer.Get(), IndexBuffer.Get(), EAccelerationStructureBuildFlags::None);
        RTGeometry = RHICreateRayTracingGeometry(GeometryInitializer);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        CRHIBufferSRVInitializer SRVInitializer(VertexBuffer.Get(), 0, VertexCount);
        VertexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!VertexBufferSRV)
        {
            return false;
        }

        SRVInitializer = CRHIBufferSRVInitializer(IndexBuffer.Get(), 0, IndexCount, EBufferSRVFormat::Uint32);
        IndexBufferSRV = RHICreateShaderResourceView(SRVInitializer);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    CreateBoundingBox(Data);
    return true;
}

bool CMesh::BuildAccelerationStructure(CRHICommandList& CmdList)
{
    CmdList.BuildRayTracingGeometry(RTGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), true);
    return true;
}

TSharedPtr<CMesh> CMesh::Make(const SMeshData& Data)
{
    TSharedPtr<CMesh> Result = MakeShared<CMesh>();
    if (Result->Init(Data))
    {
        return Result;
    }
    else
    {
        return TSharedPtr<CMesh>();
    }
}

void CMesh::CreateBoundingBox(const SMeshData& Data)
{
    constexpr float Inf = std::numeric_limits<float>::infinity();

    CVector3 MinBounds = CVector3( Inf,  Inf,  Inf);
    CVector3 MaxBounds = CVector3(-Inf, -Inf, -Inf);

    for (const SVertex& Vertex : Data.Vertices)
    {
        MinBounds = Min(MinBounds, Vertex.Position);
        MaxBounds = Max(MaxBounds, Vertex.Position);
    }

    BoundingBox.Top    = MaxBounds;
    BoundingBox.Bottom = MinBounds;
}
