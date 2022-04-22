#include "Mesh.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHICommandList.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMesh

bool CMesh::Init(const SMeshData& Data)
{
    const bool bRTOn = RHISupportsRayTracing();
    
    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount  = static_cast<uint32>(Data.Indices.Size());

    const EBufferUsageFlags BufferFlags = bRTOn ? EBufferUsageFlags::AllowShaderResource | EBufferUsageFlags::Default : EBufferUsageFlags::Default;

    CRHIVertexBufferInitializer VertexBufferInitializer(BufferFlags, VertexCount, sizeof(SVertex), EResourceAccess::VertexAndConstantBuffer);
    VertexBufferInitializer.InitialData = CRHISubresourceInitializer(Data.Vertices.Data(), Data.Vertices.SizeInBytes());
    
    VertexBuffer = RHICreateVertexBuffer(VertexBufferInitializer);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    EIndexFormat IndexFormat = EIndexFormat::uint32;
    
    CRHIIndexBufferInitializer IndexBufferInitializer(BufferFlags, IndexFormat, IndexCount, EResourceAccess::IndexBuffer);
    IndexBufferInitializer.InitialData = CRHISubresourceInitializer(Data.Indices.Data(), Data.Indices.SizeInBytes());

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
        CRHIRayTracingGeometryInitializer RayTracingGeometryInitializer(VertexBuffer, IndexBuffer, ERayTracingStructureBuildFlag::None);
        RTGeometry = RHICreateRayTracingGeometry(RayTracingGeometryInitializer);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        {
            CRHIBufferSRVInitializer BufferSRVInitializer(VertexBuffer.Get(), 0, VertexCount);
            VertexBufferSRV = RHICreateShaderResourceView(BufferSRVInitializer);
            if (!VertexBufferSRV)
            {
                return false;
            }
        }

        {
            CRHIBufferSRVInitializer BufferSRVInitializer(IndexBuffer.Get(), 0, IndexCount);
            IndexBufferSRV = RHICreateShaderResourceView(BufferSRVInitializer);
            if (!IndexBufferSRV)
            {
                return false;
            }
        }
    }

    CreateBoundingBox(Data);
    return true;
}

bool CMesh::BuildAccelerationStructure(CRHICommandList& CmdList)
{
    SBuildRayTracingGeometryInfo Info(VertexBuffer.Get(), IndexBuffer.Get(), ERayTracingStructureBuildType::Build);
    CmdList.BuildRayTracingGeometry(RTGeometry.Get(), Info);
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
