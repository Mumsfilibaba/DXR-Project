#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Resources/Model.h"

FMesh::FMesh()
    : VertexBuffer(nullptr)
    , VertexBufferSRV(nullptr)
    , VertexPositionBuffer(nullptr)
    , VertexPositionBufferSRV(nullptr)
    , VertexNormalBuffer(nullptr)
    , VertexNormalBufferSRV(nullptr)
    , VertexTexCoordBuffer(nullptr)
    , VertexTexCoordBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RTGeometry(nullptr)
    , IndexFormat(EIndexFormat::Unknown)
    , IndexCount()
    , VertexCount(0)
    , BoundingBox()
    , SubMeshes()
{
}

FMesh::~FMesh()
{
}

bool FMesh::Init(const FMeshCreateInfo& CreateInfo)
{
    const bool bEnableRayTracing = false; //FRHIDeviceInfo::SupportsRayTracing ;

    VertexCount = CreateInfo.Vertices.Size();
    IndexCount  = CreateInfo.Indices.Size();

    const EBufferUsageFlags BufferFlags = bEnableRayTracing ? EBufferUsageFlags::ShaderResource | EBufferUsageFlags::Default : EBufferUsageFlags::Default;

    // Create VertexBuffer
    FRHIBufferInfo VBInfo(VertexCount * sizeof(FVertex), sizeof(FVertex), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, CreateInfo.Vertices.Data());
    if (!VertexBuffer)
    {
        return false;
    }
    else
    {
        VertexBuffer->SetDebugName("VertexBuffer");
    }

    // Create VertexPositionBuffer
    TArray<FVertexPosition> VertexPositions(VertexCount);
    for (int32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = CreateInfo.Vertices[Index];
        VertexPositions[Index] = Vertex.Position;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexPosition), sizeof(FVertexPosition), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexPositionBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexPositions.Data());
    if (!VertexPositionBuffer)
    {
        return false;
    }
    else
    {
        VertexPositionBuffer->SetDebugName("VertexPositionBuffer");
    }

    // Create VertexNormalBuffer
    TArray<FVertexNormal> VertexNormals(VertexCount);
    for (int32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = CreateInfo.Vertices[Index];
        VertexNormals[Index] = FVertexNormal(Vertex.Normal, Vertex.Tangent);
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexNormal), sizeof(FVertexNormal), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexNormalBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexNormals.Data());
    if (!VertexNormalBuffer)
    {
        return false;
    }
    else
    {
        VertexNormalBuffer->SetDebugName("VertexNormalBuffer");
    }
    
    // Create VertexTexCoordBuffer
    TArray<FVertexTexCoord> VertexTexCoords(VertexCount);
    for (int32 Index = 0; Index < VertexCount; Index++)
    {
        const FVertex& Vertex = CreateInfo.Vertices[Index];
        VertexTexCoords[Index] = Vertex.TexCoord;
    }

    VBInfo = FRHIBufferInfo(VertexCount * sizeof(FVertexTexCoord), sizeof(FVertexTexCoord), BufferFlags | EBufferUsageFlags::VertexBuffer);
    VertexTexCoordBuffer = RHICreateBuffer(VBInfo, EResourceAccess::VertexBuffer, VertexTexCoords.Data());
    if (!VertexTexCoordBuffer)
    {
        return false;
    }
    else
    {
        VertexTexCoordBuffer->SetDebugName("VertexTexCoordBuffer");
    }

    // If we can get away with 16-bit indices, store them in this array
    TArray<uint16> NewIndicies;

    // Initial data
    const void* InitialIndicies = nullptr;

    IndexFormat = (IndexCount < TNumericLimits<uint16>::Max()) && !bEnableRayTracing ? EIndexFormat::uint16 : EIndexFormat::uint32;
    if (IndexFormat == EIndexFormat::uint16)
    {
        NewIndicies.Reserve(CreateInfo.Indices.Size());

        for (uint32 Index : CreateInfo.Indices)
        {
            NewIndicies.Emplace(uint16(Index));
        }

        InitialIndicies = NewIndicies.Data();
    }
    else
    {
        InitialIndicies = CreateInfo.Indices.Data();
    }

    FRHIBufferInfo IBInfo(IndexCount * GetStrideFromIndexFormat(IndexFormat), GetStrideFromIndexFormat(IndexFormat), BufferFlags | EBufferUsageFlags::IndexBuffer);
    IndexBuffer = RHICreateBuffer(IBInfo, EResourceAccess::IndexBuffer, InitialIndicies);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetDebugName("IndexBuffer");
    }

    if (bEnableRayTracing)
    {
        FRHIRayTracingGeometryInfo GeometryInfo(VertexBuffer.Get(), VertexCount, IndexBuffer.Get(), IndexCount, IndexFormat, EAccelerationStructureBuildFlags::None);
        RTGeometry = RHICreateRayTracingGeometry(GeometryInfo);
        if (!RTGeometry)
        {
            return false;
        }
        else
        {
            RTGeometry->SetDebugName("RayTracing Geometry");
        }

        FRHIBufferSRVInfo SRVInfo(VertexBuffer.Get(), 0, VertexCount);
        VertexBufferSRV = RHICreateShaderResourceView(SRVInfo);
        if (!VertexBufferSRV)
        {
            return false;
        }
        
        SRVInfo = FRHIBufferSRVInfo(VertexPositionBuffer.Get(), 0, VertexCount);
        VertexPositionBufferSRV = RHICreateShaderResourceView(SRVInfo);
        if (!VertexPositionBufferSRV)
        {
            return false;
        }
        
        SRVInfo = FRHIBufferSRVInfo(VertexNormalBuffer.Get(), 0, VertexCount);
        VertexNormalBufferSRV = RHICreateShaderResourceView(SRVInfo);
        if (!VertexNormalBufferSRV)
        {
            return false;
        }
        
        SRVInfo = FRHIBufferSRVInfo(VertexTexCoordBuffer.Get(), 0, VertexCount);
        VertexTexCoordBufferSRV = RHICreateShaderResourceView(SRVInfo);
        if (!VertexTexCoordBufferSRV)
        {
            return false;
        }

        SRVInfo = FRHIBufferSRVInfo(IndexBuffer.Get(), 0, IndexCount, EBufferSRVFormat::UInt32);
        IndexBufferSRV = RHICreateShaderResourceView(SRVInfo);
        if (!IndexBufferSRV)
        {
            return false;
        }
    }
    
    // Add submeshes
    if (!CreateInfo.SubMeshes.IsEmpty())
    {
        SubMeshes.Reserve(CreateInfo.SubMeshes.Size());
        
        for (const FSubMeshInfo& MeshPartition : CreateInfo.SubMeshes)
        {
            FSubMesh NewSubMesh;
            NewSubMesh.BaseVertex    = MeshPartition.BaseVertex;
            NewSubMesh.VertexCount   = MeshPartition.VertexCount;
            NewSubMesh.StartIndex    = MeshPartition.StartIndex;
            NewSubMesh.IndexCount    = MeshPartition.IndexCount;
            NewSubMesh.MaterialIndex = MeshPartition.MaterialIndex;
            AddSubMesh(NewSubMesh);
        }
    }
    else
    {
        SubMeshes.Reserve(1);
        
        FSubMesh NewSubMesh;
        NewSubMesh.BaseVertex  = 0;
        NewSubMesh.VertexCount = VertexCount;
        NewSubMesh.StartIndex  = 0;
        NewSubMesh.IndexCount  = IndexCount;
        
        AddSubMesh(NewSubMesh);
    }

    MeshName = CreateInfo.Name;
    CreateBoundingBox(CreateInfo);
    return true;
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    FRayTracingGeometryBuildInfo BuildInfo;
    BuildInfo.VertexBuffer = VertexBuffer.Get();
    BuildInfo.NumVertices  = VertexCount;
    BuildInfo.IndexBuffer  = IndexBuffer.Get();
    BuildInfo.NumIndices   = IndexCount;
    BuildInfo.IndexFormat  = IndexFormat;
    BuildInfo.bUpdate      = true;

    CommandList.BuildRayTracingGeometry(RTGeometry.Get(), BuildInfo);
    return true;
}

