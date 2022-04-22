#pragma once
#include "Engine/EngineModule.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"

#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CMesh

class ENGINE_API CMesh
{
public:
    CMesh()  = default;
    ~CMesh() = default;

    bool Init(const SMeshData& Data);

    bool BuildAccelerationStructure(CRHICommandList& CmdList);

    static TSharedPtr<CMesh> Make(const SMeshData& Data);

public:
    void CreateBoundingBox(const SMeshData& Data);

    CRHIVertexBufferRef       VertexBuffer;
    CRHIShaderResourceViewRef VertexBufferSRV;
    CRHIIndexBufferRef        IndexBuffer;
    CRHIShaderResourceViewRef IndexBufferSRV;
    CRHIRayTracingGeometryRef RTGeometry;

    uint32 VertexCount = 0;
    uint32 IndexCount  = 0;

    SAABB BoundingBox;
};