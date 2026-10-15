#include "Core/Memory/MemoryProfiler.h"
#include "RHI/RHIMemoryTracker.h"
#include "RHI/RHIResource.h"

#if ENABLE_MEMORY_PROFILER

static EMemoryGPUResourceKind GetResourceKind(ERHIResourceType ResourceType)
{
    switch (ResourceType)
    {
        case ERHIResourceType::Texture: return EMemoryGPUResourceKind::Texture;
        case ERHIResourceType::Buffer:  return EMemoryGPUResourceKind::Buffer;
        default:                        return EMemoryGPUResourceKind::Unknown;
    }
}

void RHIMemoryTracker::OnResourceCreated(const FRHIResource* Resource)
{
    if (Resource)
    {
        FMemoryProfiler::Get().OnGPUResourceCreated(Resource, GetResourceKind(Resource->GetResourceType()), Resource->GetAllocatedSize());
    }
}

void RHIMemoryTracker::OnResourceResized(const FRHIResource* Resource)
{
    if (Resource)
    {
        FMemoryProfiler::Get().OnGPUResourceResized(Resource, Resource->GetAllocatedSize());
    }
}

void RHIMemoryTracker::OnResourceReleased(const FRHIResource* Resource)
{
    FMemoryProfiler::Get().OnGPUResourceReleased(Resource);
}

void RHIMemoryTracker::OnResourceDestroyed(const FRHIResource* Resource)
{
    FMemoryProfiler::Get().OnGPUResourceDestroyed(Resource);
}

void RHIMemoryTracker::OnResourceNamed(const FRHIResource* Resource, const String& Name)
{
    FMemoryProfiler::Get().OnGPUResourceNamed(Resource, *Name);
}

#else

void RHIMemoryTracker::OnResourceCreated(const FRHIResource*)
{
}

void RHIMemoryTracker::OnResourceResized(const FRHIResource*)
{
}

void RHIMemoryTracker::OnResourceReleased(const FRHIResource*)
{
}

void RHIMemoryTracker::OnResourceDestroyed(const FRHIResource*)
{
}

void RHIMemoryTracker::OnResourceNamed(const FRHIResource*, const String&)
{
}

#endif
