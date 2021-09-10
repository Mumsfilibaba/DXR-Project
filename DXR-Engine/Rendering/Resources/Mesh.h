#pragma once
#include "Assets/MeshFactory.h"

#include "Core/Containers/Array.h"

#include "RenderLayer/Resources.h"
#include "RenderLayer/CommandList.h"

#include "Core/Math/AABB.h"

class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    bool Init( const SMeshData& Data );

    bool BuildAccelerationStructure( CommandList& CmdList );

    static TSharedPtr<Mesh> Make( const SMeshData& Data );

public:
    void CreateBoundingBox( const SMeshData& Data );

    TSharedRef<VertexBuffer>       VertexBuffer;
    TSharedRef<ShaderResourceView> VertexBufferSRV;
    TSharedRef<IndexBuffer>        IndexBuffer;
    TSharedRef<ShaderResourceView> IndexBufferSRV;
    TSharedRef<RayTracingGeometry> RTGeometry;

    uint32 VertexCount = 0;
    uint32 IndexCount = 0;

    AABB BoundingBox;
};