#pragma once
#include "VulkanDriverInstance.h"
#include "VulkanDevice.h"

#include "RHI/RHIInstance.h"

#include "Core/Containers/Array.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanInstance

class CVulkanInstance : public CRHIInstance
{
public: 
    static CRHIInstance* CreateInstance();

	FORCEINLINE CVulkanDriverInstance* GetInstance() const
	{
		return Instance.Get();
	}

    FORCEINLINE CVulkanDevice* GetDevice() const
	{
		return Device.Get();
	}
	
public:

    /*///////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIInstance Interface

    virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D*               CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture2DArray*          CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCube*             CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCubeArray*        CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture3D*               CreateTexture3D(EFormat Format,uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;

    virtual CRHISamplerState*            CreateSamplerState(const struct SRHISamplerStateInfo& CreateInfo) override final;

    virtual CRHIVertexBuffer*            CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIIndexBuffer*             CreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIConstantBuffer*          CreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIStructuredBuffer*        CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData) override final;

    virtual CRHIRayTracingScene*         CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances) override final;
    virtual CRHIRayTracingGeometry*      CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer) override final;

    virtual CRHIShaderResourceView*      CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo) override final;
    virtual CRHIUnorderedAccessView*     CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) override final;
    virtual CRHIRenderTargetView*        CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) override final;
    virtual CRHIDepthStencilView*        CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) override final;

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

    virtual CRHIDepthStencilState*       CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo) override final;
    virtual CRHIRasterizerState*         CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo) override final;
    virtual CRHIBlendState*              CreateBlendState(const SRHIBlendStateInfo& CreateInfo) override final;
    virtual CRHIInputLayoutState*        CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo) override final;

    virtual CRHIGraphicsPipelineState*   CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo) override final;
    virtual CRHIComputePipelineState*    CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo) override final;
    virtual CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo) override final;

    virtual CRHITimestampQuery*          CreateTimestampQuery() override final;

    virtual CRHIViewport*                CreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final { return nullptr; }

    virtual String GetAdapterName() const override final { return ""; }

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat(EFormat Format) const override final;

    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const override final;
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const override final;

private:
	
	CVulkanInstance();
	~CVulkanInstance() = default;
	
	TSharedRef<CVulkanDriverInstance> Instance;
    TSharedRef<CVulkanDevice>         Device;
};