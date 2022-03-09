#include "Mesh.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHICommandList.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMesh

bool CMesh::Init(const SMeshData& Data)
{
    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount = static_cast<uint32>(Data.Indices.Size());

    const uint32 BufferFlags = RHISupportsRayTracing() ? BufferFlag_ShaderResource | BufferFlag_Default : BufferFlag_Default;

    SRHIResourceData InitialData(Data.Vertices.Data(), Data.Vertices.SizeInBytes());
    VertexBuffer = RHICreateVertexBuffer<SVertex>(VertexCount, BufferFlags, ERHIResourceState::VertexAndConstantBuffer, &InitialData);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    const bool bRTOn = RHISupportsRayTracing();

    InitialData = SRHIResourceData(Data.Indices.Data(), Data.Indices.SizeInBytes());
    IndexBuffer = RHICreateIndexBuffer(ERHIIndexFormat::uint32, IndexCount, BufferFlags, ERHIResourceState::IndexBuffer, &InitialData);
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
        RTGeometry = RHICreateRayTracingGeometry(RayTracingStructureBuildFlag_None, VertexBuffer.Get(), IndexBuffer.Get());
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        VertexBufferSRV = RHICreateShaderResourceView(VertexBuffer.Get(), 0, VertexCount);
        if (!VertexBufferSRV)
        {
            return false;
        }

        IndexBufferSRV = RHICreateShaderResourceView(IndexBuffer.Get(), 0, IndexCount);
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
    CVector3 MinBounds = CVector3(Inf, Inf, Inf);
    CVector3 MaxBounds = CVector3(-Inf, -Inf, -Inf);

    for (const SVertex& Vertex : Data.Vertices)
    {
        MinBounds = Min(MinBounds, Vertex.Position);
        MaxBounds = Max(MaxBounds, Vertex.Position);
    }

    BoundingBox.Top = MaxBounds;
    BoundingBox.Bottom = MinBounds;
}
