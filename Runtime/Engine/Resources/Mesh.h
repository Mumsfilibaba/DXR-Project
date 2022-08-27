#pragma once
#include "Engine/EngineModule.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"

#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

#include "Engine/Assets/MeshFactory.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FMesh

class ENGINE_API FMesh
{
public:
    FMesh() = default;
    ~FMesh() = default;

    bool Init(const FMeshData& Data);

    bool BuildAccelerationStructure(FRHICommandList& CommandList);

    static TSharedPtr<FMesh> Make(const FMeshData& Data);

public:
    void CreateBoundingBox(const FMeshData& Data);

    FRHIVertexBufferRef       VertexBuffer;
    FRHIShaderResourceViewRef VertexBufferSRV;
    FRHIIndexBufferRef        IndexBuffer;
    FRHIShaderResourceViewRef IndexBufferSRV;
    TSharedRef<FRHIRayTracingGeometry> RTGeometry;

    uint32 VertexCount = 0;
    uint32 IndexCount = 0;

    FAABB BoundingBox;
};