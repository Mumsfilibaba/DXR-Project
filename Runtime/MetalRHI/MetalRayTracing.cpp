#include "MetalRHI/MetalRayTracing.h"

FMetalGeometryAccelerationStructureRHI::FMetalGeometryAccelerationStructureRHI(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
    : FRHIGeometryAccelerationStructure(InGeometryDesc)
{
}

FMetalGeometryAccelerationStructureRHI::~FMetalGeometryAccelerationStructureRHI() = default;

void* FMetalGeometryAccelerationStructureRHI::GetRHINativeResource() const
{
    return nullptr;
}

void* FMetalGeometryAccelerationStructureRHI::GetRHIBaseInterface()
{
    return reinterpret_cast<void*>(this);
}

void FMetalGeometryAccelerationStructureRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalGeometryAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

FMetalSceneAccelerationStructureRHI::FMetalSceneAccelerationStructureRHI(FMetalDevice* InDevice, const FRHISceneAccelerationStructureDesc& InSceneDesc)
    : FRHISceneAccelerationStructure(InSceneDesc)
    , View(new FMetalShaderResourceViewRHI(InDevice, this, FRHIShaderResourceViewDesc::CreateAccelerationStructure()))
{
}

FMetalSceneAccelerationStructureRHI::~FMetalSceneAccelerationStructureRHI() = default;

void* FMetalSceneAccelerationStructureRHI::GetRHINativeResource() const
{
    return nullptr;
}

void* FMetalSceneAccelerationStructureRHI::GetRHIBaseInterface()
{
    return reinterpret_cast<void*>(this);
}

FRHIShaderResourceView* FMetalSceneAccelerationStructureRHI::GetShaderResourceView() const
{
    return View.Get();
}

FRHIDescriptorHandle FMetalSceneAccelerationStructureRHI::GetBindlessHandle() const
{
    return FRHIDescriptorHandle();
}

void FMetalSceneAccelerationStructureRHI::SetDebugName(const String& InName)
{
    DebugName = InName;
}

void FMetalSceneAccelerationStructureRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}
