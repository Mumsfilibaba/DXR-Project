#pragma once
#include "D3D12RHIModule.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12RHICommandContext.h"
#include "D3D12RHITexture.h"

#include "RHI/RHIInterface.h"

#include "CoreApplication/Windows/WindowsWindow.h"

class CD3D12RHICommandContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

template<typename TD3D12Texture>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename TD3D12Texture>
bool IsTextureCube();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CD3D12RHIInterface : public CRHIInterface
{
public:

    /* Make a new RHI core */
    static FORCEINLINE CRHIInterface* Make() { return dbg_new CD3D12RHIInterface(); }

    /* Init the RHI Core, create device etc. */
    virtual bool Init(bool bEnableDebug) override final;

    virtual CRHITexture2D* CreateTexture2D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) override final;

    virtual CRHITexture2DArray* CreateTexture2DArray(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 NumMipLevels,
        uint32 NumSamples,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) override final;

    virtual CRHITextureCube* CreateTextureCube(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) override final;

    virtual CRHITextureCubeArray* CreateTextureCubeArray(
        EFormat Format,
        uint32 Size,
        uint32 NumMipLevels,
        uint32 NumArraySlices,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) override final;

    virtual CRHITexture3D* CreateTexture3D(
        EFormat Format,
        uint32 Width,
        uint32 Height,
        uint32 Depth,
        uint32 NumMipLevels,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitalData,
        const SClearValue& OptimizedClearValue) override final;

    virtual class CRHISamplerState* CreateSamplerState(const struct SSamplerStateCreateInfo& CreateInfo) override final;

    virtual CRHIVertexBuffer* CreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) override final;
    virtual CRHIIndexBuffer* CreateIndexBuffer(EIndexFormat Format, uint32 NumIndices, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) override final;
    virtual CRHIConstantBuffer* CreateConstantBuffer(uint32 Size, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) override final;
    virtual CRHIStructuredBuffer* CreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, EResourceState InitialState, const SResourceData* InitalData) override final;

    virtual class CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances) override final;
    virtual class CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer) override final;

    virtual CRHIShaderResourceView* CreateShaderResourceView(const SShaderResourceViewCreateInfo& CreateInfo) override final;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SUnorderedAccessViewCreateInfo& CreateInfo) override final;
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRenderTargetViewCreateInfo& CreateInfo) override final;
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SDepthStencilViewCreateInfo& CreateInfo) override final;

    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SDepthStencilStateCreateInfo& CreateInfo) override final;
    virtual class CRHIRasterizerState* CreateRasterizerState(const SRasterizerStateCreateInfo& CreateInfo) override final;
    virtual class CRHIBlendState* CreateBlendState(const SBlendStateCreateInfo& CreateInfo) override final;
    virtual class CRHIInputLayoutState* CreateInputLayout(const SInputLayoutStateCreateInfo& CreateInfo) override final;

    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SGraphicsPipelineStateCreateInfo& CreateInfo) override final;
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SComputePipelineStateCreateInfo& CreateInfo) override final;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRayTracingPipelineStateCreateInfo& CreateInfo) override final;

    virtual class CRHITimestampQuery* CreateTimestampQuery() override final;

    virtual class CRHIViewport* CreateViewport(CPlatformWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat(EFormat Format) const override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final { return DirectCmdContext.Get(); }

    virtual CString GetAdapterName() const override final { return Device->GetAdapterName(); }

    virtual void CheckRayTracingSupport(SRayTracingSupport& OutSupport) const override final;
    virtual void CheckShadingRateSupport(SShadingRateSupport& OutSupport) const override final;

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

    FORCEINLINE TSharedRef<CD3D12RHIComputePipelineState> GetGenerateMipsPipelineTexure2D() const
    {
        return GenerateMipsTex2D_PSO;
    }

    FORCEINLINE TSharedRef<CD3D12RHIComputePipelineState> GetGenerateMipsPipelineTexureCube() const
    {
        return GenerateMipsTexCube_PSO;
    }

private:

    CD3D12RHIInterface();
    ~CD3D12RHIInterface();

    template<typename TD3D12Texture>
    TD3D12Texture* CreateTexture(
        EFormat Format,
        uint32 SizeX, uint32 SizeY, uint32 SizeZ,
        uint32 NumMips,
        uint32 NumSamples,
        uint32 Flags,
        EResourceState InitialState,
        const SResourceData* InitialData,
        const SClearValue& OptimalClearValue);

    template<typename TD3D12Buffer>
    bool CreateBuffer(TD3D12Buffer* Buffer, uint32 SizeInBytes, uint32 Flags, EResourceState InitialState, const SResourceData* InitialData);

    // The Device Object
    CD3D12Device* Device = nullptr;
    // Default Command Context
    TSharedRef<CD3D12RHICommandContext> DirectCmdContext;
    // RootSignature cache
    CD3D12RootSignatureCache* RootSignatureCache = nullptr;

    // Resource Offline-Heap
    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap = nullptr;
    // RenderTarget Offline-Heap
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    // DepthStencil Offline-Heap
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    // Sampler Offline-Heap
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap = nullptr;

    // PipelineSate for GenerateMips (Texture2D)
    TSharedRef<CD3D12RHIComputePipelineState> GenerateMipsTex2D_PSO;
    // PipelineSate for GenerateMips (TextureCube)
    TSharedRef<CD3D12RHIComputePipelineState> GenerateMipsTexCube_PSO;
};

extern CD3D12RHIInterface* GD3D12RHICore;