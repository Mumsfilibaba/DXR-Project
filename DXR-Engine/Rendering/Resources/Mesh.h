#pragma once
#include "MeshFactory.h"

#include "Core/Containers/Array.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Math/AABB.h"

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

    TSharedRef<VertexBuffer>       VertexBuffer;
    TSharedRef<ShaderResourceView> VertexBufferSRV;
    TSharedRef<IndexBuffer>        IndexBuffer;
    TSharedRef<ShaderResourceView> IndexBufferSRV;
    TSharedRef<RayTracingGeometry> RTGeometry;

    uint32 VertexCount = 0;
    uint32 IndexCount = 0;

    AABB BoundingBox;
};