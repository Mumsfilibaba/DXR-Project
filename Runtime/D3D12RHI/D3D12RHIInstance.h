#pragma once
#include "D3D12RHIModule.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandContext.h"
#include "D3D12RHITexture.h"

#include "RHI/RHIInstance.h"

#include "CoreApplication/Windows/WindowsWindow.h"

class CD3D12CommandContext;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// D3D12 Texture Helpers

template<typename D3D12TextureType>
D3D12_RESOURCE_DIMENSION GetD3D12TextureResourceDimension();

template<typename D3D12TextureType>
bool IsTextureCube();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12RHIInstance

class CD3D12RHIInstance : public CRHIInstance
{
    CD3D12RHIInstance();
    ~CD3D12RHIInstance();

public:

    /** Make a new RHI core */
    static CD3D12RHIInstance* CreateD3D12Instance();

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

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIInstance Interface

    virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2D*        CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture2DArray*   CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMipLevels, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCube*      CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITextureCubeArray* CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMipLevels, uint32 NumArraySlices, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;
    virtual CRHITexture3D*        CreateTexture3D(EFormat Format,uint32 Width,uint32 Height, uint32 Depth, uint32 NumMipLevels, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) override final;

    virtual class CRHISamplerState* CreateSamplerState(const struct SRHISamplerStateInfo& CreateInfo) override final;

    virtual CRHIVertexBuffer*   CreateVertexBuffer(uint32 Stride, uint32 NumVertices, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIIndexBuffer*    CreateIndexBuffer(EIndexFormat Format, uint32 NumIndices, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIConstantBuffer* CreateConstantBuffer(uint32 Size, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData) override final;
    virtual CRHIGenericBuffer*  CreateGenericBuffer(uint32 Stride, uint32 NumElements, EBufferUsageFlags Flags, EResourceAccess InitialState, const SRHIResourceData* InitalData) override final;

    virtual class CRHIRayTracingScene*    CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances) override final;
    virtual class CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIVertexBuffer* VertexBuffer, CRHIIndexBuffer* IndexBuffer) override final;

    virtual CRHIShaderResourceView*  CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo) override final;
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) override final;
    virtual CRHIRenderTargetView*    CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) override final;
    virtual CRHIDepthStencilView*    CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) override final;

    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIVertexShader*        CreateVertexShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIHullShader*          CreateHullShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIDomainShader*        CreateDomainShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIGeometryShader*      CreateGeometryShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIMeshShader*          CreateMeshShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIPixelShader*         CreatePixelShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIRayGenShader*        CreateRayGenShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayAnyHitShader*     CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayMissShader*       CreateRayMissShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo) override final;
    virtual class CRHIRasterizerState*   CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo) override final;
    virtual class CRHIBlendState*        CreateBlendState(const SRHIBlendStateInfo& CreateInfo) override final;
    virtual class CRHIInputLayoutState*  CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo) override final;

    virtual class CRHIGraphicsPipelineState*   CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo) override final;
    virtual class CRHIComputePipelineState*    CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo) override final;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo) override final;

    virtual class CRHITimestampQuery* CreateTimestampQuery() override final;

    virtual class CRHIViewport* CreateViewport(CGenericWindow* Window, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat(EFormat Format) const override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final { return DirectCmdContext; }

    virtual String GetAdapterName() const override final { return Device->GetAdapterName(); }

    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const override final;
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const override final;

private:
    template<typename D3D12TextureType>
    D3D12TextureType* CreateTexture(EFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 NumSamples, uint32 Flags, EResourceAccess InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue);

    template<typename D3D12BufferType>
    bool CreateBuffer(D3D12BufferType* Buffer, EBufferUsageFlags Flags, uint32 SizeInBytes, EResourceAccess InitialState, const SRHIResourceData* InitialData);

    CD3D12Device* Device = nullptr;
    
    CD3D12CommandContext* DirectCmdContext;
    
    CD3D12RootSignatureCache* RootSignatureCache = nullptr;

    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap     = nullptr;
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap      = nullptr;

    TSharedRef<CD3D12RHIComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<CD3D12RHIComputePipelineState> GenerateMipsTexCube_PSO;
};

extern CD3D12RHIInstance* GD3D12RHIInstance;
