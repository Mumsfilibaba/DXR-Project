#pragma once
#include "RHI/RHIRayTracing.h"
#include "MetalRHI/MetalViews.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalSceneAccelerationStructureRHI>    FMetalSceneAccelerationStructureRHIRef;
typedef TSharedRef<class FMetalGeometryAccelerationStructureRHI> FMetalGeometryAccelerationStructureRHIRef;

class FMetalGeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure
{
public:
    FMetalGeometryAccelerationStructureRHI(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    virtual ~FMetalGeometryAccelerationStructureRHI();

    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHIBaseInterface()        override final;
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    FString DebugName;
};

class FMetalSceneAccelerationStructureRHI : public FRHISceneAccelerationStructure
{
public:
    FMetalSceneAccelerationStructureRHI(FMetalDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc);
    virtual ~FMetalSceneAccelerationStructureRHI();

    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHIBaseInterface()        override final;
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

private:
    TSharedRef<FMetalShaderResourceViewRHI> View;
    FString                                 DebugName;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
