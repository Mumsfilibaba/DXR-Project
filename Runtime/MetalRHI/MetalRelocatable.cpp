#include "MetalRHI/MetalRelocatable.h"
#include "MetalRHI/MetalResource.h"
#include "MetalRHI/MetalViews.h"
#include "Core/Threading/ScopedLock.h"

FMetalRelocatable::FMetalRelocatable()
    : RelocationListeners()
    , RelocationListenersCS()
{
}

FMetalRelocatable::~FMetalRelocatable()
{
    CHECK(RelocationListeners.IsEmpty());
}

void FMetalRelocatable::AddRelocationListener(FMetalView* View)
{
    CHECK(View != nullptr);

    TScopedLock Lock(RelocationListenersCS);
    RelocationListeners.AddUnique(View);
}

void FMetalRelocatable::RemoveRelocationListener(FMetalView* View)
{
    TScopedLock Lock(RelocationListenersCS);
    RelocationListeners.Remove(View);
}

void FMetalRelocatable::NotifyRelocated(EMetalRelocation Relocation)
{
    TScopedLock Lock(RelocationListenersCS);

    for (FMetalView* View : RelocationListeners)
    {
        View->OnResourceRelocated(Relocation);
    }
}

void FMetalRelocatable::Relocate(FMetalResourceStorage& NewPlacement)
{
    TScopedLock Lock(RelocationListenersCS);

    GetRelocatableStorage().SwapPlacement(NewPlacement);
    OnStorageSwapped();

    for (FMetalView* View : RelocationListeners)
    {
        View->OnResourceRelocated(EMetalRelocation::Defrag);
    }
}

void FMetalRelocatable::NotifyReleased()
{
    TScopedLock Lock(RelocationListenersCS);

    for (FMetalView* View : RelocationListeners)
    {
        View->OnResourceReleased();
    }

    RelocationListeners.Clear();
}
