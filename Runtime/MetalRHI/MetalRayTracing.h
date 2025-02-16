#pragma once
#include "MetalViews.h"

#include "RHI/RHIRayTracing.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalRayTracingGeometry : public FRHIRayTracingGeometry
{
public:
    FMetalRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
        : FRHIRayTracingGeometry(InGeometryInfo)
    {
    }

    ~FMetalRayTracingGeometry() = default;

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }
};

class FMetalRayTracingScene : public FRHIRayTracingScene
{
public:
    FMetalRayTracingScene(FMetalDeviceContext* InDeviceContext, const FRHIRayTracingSceneInfo& InSceneInfo)
        : FRHIRayTracingScene(InSceneInfo)
        , View(new FMetalShaderResourceView(InDeviceContext, this))
    {
    }

    ~FMetalRayTracingScene() = default;

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final { return View.Get(); }
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final{ return FRHIDescriptorHandle(); }
 
private:
    TSharedRef<FMetalShaderResourceView> View;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
