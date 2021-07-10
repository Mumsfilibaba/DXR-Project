#pragma once
#include "MeshFactory.h"

#include "Core/Containers/Array.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Scene/AABB.h"

class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    bool Init( const MeshData& Data );

    bool BuildAccelerationStructure( CommandList& CmdList );

    static TSharedPtr<Mesh> Make( const MeshData& Data );

public:
    void CreateBoundingBox( const MeshData& Data );

    TRef<VertexBuffer>       VertexBuffer;
    TRef<ShaderResourceView> VertexBufferSRV;
    TRef<IndexBuffer>        IndexBuffer;
    TRef<ShaderResourceView> IndexBufferSRV;
    TRef<RayTracingGeometry> RTGeometry;

    uint32 VertexCount = 0;
    uint32 IndexCount = 0;

    AABB BoundingBox;
};