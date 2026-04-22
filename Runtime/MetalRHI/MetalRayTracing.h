#pragma once
#include "RHI/RHIRayTracing.h"
#include "MetalRHI/MetalViews.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalRayTracingGeometry : public FRHIGeometryAccelerationStructure
{
public:
    FMetalRayTracingGeometry(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIGeometryAccelerationStructure(InGeometryDesc)
    {
    }

    ~FMetalRayTracingGeometry() = default;

    virtual void* GetRHINativeHandle() const override final { return nullptr; }
    virtual void* GetRHIBaseInterface() override final { return reinterpret_cast<void*>(this); }
};

class FMetalRayTracingScene : public FRHISceneAccelerationStructure
{
public:
    FMetalRayTracingScene(FMetalDeviceContext* InDeviceContext, const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHISceneAccelerationStructure(InSceneDesc)
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
