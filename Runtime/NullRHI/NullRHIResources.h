#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FNullRHIBuffer : public FRHIBuffer
{
    FNullRHIBuffer(const FRHIBufferInfo& InBufferInfo)
        : FRHIBuffer(InBufferInfo)
    {
    }

    virtual void* GetRHINativeHandle() const override final { return nullptr; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }
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
    virtual FRHIUnorderedAccessView* GetUnorderedAccessView() const override final { return nullptr; }
    
    virtual FRHIDescriptorHandle GetBindlessSRVHandle() const override final { return FRHIDescriptorHandle(); }
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
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }
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
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }

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

class FNullRHIViewport : public FRHIViewport
{
public:
    FNullRHIViewport(const FRHIViewportInfo& InViewportInfo)
        : FRHIViewport(InViewportInfo)
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
    FNullRHIDepthStencilState(const FRHIDepthStencilStateInitializer& InInitializer)
        : FRHIDepthStencilState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIDepthStencilStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIDepthStencilStateInitializer Initializer;
};

class FNullRHIRasterizerState : public FRHIRasterizerState
{
public:
    FNullRHIRasterizerState(const FRHIRasterizerStateInitializer& InInitializer)
        : FRHIRasterizerState()
        , Initializer(InInitializer)
    {
    }

    virtual FRHIRasterizerStateInitializer GetInitializer() const override final
    {
        return Initializer;
    }

private:
    FRHIRasterizerStateInitializer Initializer;
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
