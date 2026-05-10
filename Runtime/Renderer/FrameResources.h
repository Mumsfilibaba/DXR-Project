#pragma once
#include "Core/Containers/Map.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Math/Vector4.h"
#include "RHI/RHIResources.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIRayTracing.h"
#include "Engine/World/World.h"
#include "Engine/World/Lights/DirectionalLight.h"
#include "Renderer/RendererModule.h"

#define MAX_LIGHTS_PER_TILE (1024)
#define NUM_SHADOW_CASTING_POINT_LIGHTS (8)
#define NUM_LIGHT_PROBES (4)

struct EGBufferIndex
{
    enum Type
    {
        Albedo   = 0,
        Normal   = 1,
        Material = 2,
        Velocity = 3,
        Depth    = 4,

        Count,
        NumRenderTargets = Velocity + 1,
    };
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
    FVector3 Color    = FVector3(1.0f, 1.0f, 1.0f);
    float    Padding0 = 0.0f;
};

MARK_AS_REALLOCATABLE(FPointLightDataHLSL);

struct FShadowCastingPointLightDataHLSL
{
    // 0-16
    FVector3 Color      = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;

    // 16-32
    float FarPlane = 10.0f;
    float Padding0 = 0.0f;
    float Padding1 = 0.0f;
    float Padding2 = 0.0f;
};

MARK_AS_REALLOCATABLE(FShadowCastingPointLightDataHLSL);

struct FDirectionalLightDataHLSL
{
    // 0-16
    FVector3 Color      = FVector3(1.0f, 1.0f, 1.0f);
    float    ShadowBias = 0.005f;

    // 16-32
    FVector3 Direction = FVector3(0.0f, -1.0f, 0.0f);
    float    Padding0  = 0.0f;

    // 32-48
    FVector3 UpVector  = FVector3(0.0f, 0.0f, -1.0f);
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
    int32 MaxCascadeIndex;
    int32 bEnableTightFrustum;
    int32 bEnableStableCascades;
    float LightPositionOffset;

    // 112-128
    float LightNearPlane;
    float LightFarPlane;
    int32 Padding0;
    int32 Padding1;
};

MARK_AS_REALLOCATABLE(FCascadeGenerationInfoHLSL);

struct FLightProbeInfoHLSL
{
    // 0-16
    FVector3 BoxOriginWS;
    float    BoxProjection;

    // 16-32
    FVector3 BoxMinWS;
    float    Padding0;

    // 32-48
    FVector3 BoxMaxWS;
    float    Padding1;
};

MARK_AS_REALLOCATABLE(FLightProbeInfoHLSL);

struct FGlobalTextureFormats
{
    static constexpr EFormat DepthBufferFormat  = EFormat::D32_Float;
    static constexpr EFormat SSAOBufferFormat   = EFormat::R8_Unorm;
    static constexpr EFormat SceneTargetFormat  = EFormat::R16G16B16A16_Float;
    static constexpr EFormat RTOutputFormat     = EFormat::R16G16B16A16_Float;
    static constexpr EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    static constexpr EFormat AlbedoFormat       = EFormat::R8G8B8A8_Unorm;
    static constexpr EFormat MaterialFormat     = EFormat::R8G8B8A8_Unorm;
    static constexpr EFormat NormalFormat       = EFormat::R10G10B10A2_Unorm;
    static constexpr EFormat VelocityFormat     = EFormat::R16G16_Float;
    static constexpr EFormat ObjectIDFormat     = EFormat::R32_Uint;
    static constexpr EFormat ShadowMaskFormat   = EFormat::R32_Float;
    static constexpr EFormat ShadowMapFormat    = EFormat::D32_Float;
    static constexpr EFormat LightProbeFormat   = EFormat::R11G11B10_Float;
};

struct FFrameResources
{
    FFrameResources();
    ~FFrameResources();

    bool Initialize();
    void Release();
    void BuildLightBuffers(FRHICommandList& CommandList, FScene* Scene);

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
    FRHIInputLayoutRef  MeshInputLayout;

    // Global Buffers
    FRHIBufferRef       CameraBuffer;
    FRHIBufferRef       TransformBuffer;

