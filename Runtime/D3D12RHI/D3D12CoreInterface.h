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

    virtual CRHITexture2D* RHICreateTexture2D(const CRHITexture2DInitializer& Initializer) override final;
    virtual CRHITexture2DArray* RHICreateTexture2DArray(const CRHITexture2DArrayInitializer& Initializer) override final;
    virtual CRHITextureCube* RHICreateTextureCube(const CRHITextureCubeInitializer& Initializer) override final;
    virtual CRHITextureCubeArray*RHICreateTextureCubeArray(const CRHITextureCubeArrayInitializer& Initializer) override final;
    virtual CRHITexture3D* RHICreateTexture3D(const CRHITexture3DInitializer& Initializer) override final;

    virtual CRHISamplerState* RHICreateSamplerState(const CRHISamplerStateInitializer& Initializer) override final;

    virtual CRHIVertexBuffer* RHICreateVertexBuffer(const CRHIVertexBufferInitializer& Initializer) override final;
    virtual CRHIIndexBuffer* RHICreateIndexBuffer(const CRHIIndexBufferInitializer& Initializer) override final;
    virtual CRHIConstantBuffer* RHICreateConstantBuffer(const CRHIConstantBufferInitializer& Initializer) override final;
    virtual CRHIGenericBuffer* RHICreateGenericBuffer(const CRHIGenericBufferInitializer& Initializer) override final;

    virtual CRHIRayTracingScene* RHICreateRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer) override final;
    virtual CRHIRayTracingGeometry* RHICreateRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer) override final;

    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHITextureSRVInitializer& Initializer) override final;
    virtual CRHIShaderResourceView* RHICreateShaderResourceView(const CRHIBufferSRVInitializer& Initializer) override final;

    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHITextureUAVInitializer& Initializer) override final;
    virtual CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const CRHIBufferUAVInitializer& Initializer) override final;

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

    virtual class CRHIViewport* RHICreateViewport(const CRHIViewportInitializer& Initializer) override final;

    virtual class IRHICommandContext* RHIGetDefaultCommandContext() override final { return DirectCmdContext; }

    virtual String GetAdapterName() const override final { return Device->GetAdapterName(); }

    virtual void RHIQueryRayTracingSupport(SRayTracingSupport& OutSupport)   const override final;
    virtual void RHIQueryShadingRateSupport(SShadingRateSupport& OutSupport) const override final;

    virtual bool RHIQueryUAVFormatSupport(EFormat Format) const override final;

private:

    template<typename D3D12TextureType, typename InitializerType>
    D3D12TextureType* CreateTexture(const InitializerType& Initializer);

    // TODO: Avoid template here
    template<typename D3D12BufferType, typename InitializerType>
    D3D12BufferType* CreateBuffer(const InitializerType& Initializer);

    CD3D12Device*                Device = nullptr;
    
    CD3D12CommandContext*        DirectCmdContext;
    
    CD3D12RootSignatureCache*    RootSignatureCache = nullptr;

    CD3D12OfflineDescriptorHeap* ResourceOfflineDescriptorHeap     = nullptr;
    CD3D12OfflineDescriptorHeap* RenderTargetOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* DepthStencilOfflineDescriptorHeap = nullptr;
    CD3D12OfflineDescriptorHeap* SamplerOfflineDescriptorHeap      = nullptr;

    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTex2D_PSO;
    TSharedRef<CD3D12ComputePipelineState> GenerateMipsTexCube_PSO;
};

extern CD3D12CoreInterface* GD3D12Instance;
