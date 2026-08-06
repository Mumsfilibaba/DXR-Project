#include "Core/Templates/NumericLimits.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "Engine/Resources/Model.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureCompressor.h"
#include "RendererCore/VertexStreamCache.h"

FMesh::FMesh()
    : Declaration()
    , VertexStreams()
    , AttributeBufferSRV(nullptr)
    , IndexBuffer(nullptr)
    , IndexBufferSRV(nullptr)
    , RayTracingGeometry(nullptr)
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

bool FMesh::Init(const FMeshCreateInfo& CreateInfo, bool bCreateRayTracingResources)
{
    const bool bEnableRayTracing = RHI::bSupportsRayTracing;

    Declaration = CreateInfo.Declaration;
    VertexCount = CreateInfo.PackedVertexCount > 0 ? CreateInfo.PackedVertexCount : CreateInfo.Vertices.Size();
    IndexCount  = CreateInfo.Indices.Size();

    if (!CreateVertexStreams(CreateInfo))
    {
        return false;
    }


    const EBufferFlags BufferFlags = bEnableRayTracing ? EBufferFlags::ShaderResourceBuffer | EBufferFlags::Default : EBufferFlags::Default;

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

	FRHIBufferDesc IndexBufferDesc;
    IndexBufferDesc.Stride = GetStrideFromIndexFormat(IndexFormat);
    IndexBufferDesc.Size   = IndexCount * IndexBufferDesc.Stride;
    IndexBufferDesc.Flags  = BufferFlags | EBufferFlags::IndexBuffer;

    IndexBuffer = RHI::CreateBuffer(IndexBufferDesc, ERHIResourceState::IndexBuffer, InitialIndicies);
    if (!IndexBuffer)
    {
        return false;
    }
    else
    {
        IndexBuffer->SetDebugName("IndexBuffer");
    }

    if (bCreateRayTracingResources)
    {
        if (!EnsureRayTracingResources())
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

bool FMesh::CreateVertexStreams(const FMeshCreateInfo& CreateInfo)
{
    static const CHAR* StreamDebugNames[VERTEX_MAX_STREAMS] =
    {
        "PositionStream",
        "AttributeStream",
        "ColorStream",
        "VertexStream3"
    };

    const EBufferFlags AttributeFlags = RHI::bSupportsRayTracing ? EBufferFlags::ShaderResourceBuffer : EBufferFlags::None;

    TArray<uint8> PackedStreams[VERTEX_MAX_STREAMS];
    const bool    bUsePrePacked = CreateInfo.PackedVertexCount > 0;

    if (!bUsePrePacked && !CreateInfo.PackVertexStreams(PackedStreams))
    {
        return false;
    }

    for (uint8 StreamIndex = 0; StreamIndex < Declaration.GetNumStreams(); StreamIndex++)
    {
        const uint16 Stride = Declaration.GetStreamStride(StreamIndex);
        if (Stride == 0)
        {
            continue;
        }

        const TArray<uint8>& StreamData   = bUsePrePacked ? CreateInfo.PackedStreams[StreamIndex] : PackedStreams[StreamIndex];
        const int32          ExpectedSize = VertexCount * Stride;

        if (StreamData.Size() != ExpectedSize)
        {
            LOG_ERROR("Mesh '%s' carries %d bytes for stream %u but its declaration needs %d",
                CreateInfo.Name.Data(), StreamData.Size(), uint32(StreamIndex), ExpectedSize);
            return false;
        }

        FRHIBufferDesc StreamDesc;
        StreamDesc.Stride = Stride;
        StreamDesc.Size   = StreamData.SizeInBytes();
        StreamDesc.Flags  = EBufferFlags::VertexBuffer | EBufferFlags::Default;

        if (StreamIndex == EVertexStreamIndex::Attributes)
        {
            StreamDesc.Flags |= AttributeFlags;
        }

        VertexStreams[StreamIndex] = RHI::CreateBuffer(StreamDesc, ERHIResourceState::VertexBuffer, StreamData.Data());
        if (!VertexStreams[StreamIndex])
        {
            return false;
        }

        VertexStreams[StreamIndex]->SetDebugName(StreamDebugNames[StreamIndex]);
    }

    return true;
}

void FMesh::SetVertexBuffers(FRHICommandList& CommandList, const FVertexStreamBinding& Binding) const
{
    FRHIBuffer* Buffers[VERTEX_MAX_STREAMS];
    for (uint8 Index = 0; Index < Binding.NumStreams; Index++)
    {
        Buffers[Index] = VertexStreams[Binding.StreamIndices[Index]].Get();
        CHECK(Buffers[Index] != nullptr);
    }

    CommandList.SetVertexBuffers(MakeArrayView(Buffers, Binding.NumStreams), 0);
}

bool FMesh::EnsureRayTracingResources()
{
    if (RayTracingGeometry)
    {
        return true;
    }

    if (!RHI::bSupportsRayTracing)
    {
        return false;
    }

    FRHIBuffer* AttributeBuffer = GetAttributeBuffer();
    if (!AttributeBuffer || !IndexBuffer || IndexFormat != EIndexFormat::uint32)
    {
        LOG_ERROR("Mesh '%s' was not created ray-tracing-ready and cannot be ray traced", MeshName.Data());
        CHECK(false);
        return false;
    }

    if (!AttributeBufferSRV)
    {
        AttributeBufferSRV = RHI::CreateShaderResourceView(AttributeBuffer, FRHIShaderResourceViewDesc::CreateBuffer(0, VertexCount));
        if (!AttributeBufferSRV)
        {
            return false;
        }
    }

    if (!IndexBufferSRV)
    {
        IndexBufferSRV = RHI::CreateShaderResourceView(IndexBuffer.Get(), FRHIShaderResourceViewDesc::CreateBuffer(0, IndexCount, EBufferViewType::ByteAddress));
        if (!IndexBufferSRV)
        {
            return false;
        }
    }

    FRHIGeometryAccelerationStructureDesc GeometryDesc(GetPositionBuffer(), VertexCount, IndexBuffer.Get(), IndexCount, IndexFormat, EAccelerationStructureBuildFlags::AllowCompaction);
    RayTracingGeometry = RHI::CreateGeometryAccelerationStructure(GeometryDesc);
    if (!RayTracingGeometry)
    {
        return false;
    }

    RayTracingGeometry->SetDebugName("RayTracing Geometry");
    return true;
}

void FMesh::ReleaseRayTracingResources()
{
    RayTracingGeometry.Reset();
    AttributeBufferSRV.Reset();
    IndexBufferSRV.Reset();
}

bool FMesh::BuildAccelerationStructure(FRHICommandList& CommandList)
{
    if (!RayTracingGeometry)
    {
        return false;
    }

    FRHIGeometryAccelerationStructureBuildDesc BuildDesc;
    BuildDesc.VertexBuffer = GetPositionBuffer();
    BuildDesc.NumVertices  = VertexCount;
    BuildDesc.IndexBuffer  = IndexBuffer.Get();
    BuildDesc.NumIndices   = IndexCount;
    BuildDesc.IndexFormat  = IndexFormat;
    BuildDesc.bUpdate      = true;

    CommandList.BuildGeometryAccelerationStructure(RayTracingGeometry.Get(), BuildDesc);
    return true;
}

void FMesh::CreateBoundingBox(const FMeshCreateInfo& CreateInfo)
{
    static constexpr const float Inf = TNumericLimits<float>::Infinity();

    Vector3 MinBounds = Vector3( Inf,  Inf,  Inf);
    Vector3 MaxBounds = Vector3(-Inf, -Inf, -Inf);

    if (CreateInfo.PackedVertexCount > 0)
    {
        const TArray<uint8>& PositionStream = CreateInfo.PackedStreams[EVertexStreamIndex::Position];
        const uint16         Stride         = Declaration.GetStreamStride(EVertexStreamIndex::Position);

        for (int32 Index = 0; Index < CreateInfo.PackedVertexCount; Index++)
        {
            const Vector3& Position = *reinterpret_cast<const Vector3*>(PositionStream.Data() + (Index * Stride));
            MinBounds = Vector3::Min(MinBounds, Position);
            MaxBounds = Vector3::Max(MaxBounds, Position);
        }
    }
    else
    {
        for (const FSourceVertex& Vertex : CreateInfo.Vertices)
        {
            MinBounds = Vector3::Min(MinBounds, Vertex.Position);
            MaxBounds = Vector3::Max(MaxBounds, Vertex.Position);
        }
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
    
    const auto GetRHITexture = [=](const FModelCreateInfo& ModelCreateInfo, EMaterialTexture::Type MaterialTexture, int32 MaterialIndex)
    {
        const FTexture2DRef Texture = ModelCreateInfo.Materials[MaterialIndex].Textures[MaterialTexture];
        return Texture ? Texture->GetRHITexture() : FEngine::Get()->BaseTexture;
    };
    
    for (int32 Index = 0; Index < NumMaterials; Index++)
    {
        FMaterialInfo MaterialInfo;
        MaterialInfo.Albedo           = FFloatColor(CreateInfo.Materials[Index].Diffuse);
        MaterialInfo.AmbientOcclusion = CreateInfo.Materials[Index].AmbientFactor;
        MaterialInfo.Metallic         = CreateInfo.Materials[Index].Metallic;
        MaterialInfo.Roughness        = CreateInfo.Materials[Index].Roughness;
        MaterialInfo.MaterialFlags    = CreateInfo.Materials[Index].MaterialFlags & (EMaterialFlags::EnableHeight | EMaterialFlags::EnableAlpha | EMaterialFlags::EnableNormalMapping | EMaterialFlags::DoubleSided | EMaterialFlags::ForceForwardPass);
        
        if (CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            MaterialInfo.MaterialFlags |= EMaterialFlags::EnableNormalMapping;
        }

        TSharedPtr<FMaterial> Material = MakeSharedPtr<FMaterial>(MaterialInfo);
        Material->AlbedoMap = GetRHITexture(CreateInfo, EMaterialTexture::Diffuse, Index);

        // If a separate AlphaMask texture exists, bake it into AlbedoMap.a
        const FTexture2DRef& AlphaMaskTex = CreateInfo.Materials[Index].Textures[EMaterialTexture::AlphaMask];
        if (AlphaMaskTex)
        {
            FRHITextureRef AlbedoWithAlpha;
            if (FTextureFactory::Get().BakeAlphaIntoAlbedo(Material->AlbedoMap, AlphaMaskTex->GetRHITexture(), AlbedoWithAlpha))
            {
                Material->AlbedoMap = AlbedoWithAlpha;
                Material->EnableAlphaMask(true);
                LOG_INFO("[FModel] Baked separate alpha mask into AlbedoMap.a for material '%s'", *CreateInfo.Materials[Index].Name);
            }
        }

        if (CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal])
        {
            Material->NormalMap = CreateInfo.Materials[Index].Textures[EMaterialTexture::Normal]->GetRHITexture();
        }

        const FTexture2DRef& SpecularTex  = CreateInfo.Materials[Index].Textures[EMaterialTexture::Specular];
        const FTexture2DRef& RoughnessTex = CreateInfo.Materials[Index].Textures[EMaterialTexture::Roughness];
        const FTexture2DRef& MetallicTex  = CreateInfo.Materials[Index].Textures[EMaterialTexture::Metallic];
        const FTexture2DRef& AOTex        = CreateInfo.Materials[Index].Textures[EMaterialTexture::AmbientOcclusion];

        if (SpecularTex)
        {
            Material->MaterialMap = SpecularTex->GetRHITexture();
        }
        else if (RoughnessTex || MetallicTex || AOTex)
        {
            FRHITextureRef DefaultWhite   = FEngine::Get()->BaseTexture;
            FRHITextureRef AOInput        = AOTex ? AOTex->GetRHITexture() : DefaultWhite;
            FRHITextureRef RoughnessInput = RoughnessTex ? RoughnessTex->GetRHITexture() : DefaultWhite;
            FRHITextureRef MetallicInput  = MetallicTex ? MetallicTex->GetRHITexture() : DefaultWhite;

            FRHITextureRef PackedMaterial;
            if (FTextureFactory::Get().PackMaterialParamsTexture(AOInput, RoughnessInput, MetallicInput, PackedMaterial))
            {
                Material->MaterialMap = PackedMaterial;
                LOG_INFO("[FModel] Packed separate AO/Roughness/Metallic into MaterialMap for material '%s'", *CreateInfo.Materials[Index].Name);
            }
            else
            {
                Material->MaterialMap = FEngine::Get()->BaseTexture;
            }
        }
        else
        {
            Material->MaterialMap = FEngine::Get()->BaseTexture;
        }

        // Block-compress uncompressed textures for reduced memory usage
        FTextureCompressor& Compressor = FTextureFactory::Get().GetTextureCompressor();

        auto TryCompressBC1 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC1(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC1", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        auto TryCompressBC3 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC3(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC3", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        auto TryCompressBC5 = [&](FRHITextureRef& Texture)
        {
            if (Texture && !IsBlockCompressed(Texture->GetDesc().Format) && IsBlockCompressedAligned(Texture->GetDesc().Extent.X) && IsBlockCompressedAligned(Texture->GetDesc().Extent.Y))
            {
                FRHITextureRef Compressed;
                if (Compressor.CompressBC5(Texture, Compressed))
                {
                    LOG_INFO("[FModel] Compressed texture (%dx%d) %s -> BC5", Texture->GetDesc().Extent.X, Texture->GetDesc().Extent.Y, ToString(Texture->GetDesc().Format));
                    Texture = Compressed;
                }
            }
        };

        if (Material->HasAlphaMask())
        {
            TryCompressBC3(Material->AlbedoMap);
        }
        else
        {
            TryCompressBC1(Material->AlbedoMap);
        }
        
        TryCompressBC5(Material->NormalMap);
        TryCompressBC1(Material->MaterialMap);

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

            FStaticMeshComponent* MeshComponent = NewObject<FStaticMeshComponent>();
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
                            MeshComponent->SetMaterial(FEngine::Get()->BaseMaterial, SubMeshIdx);
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
                        MeshComponent->SetMaterial(FEngine::Get()->BaseMaterial);
                    }
                }
                
                NewActor->AddComponent(MeshComponent);
            }
        }
    }
}