FRHIBuffer* FMesh::GetVertexBuffer(EVertexStream VertexStream) const
{
    switch (VertexStream)
    {
        case EVertexStream::Packed:    return VertexBuffer.Get();
        case EVertexStream::Positions: return VertexPositionBuffer.Get();
        case EVertexStream::Normals:   return VertexNormalBuffer.Get();
        case EVertexStream::TexCoords: return VertexTexCoordBuffer.Get();
        default:                       return nullptr;
    }
}

FRHIShaderResourceView* FMesh::GetVertexBufferSRV(EVertexStream VertexStream) const
{
    switch (VertexStream)
    {
        case EVertexStream::Packed:    return VertexBufferSRV.Get();
        case EVertexStream::Positions: return VertexPositionBufferSRV.Get();
        case EVertexStream::Normals:   return VertexNormalBufferSRV.Get();
        case EVertexStream::TexCoords: return VertexTexCoordBufferSRV.Get();
        default:                       return nullptr;
    }
}

void FMesh::CreateBoundingBox(const FMeshCreateInfo& CreateInfo)
{
    constexpr float Inf = TNumericLimits<float>::Infinity();

    FVector3 MinBounds = FVector3( Inf,  Inf,  Inf);
    FVector3 MaxBounds = FVector3(-Inf, -Inf, -Inf);

    for (const FVertex& Vertex : CreateInfo.Vertices)
    {
        MinBounds = FVector3::Min(MinBounds, Vertex.Position);
        MaxBounds = FVector3::Max(MaxBounds, Vertex.Position);
    }

    BoundingBox.Max = MaxBounds;
    BoundingBox.Min = MinBounds;
}

FModel::FModel()
    : Meshes()
    , Materials()
    , UniformScale(1.0f)
    , BoundingBox()
{
}

FModel::~FModel()
{
}

