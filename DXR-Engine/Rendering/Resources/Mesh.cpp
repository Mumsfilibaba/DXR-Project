#include "Mesh.h"

#include "RenderLayer/CommandList.h"
#include "RenderLayer/RenderLayer.h"

Bool Mesh::Init(const MeshData& Data)
{
    VertexCount = static_cast<UInt32>(Data.Vertices.Size());
    IndexCount  = static_cast<UInt32>(Data.Indices.Size());

    const UInt32 BufferFlags = IsRayTracingSupported() ? BufferFlag_SRV | BufferFlag_Default : BufferFlag_Default;

    ResourceData InitialData(Data.Vertices.Data(), Data.Vertices.SizeInBytes());
    VertexBuffer = CreateVertexBuffer<Vertex>(VertexCount, BufferFlags, EResourceState::VertexAndConstantBuffer, &InitialData);
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName("VertexBuffer");
    }

    TArray<UInt16> SmallIndicies;
    EIndexFormat Format = EIndexFormat::UInt32;
    if (VertexCount < UINT16_MAX)
    {
        Format = EIndexFormat::UInt16;
        SmallIndicies.Resize(Data.Indices.Size());
        for (UInt32 i = 0; i < IndexCount; i++)
        {
            SmallIndicies[i] = (UInt16)Data.Indices[i];
        }

        InitialData = ResourceData(SmallIndicies.Data(), SmallIndicies.SizeInBytes());
    }
    else
    {
        InitialData = ResourceData(Data.Indices.Data(), Data.Indices.SizeInBytes());
    }

    IndexBuffer = CreateIndexBuffer(Format, IndexCount, BufferFlags, EResourceState::IndexBuffer, &InitialData);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName("IndexBuffer");
    }

    if (IsRayTracingSupported())
    {
        RTGeometry = CreateRayTracingGeometry(RayTracingStructureBuildFlag_None, VertexBuffer.Get(), IndexBuffer.Get());
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetName("RayTracing Geometry");
        }

        VertexBufferSRV = CreateShaderResourceView(VertexBuffer.Get(), 0, VertexCount);
        if (!VertexBufferSRV)
        {
            return false;
        }

        IndexBufferSRV = CreateShaderResourceView(IndexBuffer.Get(), 0, IndexCount);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    CreateBoundingBox(Data);
    return true;
}

Bool Mesh::BuildAccelerationStructure(CommandList& CmdList)
{
    CmdList.BuildRayTracingGeometry(RTGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), true);
    return true;
}

TSharedPtr<Mesh> Mesh::Make(const MeshData& Data)
{
    TSharedPtr<Mesh> Result = MakeShared<Mesh>();
    if (Result->Init(Data))
    {
        return Result;
    }
    else
    {
        return TSharedPtr<Mesh>(nullptr);
    }
}

void Mesh::CreateBoundingBox(const MeshData& Data)
{
    constexpr Float Inf = std::numeric_limits<Float>::infinity();
    XMFLOAT3 Min = XMFLOAT3(Inf, Inf, Inf);
    XMFLOAT3 Max = XMFLOAT3(-Inf, -Inf, -Inf);

    for (const Vertex& Vertex : Data.Vertices)
    {
        // X
        Min.x = Math::Min<Float>(Min.x, Vertex.Position.x);
        Max.x = Math::Max<Float>(Max.x, Vertex.Position.x);
        // Y
        Min.y = Math::Min<Float>(Min.y, Vertex.Position.y);
        Max.y = Math::Max<Float>(Max.y, Vertex.Position.y);
        // Z
        Min.z = Math::Min<Float>(Min.z, Vertex.Position.z);
        Max.z = Math::Max<Float>(Max.z, Vertex.Position.z);
    }

    BoundingBox.Top    = Max;
    BoundingBox.Bottom = Min;
}
