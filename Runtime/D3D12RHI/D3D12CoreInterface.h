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

class CD3D12CommandContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Texture Helpers

template<typename D3D12TextureType>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename D3D12TextureType>
bool IsTextureCube();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12CoreInterface

class CD3D12CoreInterface : public CRHICoreInterface
{
    CD3D12CoreInterface();
    ~CD3D12CoreInterface();

public:

    static CD3D12CoreInterface* CreateD3D12Instance();

    FORCEINLINE CD3D12Device* GetDevice() const
    {
        return Device;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetResourceOfflineDescriptorHeap() const
    {
        return ResourceOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetRenderTargetOfflineDescriptorHeap() const
    {
        return RenderTargetOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetDepthStencilOfflineDescriptorHeap() const
    {
        return DepthStencilOfflineDescriptorHeap;
    }

    FORCEINLINE CD3D12OfflineDescriptorHeap* GetSamplerOfflineDescriptorHeap() const
    {
        return SamplerOfflineDescriptorHeap;
    }

    FORCEINLINE TSharedRef<CD3D12ComputePipelineState> GetGenerateMipsPipelineTexure2D() const
    {
        return GenerateMipsTex2D_PSO;
    }

    FORCEINLINE TSharedRef<CD3D12ComputePipelineState> GetGenerateMipsPipelineTexureCube() const
    {
        return GenerateMipsTexCube_PSO;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHICoreInterface Interface

    virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D* CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture2DArray* CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCube* CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCubeArray*CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture3D* CreateTexture3D(EFormat Format,uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, ETextureUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;

    virtual CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final;

    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) override final;
    virtual CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) override final;
    virtual CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) override final;
    virtual CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) override final;

    virtual CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) override final;
    virtual CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) override final;

    virtual CRHIShaderResourceView* CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo) override final;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) override final;
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) override final;
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) override final;

    virtual CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual CRHIDepthStencilState* RHICreateDepthStencilState(const CRHIDepthStencilStateInitializer& Initializer) override final;
    virtual CRHIRasterizerState* RHICreateRasterizerState(const CRHIRasterizerStateInitializer& Initializer) override final;
    virtual CRHIBlendState* RHICreateBlendState(const CRHIBlendStateInitializer& Initializer) override final;
    virtual CRHIVertexInputLayout* RHICreateVertexInputLayout(const CRHIVertexInputLayoutInitializer& Initializer) override final;

    virtual CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const CRHIGraphicsPipelineStateInitializer& Initializer) override final;
    virtual CRHIComputePipelineState* RHICreateComputePipelineState(const CRHIComputePipelineStateInitializer& Initializer) override final;
    virtual CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const CRHIRayTracingPipelineStateInitializer& Initializer) override final;

    virtual class CRHITimestampQuery* RHICreateTimestampQuery() override final;

    virtual class CRHIViewport* CreateViewport(CGenericWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final { return DirectCmdContext; }

    virtual String GetAdapterName() const override final { return Device->GetAdapterName(); }

    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport)   const override final;
    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

private:

    template<typename D3D12TextureType>
    D3D12TextureType* CreateTexture( EFormat Format
                                   , uint32 SizeX
                                   , uint32 SizeY
                                   , uint32 SizeZ
                                   , uint32 NumMips
                                   , uint32 NumSamples
                                   , ETextureUsageFlags Flags
                                   , EResourceAccess InitialState
                                   , const SRHIResourceData* InitialData
                                   , const SClearValue& OptimalClearValue);

    // TODO: Avoid template here
    template<typename D3D12BufferType>
    bool CreateBuffer(D3D12BufferType* Buffer, uint32 Size, const CRHIBufferInitializer& Initializer);

    CD3D12Device* Device = nullptr;
    
    CD3D12CommandContext* DirectCmdContext;
    
    CD3D12RootSignatureCache* RootSignatureCache = nullptr;

    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap     = nullptr;
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap      = nullptr;

    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTexCube_PSO;
};

extern CD3D12CoreInterface* GD3D12Instance;
