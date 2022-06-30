#pragma once
#include "D3D12Module.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12SamplerState.h"
#include "D3D12Shader.h"
#include "D3D12RayTracing.h"

#include "RHI/RHICoreInterface.h"

#include "CoreApplication/Windows/WindowsWindow.h"

class FD3D12CommandContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RayTracingDesc

struct FD3D12RayTracingDesc
{
    FD3D12RayTracingDesc()
        : Tier(D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    { }

    D3D12_RAYTRACING_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VariableRateShadingDesc

struct FD3D12VariableRateShadingDesc
{
    FD3D12VariableRateShadingDesc()
        : Tier(D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
        , ShadingRateImageTileSize(0)
    { }

    D3D12_VARIABLE_SHADING_RATE_TIER Tier;
    uint32 ShadingRateImageTileSize;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12MeshShadingDesc

struct FD3D12MeshShadingDesc
{
    FD3D12MeshShadingDesc()
        : Tier(D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    { }

    D3D12_MESH_SHADER_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12SamplerFeedbackDesc

struct FD3D12SamplerFeedbackDesc
{
    FD3D12SamplerFeedbackDesc()
        : Tier(D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED)
    { }

    D3D12_SAMPLER_FEEDBACK_TIER Tier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CoreInterface

class FD3D12CoreInterface : public CRHICoreInterface
{
    FD3D12CoreInterface();
    ~FD3D12CoreInterface();

public:

    static FD3D12CoreInterface* CreateD3D12Instance();

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHICoreInterface Interface

    virtual bool                     Initialize(bool bEnableDebug) override final;

    virtual FRHITexture2D*           RHICreateTexture2D(const FRHITexture2DInitializer& Initializer) override final;
    virtual FRHITexture2DArray*      RHICreateTexture2DArray(const FRHITexture2DArrayInitializer& Initializer) override final;
    virtual FRHITextureCube*         RHICreateTextureCube(const FRHITextureCubeInitializer& Initializer) override final;
    virtual FRHITextureCubeArray*    RHICreateTextureCubeArray(const FRHITextureCubeArrayInitializer& Initializer) override final;
    virtual FRHITexture3D*           RHICreateTexture3D(const FRHITexture3DInitializer& Initializer) override final;

    virtual FRHISamplerState*        RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final;

    virtual FRHIVertexBuffer*        RHICreateVertexBuffer(const FRHIVertexBufferInitializer& Initializer) override final;
    virtual FRHIIndexBuffer*         RHICreateIndexBuffer(const FRHIIndexBufferInitializer& Initializer) override final;
    virtual FRHIConstantBuffer*      RHICreateConstantBuffer(const FRHIConstantBufferInitializer& Initializer) override final;
    virtual FRHIGenericBuffer*       RHICreateGenericBuffer(const FRHIGenericBufferInitializer& Initializer) override final;

    virtual FRHIRayTracingScene*     RHICreateRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer) override final;
    virtual FRHIRayTracingGeometry*  RHICreateRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer) override final;

    virtual FRHIShaderResourceView*  RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) override final;
    virtual FRHIShaderResourceView*  RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) override final;

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) override final;

    virtual FRHIComputeShader*       RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIVertexShader*        RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIHullShader*          RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIDomainShader*        RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIGeometryShader*      RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIMeshShader*          RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIPixelShader*         RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIRayGenShader*        RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayAnyHitShader*     RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader*       RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIDepthStencilState*   RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) override final;
    virtual FRHIRasterizerState*     RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) override final;
    virtual FRHIBlendState*          RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) override final;
    virtual FRHIVertexInputLayout*   RHICreateVertexInputLayout(const FRHIVertexInputLayoutInitializer& Initializer) override final;

    virtual FRHIGraphicsPipelineState*   RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateInitializer& Initializer) override final;
    virtual FRHIComputePipelineState*    RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateInitializer& Initializer) override final;

    virtual FRHITimestampQuery* RHICreateTimestampQuery() override final;

    virtual FRHIViewport*       RHICreateViewport(const FRHIViewportInitializer& Initializer) override final;

    virtual IRHICommandContext* RHIGetDefaultCommandContext() override final { return DirectCmdContext; }

    virtual String GetAdapterDescription() const override final { return Adapter->GetDescription(); }

    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport)   const override final;
    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

public:

    FORCEINLINE FD3D12Device*  GetDevice()  const { return Device.Get(); }
    FORCEINLINE FD3D12Adapter* GetAdapter() const { return Adapter.Get(); }

    FORCEINLINE const FD3D12RayTracingDesc&          GetRayTracingDesc()          const { return RayTracingDesc;}
    FORCEINLINE const FD3D12VariableRateShadingDesc& GetVariableRateShadingDesc() const { return VariableRateShadingDesc; }
    FORCEINLINE const FD3D12MeshShadingDesc&         GetMeshShadingDesc()         const { return MeshShadingDesc;}
    FORCEINLINE const FD3D12SamplerFeedbackDesc&     GetSamplerFeedbackDesc()     const { return SamplerFeedbackDesc;}

    FORCEINLINE FD3D12OfflineDescriptorHeap* GetResourceOfflineDescriptorHeap() const { return ResourceOfflineDescriptorHeap; }

    FORCEINLINE FD3D12OfflineDescriptorHeap* GetRenderTargetOfflineDescriptorHeap() const { return RenderTargetOfflineDescriptorHeap; }

    FORCEINLINE FD3D12OfflineDescriptorHeap* GetDepthStencilOfflineDescriptorHeap() const { return DepthStencilOfflineDescriptorHeap; }

    FORCEINLINE FD3D12OfflineDescriptorHeap* GetSamplerOfflineDescriptorHeap() const { return SamplerOfflineDescriptorHeap; }

    FORCEINLINE TSharedRef<FD3D12ComputePipelineState> GetGenerateMipsPipelineTexure2D() const { return GenerateMipsTex2D_PSO; }

    FORCEINLINE TSharedRef<FD3D12ComputePipelineState> GetGenerateMipsPipelineTexureCube() const { return GenerateMipsTexCube_PSO; }

private:

    template<typename D3D12TextureType, typename InitializerType>
    D3D12TextureType* CreateTexture(const InitializerType& Initializer);

    template<typename D3D12BufferType, typename InitializerType>
    D3D12BufferType* CreateBuffer(const InitializerType& Initializer);

    FD3D12AdapterRef              Adapter;
    FD3D12DeviceRef               Device;
    
    FD3D12RayTracingDesc          RayTracingDesc;
    FD3D12MeshShadingDesc         MeshShadingDesc;
    FD3D12SamplerFeedbackDesc     SamplerFeedbackDesc;
    FD3D12VariableRateShadingDesc VariableRateShadingDesc;

    FD3D12CommandContext*         DirectCmdContext;

    FD3D12OfflineDescriptorHeap*  ResourceOfflineDescriptorHeap     = nullptr;
    FD3D12OfflineDescriptorHeap*  RenderTargetOfflineDescriptorHeap = nullptr;
    FD3D12OfflineDescriptorHeap*  DepthStencilOfflineDescriptorHeap = nullptr;
    FD3D12OfflineDescriptorHeap*  SamplerOfflineDescriptorHeap      = nullptr;

    TSharedRef<FD3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<FD3D12ComputePipelineState> GenerateMipsTexCube_PSO;
};
