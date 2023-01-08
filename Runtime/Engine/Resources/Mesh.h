#pragma once
#include "Engine/EngineModule.h"
#include "Engine/Assets/MeshFactory.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Math/AABB.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"

class ENGINE_API FMesh
{
public:
    FMesh()  = default;
    ~FMesh() = default;

    bool Init(const FMeshData& Data);

    bool BuildAccelerationStructure(FRHICommandList& CommandList);

    static TSharedPtr<FMesh> Create(const FMeshData& Data);

public:
    void CreateBoundingBox(const FMeshData& Data);

    FRHIBufferRef             VertexBuffer;
    FRHIShaderResourceViewRef VertexBufferSRV;

    FRHIBufferRef             IndexBuffer;
    FRHIShaderResourceViewRef IndexBufferSRV;
    
    FRHIRayTracingGeometryRef RTGeometry;

    uint32       VertexCount = 0;
    
    EIndexFormat IndexFormat;
    uint32       IndexCount  = 0;

    FAABB        BoundingBox;
};