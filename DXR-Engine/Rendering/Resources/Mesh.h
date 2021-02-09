#pragma once
#include "MeshFactory.h"

#include <Containers/SharedPtr.h>

#include "RenderLayer/Resources.h"
#include "RenderLayer/RayTracing.h"
#include "RenderLayer/CommandList.h"

#include "Scene/AABB.h"

class Mesh
{
public:
    Mesh()  = default;
    ~Mesh() = default;

    Bool Init(const MeshData& Data);
    
    Bool BuildAccelerationStructure(CommandList& CmdList);

    static TSharedPtr<Mesh> Make(const MeshData& Data);

public:
    void CreateBoundingBox(const MeshData& Data);

    TSharedRef<VertexBuffer>       VertexBuffer;
    TSharedRef<ShaderResourceView> VertexBufferSRV;
    TSharedRef<IndexBuffer>        IndexBuffer;
    TSharedRef<ShaderResourceView> IndexBufferSRV;
    TSharedRef<RayTracingGeometry> RTGeometry;
    
    UInt32 VertexCount = 0;
    UInt32 IndexCount  = 0;

    Float ShadowOffset = 0.0f;

    AABB BoundingBox;
};