    // Global Samplers
    FRHISamplerStateRef PointLightShadowSampler;
    FRHISamplerStateRef ShadowSamplerPoint;
    FRHISamplerStateRef ShadowSamplerPointCmp;
    FRHISamplerStateRef ShadowSamplerLinearCmp;
    FRHISamplerStateRef LightProbeSampler;
    FRHISamplerStateRef GBufferSampler;
    FRHISamplerStateRef FXAASampler;

    FRHITextureRef      IntegrationLUT;
    FRHISamplerStateRef IntegrationLUTSampler;

    // GBuffer
    FRHITextureRef      SSAOBuffer;
    FRHITextureRef      SceneTarget;
    FRHITextureRef      TonemappedTarget;
    FRHITextureRef      GBuffer[EGBufferIndex::Count];

#if EDITOR_BUILD
    // Editor-only: non-jittered depth + ObjectID buffers (used for stable selection outlines and picking).
    FRHITextureRef EditorNoJitterDepth;
    FRHITextureRef EditorObjectID_NoJitter;
#endif

    // TODO: Depth-pyramid, could be used for other techniques as well 
    static constexpr int32 NumReducedDepthBuffers = 2;
    FRHITextureRef ReducedDepthBuffer[NumReducedDepthBuffers];

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

    // Per-light DSV covering all 6 cube faces of a single shadow-casting point light
    TArray<FRHIDepthStencilViewRef>          PointLightShadowMapDSVs;
    // Per-face DSV: size MaxPointLightShadows * RHI_NUM_CUBE_FACES
    TArray<FRHIDepthStencilViewRef>          PointLightShadowMapFaceDSVs;

    // DirectionalLight NOTE: Only one directional light
    FDirectionalLightDataHLSL  DirectionalLightData;
    FRHIBufferRef              DirectionalLightDataBuffer;
    bool                       DirectionalLightDataDirty;
    float                      CascadeSplitLambda;

    FCascadeGenerationInfoHLSL CascadeGenerationData;
    bool                       CascadeGenerationDataDirty;
    FRHIBufferRef              CascadeGenerationDataBuffer;

    FRHITextureRef             ShadowCascades;
    FRHIShaderResourceViewRef  ShadowCascadesSRVs[NUM_SHADOW_CASCADES];

    // Covers all cascades
    FRHIDepthStencilViewRef                                    ShadowCascadesCombinedDSV;
    // One DSV per cascade
    TStaticArray<FRHIDepthStencilViewRef, NUM_SHADOW_CASCADES> ShadowCascadePerCascadeDSVs;

    FRHITextureRef              DirectionalShadowMask;
    FRHITextureRef              CascadeIndexBuffer;
    FRHIBufferRef               CascadeMatrixBuffer;
    FRHIShaderResourceViewRef   CascadeMatrixBufferSRV;
    FRHIUnorderedAccessViewRef  CascadeMatrixBufferUAV;
    FRHIBufferRef               CascadeSplitsBuffer;
    FRHIShaderResourceViewRef   CascadeSplitsBufferSRV;
    FRHIUnorderedAccessViewRef  CascadeSplitsBufferUAV;
    // Light-Probes
    FRHIBufferRef               LightProbeBuffer;
    TArray<FLightProbeInfoHLSL> LightProbeInfos;

    // RayTracing
    FRHITextureRef                                    RTOutput;
    FRHISceneAccelerationStructureRef                 RTScene;
    FRayTracingShaderResources                        GlobalResources;
    FRayTracingShaderResources                        RayGenLocalResources;
    FRayTracingShaderResources                        MissLocalResources;
    TArray<FRHIGeometryAccelerationStructureInstance> RTGeometryInstances;
    TArray<FRayTracingShaderResources>                RTHitGroupResources;
    TMap<class FMesh*, uint32>                        RTMeshToHitGroupIndex;
    TResourceCache<FRHIShaderResourceView>            RTMaterialTextureCache;

    // Output Target
	uint32 CurrentRenderWidth;
	uint32 CurrentRenderHeight;
};

