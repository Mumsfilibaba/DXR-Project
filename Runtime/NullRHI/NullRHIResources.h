#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FNullRHIBuffer : public FRHIBuffer
{
public:
    FNullRHIBuffer(const FRHIBufferInfo& InBufferInfo)
        : FRHIBuffer(InBufferInfo)
    {
    }

    virtual void* GetRHINativeHandle() const override final
    {
        return nullptr;
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final
    {
        return FRHIDescriptorHandle();
    }

    virtual void SetDebugName(const FString& InDebugName) override final
    {
        DebugName = InDebugName;
    }
    
    virtual FString GetDebugName() const override final
    {
        return DebugName;
    }

private:
    FString DebugName;
};

struct FNullRHIShaderResourceView : public FRHIShaderResourceView
{
    FNullRHIShaderResourceView(FRHIResource* InResource)
        : FRHIShaderResourceView(InResource)
    {
    }
};

struct FNullRHIUnorderedAccessView : public FRHIUnorderedAccessView
{
    FNullRHIUnorderedAccessView(FRHIResource* InResource)
        : FRHIUnorderedAccessView(InResource)
    {
    }
};

class FNullRHITexture : public FRHITexture
{
public:
    FNullRHITexture(const FRHITextureInfo& InTextureInfo)
        : FRHITexture(InTextureInfo)
        , ShaderResourceView(new FNullRHIShaderResourceView(this))
        , UnorderedAccessView(new FNullRHIUnorderedAccessView(this))
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIShaderResourceView* GetShaderResourceView()  const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessUAVHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView>  ShaderResourceView;
    TSharedRef<FNullRHIUnorderedAccessView> UnorderedAccessView;
};

struct FNullRHIRayTracingGeometry : public FRHIRayTracingGeometry
{
    FNullRHIRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
        : FRHIRayTracingGeometry(InGeometryInfo)
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

class FNullRHIRayTracingScene : public FRHIRayTracingScene
{
public:
    FNullRHIRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
        : FRHIRayTracingScene(InSceneInfo)
        , View(new FNullRHIShaderResourceView(this))
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

private:
    TSharedRef<FNullRHIShaderResourceView> View;
};

struct FNullRHISamplerState : public FRHISamplerState
{
    FNullRHISamplerState(const FRHISamplerStateInfo& InSamplerInfo)
        : FRHISamplerState(InSamplerInfo)
    {
    }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

class FNullRHISwapChain : public FRHISwapChain
{
public:
    FNullRHISwapChain(const FRHISwapChainInfo& InSwapChainInfo)
        : FRHISwapChain(InSwapChainInfo)
        , BackBuffer(nullptr)
    { 
        FRHITextureInfo BackBufferInfo = FRHITextureInfo::CreateTexture2D(Info.ColorFormat, Info.Width, Info.Height, 1, 1, ETextureUsageFlags::Presentable | ETextureUsageFlags::RenderTarget);
        BackBuffer = new FNullRHITexture(BackBufferInfo);
    }

    bool Resize(uint32 InWidth, uint32 InHeight)
    {
        Info.Width  = uint16(InWidth);
        Info.Height = uint16(InHeight);
        return true;
    }

    virtual FRHITexture* GetBackBuffer() const override final { return BackBuffer.Get(); }

private:
    TSharedRef<FNullRHITexture> BackBuffer;
};

struct FNullRHIQuery : public FRHIQuery
{
    FNullRHIQuery(EQueryType InQueryType)
        : FRHIQuery(InQueryType)
    {
    }
};

class FNullRHIVertexLayout : public FRHIVertexLayout
{
public:
    FNullRHIVertexLayout(const FRHIVertexLayoutInitializerList& InInitializerList)
        : FRHIVertexLayout()
        , InitializerList(InInitializerList)
    {
    }

    virtual FRHIVertexLayoutInitializerList GetInitializerList() const override final
    {
        return InitializerList;
    }

private:
    FRHIVertexLayoutInitializerList InitializerList;
};

class FNullRHIDepthStencilState : public FRHIDepthStencilState
{
public:
    FNullRHIDepthStencilState(const FRHIDepthStencilStateInfo& InInfo)
        : FRHIDepthStencilState()
        , Info(InInfo)
    {
    }

    virtual FRHIDepthStencilStateInfo GetInfo() const override final
    {
        return Info;
    }

private:
    FRHIDepthStencilStateInfo Info;
};

class FNullRHIRasterizerState : public FRHIRasterizerState
{
public:
    FNullRHIRasterizerState(const FRHIRasterizerStateInfo& InInfo)
        : FRHIRasterizerState()
        , Info(InInfo)
    {
    }

    virtual FRHIRasterizerStateInfo GetInfo() const override final
    {
        return Info;
    }

private:
    FRHIRasterizerStateInfo Info;
};

struct FNullRHIBlendState : public FRHIBlendState
{
public:
    FNullRHIBlendState(const FRHIBlendStateInitializer& InInitializer)
        : FRHIBlendState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIBlendStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIBlendStateInitializer Initializer;
};

struct FNullRHIGraphicsPipelineState : public FRHIGraphicsPipelineState
{
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

struct FNullRHIComputePipelineState : public FRHIComputePipelineState
{
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

struct FNullRHIRayTracingPipelineState : public FRHIRayTracingPipelineState
{
    virtual void* GetRHINativeHandle() const override final { return nullptr; }
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
