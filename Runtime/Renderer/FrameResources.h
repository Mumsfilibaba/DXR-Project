#pragma once
#include "RendererModule.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Math/Vector4.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIRayTracing.h"
#include "Engine/World/World.h"
#include "Engine/World/Components/ProxySceneComponent.h"
#include "Engine/World/Lights/DirectionalLight.h"

#define MAX_LIGHTS_PER_TILE (1024)
#define NUM_DEFAULT_SHADOW_CASTING_POINT_LIGHTS (8)

enum EGBufferIndex
{
    GBufferIndex_Albedo     = 0,
    GBufferIndex_Normal     = 1,
    GBufferIndex_Material   = 2,
    GBufferIndex_Depth      = 3,
    GBufferIndex_ViewNormal = 4,
    GBufferIndex_Velocity   = 5,

    GBuffer_NumBuffers
};

template<typename TResource>
class TResourceCache
{
public:
    int32 Add(TResource* Resource)
    {
        if (Resource == nullptr)
        {
            return -1;
        }

        if (int32* TextureIndex = ResourceIndices.Find(Resource))
        {
            return *TextureIndex;
        }
        else
        {
            int32 NewIndex = Resources.Size();
            ResourceIndices[Resource] = NewIndex;
            Resources.Emplace(Resource);
            return NewIndex;
        }
    }

    TResource* Get(uint32 Index) const
    {
        return Resources[Index];
    }

    uint32 Size() const
    {
        return Resources.Size();
    }

private:
    TArray<TResource*>      Resources;
    TMap<TResource*, int32> ResourceIndices;
};


struct FPointLightDataHLSL
{
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    Padding0;
};

MARK_AS_REALLOCATABLE(FPointLightDataHLSL);

struct FShadowCastingPointLightDataHLSL
{
    // 0-16
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;
    // 16-24
    float    FarPlane = 10.0f;
    float    MaxShadowBias = 0.05f;
    // 24-32
    float    Padding0;
    float    Padding1;
};

MARK_AS_REALLOCATABLE(FShadowCastingPointLightDataHLSL);

struct FDirectionalLightDataHLSL
{
    // 0-16
    FVector3 Color = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;
    // 16-32
    FVector3 Direction = FVector3(0.0f, -1.0f, 0.0f);
    float    MaxShadowBias = 0.05f;
    // 32-48
    FVector3 UpVector = FVector3(0.0f, 0.0f, -1.0f);
    float    LightSize = 0.0f;
    // 48-112
    FMatrix4 ShadowMatrix;
};

MARK_AS_REALLOCATABLE(FDirectionalLightDataHLSL);

struct FCascadeGenerationInfoHLSL
{
    // 0-64
    FMatrix4 ShadowMatrix;
    // 64-80
    FVector3 LightDirection;
    float    CascadeSplitLambda;
    // 80-96
    FVector3 LightUp;
    float    CascadeResolution;
    // 96-112
    int32    bDepthReductionEnabled;
    int32    MaxCascadeIndex;
    int32    Padding0;
    int32    Padding1;
};

MARK_AS_REALLOCATABLE(FCascadeGenerationInfoHLSL);

struct FProxyLightProbe
{
    void Release()
    {
        IrradianceMap.Reset();
        SpecularIrradianceMap.Reset();

        IrradianceMapUAV.Reset();
        for (FRHIUnorderedAccessViewRef& UAV : SpecularIrradianceMapUAVs)
        {
            UAV.Reset();
        }

        SpecularIrradianceMapUAVs.Clear();
        WeakSpecularIrradianceMapUAVs.Clear();
    }

    FRHITextureRef IrradianceMap;
    FRHITextureRef SpecularIrradianceMap;

    // TODO: We should be able to do this without the UAVs saved
    FRHIUnorderedAccessViewRef         IrradianceMapUAV;
    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;
    TArray<FRHIUnorderedAccessView*>   WeakSpecularIrradianceMapUAVs;
};

struct FOcclusionVolume
{
    FRHIBufferRef VertexBuffer;
    FRHIBufferRef IndexBuffer;
    uint32        IndexCount;
    EIndexFormat  IndexFormat;
};

struct FFrameResources
{
    FFrameResources();
    ~FFrameResources();

    bool Initialize();
    void Release();
    void BuildLightBuffers(FRHICommandList& CommandList, FScene* Scene);

