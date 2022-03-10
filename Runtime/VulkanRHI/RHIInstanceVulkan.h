#pragma once
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanCommandContext.h"
#include "VulkanQueue.h"

#include "RHI/RHIInstance.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstanceVulkan

class CRHIInstanceVulkan : public CRHIInstance
{
public:
    
    /* Create a new VulkanInstance */
    static CRHIInstance* CreateInstance();

    FORCEINLINE CVulkanInstance* GetInstance() const
    {
        return Instance.Get();
    }

    FORCEINLINE CVulkanPhysicalDevice* GetAdapter() const
    {
        return Adapter.Get();
    }

    FORCEINLINE CVulkanDevice* GetDevice() const
    {
        return Device.Get();
    }
    
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIInstance Interface

    virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D*               CreateTexture2D(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture2DArray*          CreateTexture2DArray(ERHIFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCube*             CreateTextureCube(ERHIFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCubeArray*        CreateTextureCubeArray(ERHIFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture3D*               CreateTexture3D(ERHIFormat Format,uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHIBufferRef                CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const SRHIResourceData* InitalData) override final;

    virtual CRHISamplerStateRef          CreateSamplerState(const class CRHISamplerStateDesc& CreateInfo) override final;
    
    virtual CRHIRayTracingScene*         CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances) override final;
    virtual CRHIRayTracingGeometry*      CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices) override final;

    virtual CRHIShaderResourceView*      CreateShaderResourceView(const SRHIShaderResourceViewDesc& CreateInfo) override final;
    virtual CRHIUnorderedAccessView*     CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& CreateInfo) override final;
    virtual CRHIRenderTargetView*        CreateRenderTargetView(const SRHIRenderTargetViewDesc& CreateInfo) override final;
    virtual CRHIDepthStencilView*        CreateDepthStencilView(const SRHIDepthStencilViewDesc& CreateInfo) override final;

    virtual CRHIComputeShader*           CreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIVertexShader*            CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIHullShader*              CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIDomainShader*            CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIGeometryShader*          CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIMeshShader*              CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIAmplificationShader*     CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIPixelShader*             CreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIRayGenShader*            CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayAnyHitShader*         CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayClosestHitShader*     CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayMissShader*           CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIDepthStencilState*       CreateDepthStencilState(const SRHIDepthStencilStateDesc& CreateInfo) override final;
    virtual CRHIRasterizerState*         CreateRasterizerState(const SRHIRasterizerStateDesc& CreateInfo) override final;
    virtual CRHIBlendState*              CreateBlendState(const SRHIBlendStateDesc& CreateInfo) override final;
    virtual CRHIInputLayoutState*        CreateInputLayout(const SRHIInputLayoutStateDesc& CreateInfo) override final;

    virtual CRHIGraphicsPipelineState*   CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& CreateInfo) override final;
    virtual CRHIComputePipelineState*    CreateComputePipelineState(const SRHIComputePipelineStateDesc& CreateInfo) override final;
    virtual CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& CreateInfo) override final;

    virtual CRHITimestampQuery*          CreateTimestampQuery() override final;

    virtual CRHIViewport*                CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat(ERHIFormat Format) const override final;

    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const override final;
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const override final;
    
    virtual IRHICommandContext* GetDefaultCommandContext() override final { return DirectCommandContext.Get(); }
    
    virtual String GetAdapterName() const override final;

private:
    
    CRHIInstanceVulkan();
    ~CRHIInstanceVulkan() = default;
    
    TSharedRef<CVulkanInstance> Instance;
    TSharedRef<CVulkanPhysicalDevice> Adapter;
    TSharedRef<CVulkanDevice>         Device;

    TSharedRef<CVulkanQueue>   DirectCommandQueue;
    TSharedRef<CVulkanCommandContext> DirectCommandContext;
};
