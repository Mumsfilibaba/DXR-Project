#pragma once
#include "D3D12Device.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandContext.h"
#include "D3D12Texture.h"
#include "D3D12SamplerState.h"
#include "D3D12Shader.h"
#include "D3D12RayTracing.h"
#include "RHI/RHIInterface.h"
#include "CoreApplication/Windows/WindowsWindow.h"

class FD3D12CommandContext;

struct D3D12_RHI_API FD3D12InterfaceModule final
    : public FRHIInterfaceModule
{
    virtual class FRHIInterface* CreateInterface() override final;
};


class D3D12_RHI_API FD3D12Interface 
    : public FRHIInterface
{
public:
    FD3D12Interface();
    ~FD3D12Interface();
    
    static FD3D12Interface* GetRHI() 
    {
        CHECK(GD3D12Interface != nullptr);
        return GD3D12Interface; 
    }

    virtual bool Initialize() override final;

    virtual FRHITexture*             RHICreateTexture(const FRHITextureDesc& InDesc, EResourceAccess InInitialState, const IRHITextureData* InInitialData) override final;
    virtual FRHIBuffer*              RHICreateBuffer(const FRHIBufferDesc& InDesc, EResourceAccess InInitialState, const void* InInitialData)              override final;

    virtual FRHISamplerState*        RHICreateSamplerState(const FRHISamplerStateDesc& InDesc) override final;
    
    virtual FRHIViewport*            RHICreateViewport(const FRHIViewportDesc& InDesc) override final;

    virtual FRHITimestampQuery*      RHICreateTimestampQuery() override final;
    
    virtual FRHIRayTracingScene*     RHICreateRayTracingScene(const FRHIRayTracingSceneDesc& InDesc)       override final;
    virtual FRHIRayTracingGeometry*  RHICreateRayTracingGeometry(const FRHIRayTracingGeometryDesc& InDesc) override final;

    virtual FRHIShaderResourceView*  RHICreateShaderResourceView(const FRHITextureSRVDesc& InDesc) override final;
    virtual FRHIShaderResourceView*  RHICreateShaderResourceView(const FRHIBufferSRVDesc& InDesc)  override final;

    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHITextureUAVDesc& InDesc) override final;
    virtual FRHIUnorderedAccessView* RHICreateUnorderedAccessView(const FRHIBufferUAVDesc& InDesc)  override final;

    virtual FRHIComputeShader*       RHICreateComputeShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIVertexShader*        RHICreateVertexShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIHullShader*          RHICreateHullShader(const TArray<uint8>& ShaderCode)          override final;
    virtual FRHIDomainShader*        RHICreateDomainShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIGeometryShader*      RHICreateGeometryShader(const TArray<uint8>& ShaderCode)      override final;
    virtual FRHIPixelShader*         RHICreatePixelShader(const TArray<uint8>& ShaderCode)         override final;
    
    virtual FRHIMeshShader*          RHICreateMeshShader(const TArray<uint8>& ShaderCode)          override final;
    virtual FRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode) override final;

    virtual FRHIRayGenShader*        RHICreateRayGenShader(const TArray<uint8>& ShaderCode)        override final;
    virtual FRHIRayAnyHitShader*     RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)     override final;
    virtual FRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode) override final;
    virtual FRHIRayMissShader*       RHICreateRayMissShader(const TArray<uint8>& ShaderCode)       override final;

    virtual FRHIDepthStencilState*       RHICreateDepthStencilState(const FRHIDepthStencilStateDesc& InDesc) override final;
    virtual FRHIRasterizerState*         RHICreateRasterizerState(const FRHIRasterizerStateDesc& InDesc)     override final;
    virtual FRHIBlendState*              RHICreateBlendState(const FRHIBlendStateDesc& InDesc)               override final;
    virtual FRHIVertexInputLayout*       RHICreateVertexInputLayout(const FRHIVertexInputLayoutDesc& InDesc) override final;

    virtual FRHIGraphicsPipelineState*   RHICreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& InDesc)     override final;
    virtual FRHIComputePipelineState*    RHICreateComputePipelineState(const FRHIComputePipelineStateDesc& InDesc)       override final;
    virtual FRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& InDesc) override final;

    virtual IRHICommandContext* RHIObtainCommandContext() override final { return DirectContext; }

    virtual void RHIQueryRayTracingSupport(FRHIRayTracingSupport& OutSupport)   const override final;
    virtual void RHIQueryShadingRateSupport(FRHIShadingRateSupport& OutSupport) const override final;
    virtual bool RHIQueryUAVFormatSupport(EFormat Format)                       const override final;

    virtual FString RHIGetAdapterDescription() const override final 
    { 
        CHECK(Adapter != nullptr);
        return Adapter->GetDescription(); 
    }

    virtual void* RHIGetAdapter() override final 
    {
        CHECK(Adapter != nullptr);
        return reinterpret_cast<void*>(Adapter->GetDXGIAdapter());
    }

    virtual void* RHIGetDevice() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12Device());
    }

    virtual void* RHIGetDirectCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Direct));
    }

    virtual void* RHIGetComputeCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Compute));
    }

    virtual void* RHIGetCopyCommandQueue() override final
    {
        CHECK(Device != nullptr);
        return reinterpret_cast<void*>(Device->GetD3D12CommandQueue(ED3D12CommandQueueType::Copy));
    }

public:
    FORCEINLINE FD3D12OfflineDescriptorHeap*  GetResourceOfflineDescriptorHeap()     const { return ResourceOfflineDescriptorHeap; }
    FORCEINLINE FD3D12OfflineDescriptorHeap*  GetRenderTargetOfflineDescriptorHeap() const { return RenderTargetOfflineDescriptorHeap; }
    FORCEINLINE FD3D12OfflineDescriptorHeap*  GetDepthStencilOfflineDescriptorHeap() const { return DepthStencilOfflineDescriptorHeap; }
    FORCEINLINE FD3D12OfflineDescriptorHeap*  GetSamplerOfflineDescriptorHeap()      const { return SamplerOfflineDescriptorHeap; }

    FORCEINLINE FD3D12ComputePipelineStateRef GetGenerateMipsPipelineTexure2D()   const { return GenerateMipsTex2D_PSO; }
    FORCEINLINE FD3D12ComputePipelineStateRef GetGenerateMipsPipelineTexureCube() const { return GenerateMipsTexCube_PSO; }
    
    FORCEINLINE FD3D12Adapter* GetAdapter() const { return Adapter.Get(); }
    FORCEINLINE FD3D12Device*  GetDevice()  const { return Device.Get(); }

    FORCEINLINE FD3D12CommandContext* ObtainCommandContext() { return DirectContext; }

private:
    FD3D12AdapterRef              Adapter;
    FD3D12DeviceRef               Device;

    FD3D12CommandContext*         DirectContext;

    FD3D12OfflineDescriptorHeap*  ResourceOfflineDescriptorHeap     = nullptr;
    FD3D12OfflineDescriptorHeap*  RenderTargetOfflineDescriptorHeap = nullptr;
    FD3D12OfflineDescriptorHeap*  DepthStencilOfflineDescriptorHeap = nullptr;
    FD3D12OfflineDescriptorHeap*  SamplerOfflineDescriptorHeap      = nullptr;

    FD3D12ComputePipelineStateRef GenerateMipsTex2D_PSO;
    FD3D12ComputePipelineStateRef GenerateMipsTexCube_PSO;

    static FD3D12Interface* GD3D12Interface;
};