    const EFormat DepthBufferFormat   = EFormat::D32_Float;
    const EFormat SSAOBufferFormat    = EFormat::R8_Unorm;
    const EFormat FinalTargetFormat   = EFormat::R16G16B16A16_Float;
    const EFormat RTOutputFormat      = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat  = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat        = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat      = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat        = EFormat::R10G10B10A2_Unorm;
    const EFormat ViewNormalFormat    = EFormat::R10G10B10A2_Unorm;
    const EFormat VelocityFormat      = EFormat::R16G16_Float;
    const EFormat ShadowMaskFormat    = EFormat::R8_Unorm;
    const EFormat ShadowMapFormat     = EFormat::D32_Float;
    const EFormat LightProbeFormat    = EFormat::R11G11B10_Float;
    
    // Limits
    const uint32 MaxPointLights       = 256;
    const uint32 MaxDirectionalLights = 256;
    const uint32 MaxPointLightShadows = 8;

    // Settings
    int32 CascadeSize                 = 0;
    int32 PointLightShadowSize        = 512;
    int32 IrradianceProbeSize         = 0;
    int32 SpecularIrradianceProbeSize = 0;

    // Global VertexInput
    FRHIVertexInputLayoutRef MeshInputLayout;

    // Main Window
    FRHITexture*             BackBuffer;

    // GlobalBuffers
    FRHIBufferRef            CameraBuffer;
    FRHIBufferRef            TransformBuffer;

    // Samplers
    FRHISamplerStateRef      PointLightShadowSampler;
    FRHISamplerStateRef      DirectionalLightShadowSampler;
    FRHISamplerStateRef      IrradianceSampler;
    FRHISamplerStateRef      GBufferSampler;
    FRHISamplerStateRef      FXAASampler;

    FRHITextureRef           Skybox;

    FRHITextureRef           IntegrationLUT;
    FRHISamplerStateRef      IntegrationLUTSampler;

    // GBuffer
    FRHITextureRef           SSAOBuffer;
    FRHITextureRef           FinalTarget;
    FRHITextureRef           GBuffer[GBuffer_NumBuffers];

    // TODO: Move to the RenderPass and store only the final downsized texture
    // Two resources that can be ping-ponged between
    inline static constexpr int32 NumReducedDepthBuffers = 2;
    FRHITextureRef           ReducedDepthBuffer[NumReducedDepthBuffers];

    // PointLights
    TArray<FVector4>                         PointLightsPosRad;
    TArray<FPointLightDataHLSL>              PointLightsData;
    FRHIBufferRef                            PointLightsBuffer;
    FRHIBufferRef                            PointLightsPosRadBuffer;

    TArray<FVector4>                         ShadowCastingPointLightsPosRad;
    TArray<FShadowCastingPointLightDataHLSL> ShadowCastingPointLightsData;
    FRHIBufferRef                            ShadowCastingPointLightsBuffer;
    FRHIBufferRef                            ShadowCastingPointLightsPosRadBuffer;
    FRHITextureRef                           PointLightShadowMaps;

    // DirectionalLight NOTE: Only one directional light
    FDirectionalLightDataHLSL  DirectionalLightData;
    bool                       DirectionalLightDataDirty;
    float                      CascadeSplitLambda;

    FRHIBufferRef              DirectionalLightDataBuffer;

    FCascadeGenerationInfoHLSL CascadeGenerationData;
    bool                       CascadeGenerationDataDirty;
    FRHIBufferRef              CascadeGenerationDataBuffer;

    FRHITextureRef             ShadowMapCascades;
    FRHITextureRef             DirectionalShadowMask;
    FRHITextureRef             CascadeIndexBuffer;

    FRHIBufferRef              CascadeMatrixBuffer;
    FRHIShaderResourceViewRef  CascadeMatrixBufferSRV;
    FRHIUnorderedAccessViewRef CascadeMatrixBufferUAV;

    FRHIBufferRef              CascadeSplitsBuffer;
    FRHIShaderResourceViewRef  CascadeSplitsBufferSRV;
    FRHIUnorderedAccessViewRef CascadeSplitsBufferUAV;

    // Occlusion Cube
    FOcclusionVolume           OcclusionVolume;

    // SkyLight
    FProxyLightProbe           Skylight;
    FProxyLightProbe           LocalProbe;

    // RayTracing
    FRHITextureRef             RTOutput;
    FRHIRayTracingSceneRef     RTScene;

    FRayTracingShaderResources             GlobalResources;
    FRayTracingShaderResources             RayGenLocalResources;
    FRayTracingShaderResources             MissLocalResources;
    TArray<FRHIRayTracingGeometryInstance> RTGeometryInstances;

    TArray<FRayTracingShaderResources>     RTHitGroupResources;
    TMap<class FMesh*, uint32>             RTMeshToHitGroupIndex;
    TResourceCache<FRHIShaderResourceView> RTMaterialTextureCache;

    // BackBuffer
    FRHIViewportRef MainViewport;
    EFormat         BackBufferFormat;
    uint32          CurrentWidth;
    uint32          CurrentHeight;
};

