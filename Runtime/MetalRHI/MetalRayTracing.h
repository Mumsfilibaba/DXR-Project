#pragma once
#include "Core/Containers/UniquePtr.h"
#include "RHI/RHIRayTracing.h"
#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalViews.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalCommandContext;

typedef TSharedRef<class FMetalSceneAccelerationStructureRHI>    FMetalSceneAccelerationStructureRHIRef;
typedef TSharedRef<class FMetalGeometryAccelerationStructureRHI> FMetalGeometryAccelerationStructureRHIRef;

class FMetalAccelerationStructure : public FMetalDeviceChild, public FNonCopyable
{
public:
    id<MTLAccelerationStructure> GetMTLAccelerationStructure() const
    {
        return Storage ? Storage->GetAccelerationStructure() : nil;
    }

    FMetalResidencyEntry* GetResidencyEntry() const
    {
        return Storage ? Storage->GetResidencyEntry() : nullptr;
    }

    bool CompactInPlace(FMetalCommandContext& Context, uint64 CompactedSizeInBytes);
    bool CopyFrom(FMetalCommandContext& Context, const FMetalAccelerationStructure& Source, bool bCompact);

protected:
    explicit FMetalAccelerationStructure(FMetalDevice* InDevice);
    virtual ~FMetalAccelerationStructure();

    bool ReserveStorage(FMetalCommandContext& Context, MTLSizeAndAlign SizeAndAlign);
    bool EncodeBuild(FMetalCommandContext& Context, MTLAccelerationStructureDescriptor* Descriptor, bool bRefit);

    virtual void OnStorageReplaced()
    {
    }

    void SetLabel(const String& InName);

    const String& GetLabel() const
    {
        return DebugName;
    }

private:
    TUniquePtr<FMetalResourceStorage> AllocateStorage(FMetalCommandContext& Context, MTLSizeAndAlign SizeAndAlign);
    void SetStorage(FMetalCommandContext& Context, TUniquePtr<FMetalResourceStorage> NewStorage);
    bool EncodeCopy(FMetalCommandContext& Context, id<MTLAccelerationStructure> Source, FMetalResidencyEntry* SourceEntry, MTLSizeAndAlign SizeAndAlign, bool bCompact);

    TUniquePtr<FMetalResourceStorage> Storage;
    FMetalResidencyEntry*             PinnedEntry;
    String                            DebugName;
};

class FMetalGeometryAccelerationStructureRHI : public FRHIGeometryAccelerationStructure, public FMetalAccelerationStructure
{
public:
    FMetalGeometryAccelerationStructureRHI(FMetalDevice* InDevice, const FRHIGeometryAccelerationStructureDesc& InGeometryDesc);
    virtual ~FMetalGeometryAccelerationStructureRHI();

    // FRHIGeometryAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Build(FMetalCommandContext& Context, const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc);

private:
    MTLPrimitiveAccelerationStructureDescriptor* CreateDescriptor(const FRHIGeometryAccelerationStructureBuildDesc& BuildDesc) const;
};

class FMetalSceneAccelerationStructureRHI : public FRHISceneAccelerationStructure, public FMetalAccelerationStructure
{
public:
    FMetalSceneAccelerationStructureRHI(FMetalDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc);
    virtual ~FMetalSceneAccelerationStructureRHI();

    // FRHISceneAccelerationStructure Interface
    virtual void* GetRHINativeResource() const override final;

    virtual FRHIShaderResourceView* GetShaderResourceView() const override final;
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const override final;

    virtual void SetDebugName(const String& InName)       override final;
    virtual void GetDebugName(String& OutDebugName) const override final;

    bool Build(FMetalCommandContext& Context, const FRHISceneAccelerationStructureBuildDesc& BuildDesc);

    FCriticalSection& GetBindlessLock()
    {
        return BindlessCS;
    }

protected:
    virtual void OnStorageReplaced() override final;

private:
    TSharedRef<FMetalShaderResourceViewRHI> View;
    FCriticalSection                        BindlessCS;
    uint32                                  NumBuiltInstances;
};

inline FMetalAccelerationStructure* GetMetalAccelerationStructure(FRHIRayTracingAccelerationStructure* AccelerationStructure)
{
    if (!AccelerationStructure)
    {
        return nullptr;
    }

    if (AccelerationStructure->GetAccelerationStructureType() == ERayTracingAccelerationStructureType::Scene)
    {
        return static_cast<FMetalSceneAccelerationStructureRHI*>(AccelerationStructure);
    }

    return static_cast<FMetalGeometryAccelerationStructureRHI*>(AccelerationStructure);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