bool FModel::Init(const FModelCreateInfo& CreateInfo)
{
    UniformScale = CreateInfo.Scale;
    
    const int32 NumMeshes = CreateInfo.Meshes.Size();
    Meshes.Reserve(NumMeshes);
    
    for (int32 Index = 0; Index < NumMeshes; Index++)
    {
        TSharedPtr<FMesh> Mesh = MakeSharedPtr<FMesh>();
        if (!Mesh->Init(CreateInfo.Meshes[Index]))
        {
            return false;
        }
        
        Meshes.Add(Mesh);
    }
    
    if (Meshes.Size() != CreateInfo.Meshes.Size())
    {
        DEBUG_BREAK();
        return false;
    }
    
    const int32 NumMaterials = CreateInfo.Materials.Size();
    Materials.Reserve(NumMaterials);
    
    const auto RetrieveRHITexture = [=](const FModelCreateInfo& ModelCreateInfo, EMaterialTexture::Type MaterialTexture, int32 MaterialIndex)
    {
        const FTexture2DRef Texture = ModelCreateInfo.Materials[MaterialIndex].Textures[MaterialTexture];
        return Texture ? Texture->GetRHITexture() : GEngine->BaseTexture;
    };
    
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        FMaterialInfo MaterialInfo;
        MaterialInfo.Albedo           = FFloatColor(CreateInfo.Materials[Index].Diffuse);
        MaterialInfo.AmbientOcclusion = CreateInfo.Materials[Index].AmbientFactor;
        MaterialInfo.Metallic         = CreateInfo.Materials[Index].Metallic;
        MaterialInfo.Roughness        = CreateInfo.Materials[Index].Roughness;
        MaterialInfo.MaterialFlags    = CreateInfo.Materials[Index].MaterialFlags;
        
        if (CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            MaterialInfo.MaterialFlags |= EMaterialFlags::EnableNormalMapping;
        }

        TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(MaterialInfo);
        Material->AlbedoMap    = RetrieveRHITexture(CreateInfo, EMaterialTexture::Diffuse, Index);
        Material->AOMap        = RetrieveRHITexture(CreateInfo, EMaterialTexture::AmbientOcclusion, Index);
        Material->SpecularMap  = RetrieveRHITexture(CreateInfo, EMaterialTexture::Specular, Index);
        Material->MetallicMap  = RetrieveRHITexture(CreateInfo, EMaterialTexture::Metallic, Index);
        Material->RoughnessMap = RetrieveRHITexture(CreateInfo, EMaterialTexture::Roughness, Index);
        Material->AlphaMask    = RetrieveRHITexture(CreateInfo, EMaterialTexture::AlphaMask, Index);

        if (CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            Material->NormalMap = CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal]->GetRHITexture();
        }

        Material->Initialize();
        Material->SetName(CreateInfo.Materials[Index].Name);
        
        Materials.Add(Material);
    }
    
    if (Materials.Size() != CreateInfo.Materials.Size())
    {
        DEBUG_BREAK();
        return false;
    }
    
    return true;
}

bool FModel::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    for (const TSharedPtr<FMesh>& Mesh : Meshes)
    {
        if (!Mesh->BuildAccelerationStructure(CommandList))
        {
            return false;
        }
    }
    
    return true;
}

void FModel::AddToWorld(FWorld* World)
{
    const int32 NumMeshes = Meshes.Size();
    for (int32 MeshIdx = 0; MeshIdx < NumMeshes; MeshIdx++)
    {
        const TSharedPtr<FMesh>& Mesh = Meshes[MeshIdx];
        if (Mesh)
        {
            FActor* NewActor = World->CreateActor();
            NewActor->SetName(Mesh->GetName());
            NewActor->GetTransform().SetUniformScale(UniformScale);

            FMeshComponent* MeshComponent = NewObject<FMeshComponent>();
            if (MeshComponent)
            {
                MeshComponent->SetMesh(Mesh);

                const int32 NumSubMeshes = Mesh->GetNumSubMeshes();
                if (NumSubMeshes > 0)
                {
                    for (int32 SubMeshIdx = 0; SubMeshIdx < NumSubMeshes; SubMeshIdx++)
                    {
                        const FSubMesh& SubMesh = Mesh->GetSubMesh(SubMeshIdx);
                        if (SubMesh.MaterialIndex >= 0 && Materials.Size() >= SubMesh.MaterialIndex)
                        {
                            const TSharedPtr<FMaterial>& Material = Materials[SubMesh.MaterialIndex];
                            MeshComponent->SetMaterial(Material, SubMeshIdx);
                            CHECK(MeshComponent->GetMaterial(SubMeshIdx) == Material);
                        }
                        else
                        {
                            MeshComponent->SetMaterial(GEngine->BaseMaterial, SubMeshIdx);
                        }
                    }
                }
                else
                {
                    if (!Materials.IsEmpty())
                    {
                        MeshComponent->SetMaterial(Materials[0]);
                    }
                    else
                    {
                        MeshComponent->SetMaterial(GEngine->BaseMaterial);
                    }
                }
                
                NewActor->AddComponent(MeshComponent);
            }
        }
    }
}
