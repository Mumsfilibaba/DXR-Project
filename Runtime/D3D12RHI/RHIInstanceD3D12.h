#pragma once
#include "D3D12RHI.h"
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"

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
// CRHIInstanceD3D12

class CRHIInstanceD3D12 : public CRHIInstance
{
public:

    /**
     * Create a new CRHIInstanceD3D12 
     * 
     * @return: Returns the newly created CRHIInstanceD3D12
     */
    static CRHIInstance* CreateInstance();

    FORCEINLINE CD3D12Device* GetDevice() const
    {
        return Device.Get();
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
    // CRHIInstance Interface 

    virtual bool Initialize(bool bEnableDebug) override final;

    virtual CRHITexture2DRef                   CreateTexture2D(const CRHITextureDesc& InTextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData)        override final;
    virtual CRHITexture2DArrayRef              CreateTexture2DArray(const CRHITextureDesc& InTextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData)   override final;
    virtual CRHITextureCubeRef                 CreateTextureCube(const CRHITextureDesc& InTextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData)      override final;
    virtual CRHITextureCubeArrayRef            CreateTextureCubeArray(const CRHITextureDesc& InTextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) override final;
    virtual CRHITexture3DRef                   CreateTexture3D(const CRHITextureDesc& InTextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData)        override final;
    virtual CRHIBufferRef                      CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData)               override final;

    virtual CRHISamplerStateRef                CreateSamplerState(const class CRHISamplerStateDesc& CreateInfo) override final;

    virtual class CRHIRayTracingScene*         CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances) override final;
    virtual class CRHIRayTracingGeometry*      CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices) override final;

    virtual CRHIShaderResourceView*            CreateShaderResourceView(const SRHIShaderResourceViewDesc& CreateInfo)   override final;
    virtual CRHIUnorderedAccessView*           CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& CreateInfo) override final;
    virtual CRHIRenderTargetView*              CreateRenderTargetView(const SRHIRenderTargetViewDesc& CreateInfo)       override final;
    virtual CRHIDepthStencilView*              CreateDepthStencilView(const SRHIDepthStencilViewDesc& CreateInfo)       override final;

    virtual class CRHIComputeShader*           CreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual class CRHIVertexShader*            CreateVertexShader(const TArray<uint8>& ShaderCode)        override final;
    virtual class CRHIHullShader*              CreateHullShader(const TArray<uint8>& ShaderCode)          override final;
    virtual class CRHIDomainShader*            CreateDomainShader(const TArray<uint8>& ShaderCode)        override final;
    virtual class CRHIGeometryShader*          CreateGeometryShader(const TArray<uint8>& ShaderCode)      override final;
    virtual class CRHIMeshShader*              CreateMeshShader(const TArray<uint8>& ShaderCode)          override final;
    virtual class CRHIAmplificationShader*     CreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIPixelShader*             CreatePixelShader(const TArray<uint8>& ShaderCode)         override final;

    virtual class CRHIRayGenShader*            CreateRayGenShader(const TArray<uint8>& ShaderCode)        override final;
    virtual class CRHIRayAnyHitShader*         CreateRayAnyHitShader(const TArray<uint8>& ShaderCode)     override final;
    virtual class CRHIRayClosestHitShader*     CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual class CRHIRayMissShader*           CreateRayMissShader(const TArray<uint8>& ShaderCode)       override final;

    virtual class CRHIDepthStencilState*       CreateDepthStencilState(const SRHIDepthStencilStateDesc& CreateInfo) override final;
    virtual class CRHIRasterizerState*         CreateRasterizerState(const SRHIRasterizerStateDesc& CreateInfo)     override final;
    virtual class CRHIBlendState*              CreateBlendState(const SRHIBlendStateDesc& CreateInfo)               override final;
    virtual class CRHIInputLayoutState*        CreateInputLayout(const SRHIInputLayoutStateDesc& CreateInfo)        override final;

    virtual class CRHIGraphicsPipelineState*   CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& CreateInfo)     override final;
    virtual class CRHIComputePipelineState*    CreateComputePipelineState(const SRHIComputePipelineStateDesc& CreateInfo)       override final;
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& CreateInfo) override final;

    virtual class CRHITimestampQuery*          CreateTimestampQuery() override final;

    virtual CRHIViewportRef                    CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat) override final;

    // TODO: Create functions like "CheckRayTracingSupport(RayTracingSupportInfo& OutInfo)" instead
    virtual bool UAVSupportsFormat(ERHIFormat Format) const override final;

    virtual class IRHICommandContext* GetDefaultCommandContext() override final { return DirectCommandContext.Get(); }

    virtual String GetAdapterName() const override final { return Device->GetAdapterName(); }

    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const override final;
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const override final;

private:

    CRHIInstanceD3D12();
    ~CRHIInstanceD3D12();

    template<typename D3D12TextureType>
    D3D12TextureType* CreateTexture(ERHIFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData, const SClearValue& OptimalClearValue);

    CD3D12DeviceRef         Device;
    CD3D12CommandContextRef DirectCommandContext;
    
    CD3D12RootSignatureCache* RootSignatureCache = nullptr;

    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap     = nullptr;
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap      = nullptr;

    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTexCube_PSO;
};

extern CRHIInstanceD3D12* GD3D12RHIInstance;
