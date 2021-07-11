#include "Mesh.h"

#include "RenderLayer/CommandList.h"
#include "RenderLayer/RenderLayer.h"

bool Mesh::Init( const MeshData& Data )
{
    VertexCount = static_cast<uint32>(Data.Vertices.Size());
    IndexCount = static_cast<uint32>(Data.Indices.Size());

    const uint32 BufferFlags = IsRayTracingSupported() ? BufferFlag_SRV | BufferFlag_Default : BufferFlag_Default;

    ResourceData InitialData( Data.Vertices.Data(), Data.Vertices.SizeInBytes() );
    VertexBuffer = CreateVertexBuffer<Vertex>( VertexCount, BufferFlags, EResourceState::VertexAndConstantBuffer, &InitialData );
    if ( !VertexBuffer )
    {
        return false;
    }
    else
    {
        VertexBuffer->SetName( "VertexBuffer" );
    }

    const bool RTOn = IsRayTracingSupported();

    InitialData = ResourceData( Data.Indices.Data(), Data.Indices.SizeInBytes() );
    IndexBuffer = CreateIndexBuffer( EIndexFormat::uint32, IndexCount, BufferFlags, EResourceState::IndexBuffer, &InitialData );
    if ( !IndexBuffer )
    {
        return false;
    }
    else
    {
        IndexBuffer->SetName( "IndexBuffer" );
    }

    if ( RTOn )
    {
        RTGeometry = CreateRayTracingGeometry( RayTracingStructureBuildFlag_None, VertexBuffer.Get(), IndexBuffer.Get() );
        if ( !RTGeometry )
        {
            return false;
        }
        else
        {
            RTGeometry->SetName( "RayTracing Geometry" );
        }

        VertexBufferSRV = CreateShaderResourceView( VertexBuffer.Get(), 0, VertexCount );
        if ( !VertexBufferSRV )
        {
            return false;
        }

        IndexBufferSRV = CreateShaderResourceView( IndexBuffer.Get(), 0, IndexCount );
        if ( !IndexBufferSRV )
        {
            return false;
        }
    }

    CreateBoundingBox( Data );
    return true;
}

bool Mesh::BuildAccelerationStructure( CommandList& CmdList )
{
    CmdList.BuildRayTracingGeometry( RTGeometry.Get(), VertexBuffer.Get(), IndexBuffer.Get(), true );
    return true;
}

TSharedPtr<Mesh> Mesh::Make( const MeshData& Data )
{
    TSharedPtr<Mesh> Result = MakeShared<Mesh>();
    if ( Result->Init( Data ) )
    {
        return Result;
    }
    else
    {
        return TSharedPtr<Mesh>( nullptr );
    }
}

void Mesh::CreateBoundingBox( const MeshData& Data )
{
    constexpr float Inf = std::numeric_limits<float>::infinity();
    XMFLOAT3 Min = XMFLOAT3( Inf, Inf, Inf );
    XMFLOAT3 Max = XMFLOAT3( -Inf, -Inf, -Inf );

    for ( const Vertex& Vertex : Data.Vertices )
    {
        // X
        Min.x = NMath::Min<float>( Min.x, Vertex.Position.x );
        Max.x = NMath::Max<float>( Max.x, Vertex.Position.x );
        // Y
        Min.y = NMath::Min<float>( Min.y, Vertex.Position.y );
        Max.y = NMath::Max<float>( Max.y, Vertex.Position.y );
        // Z
        Min.z = NMath::Min<float>( Min.z, Vertex.Position.z );
        Max.z = NMath::Max<float>( Max.z, Vertex.Position.z );
    }

    BoundingBox.Top = Max;
    BoundingBox.Bottom = Min;
}
