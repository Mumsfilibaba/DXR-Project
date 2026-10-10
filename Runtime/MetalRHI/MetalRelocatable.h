#pragma once
#include "Core/Containers/Array.h"
#include "Core/Platform/CriticalSection.h"

class FMetalView;
class FMetalResourceStorage;

enum class EMetalRelocation : uint8
{
    // A transient buffer took new storage per update, and its views take fresh bindless slots
    Transient,

    // A defrag move replaced the placement, and every bindless slot keeps its index, which the renderer may have cached
    Defrag,
};

class FMetalRelocatable
{
public:
    FMetalRelocatable();
    virtual ~FMetalRelocatable();

    // The caller holds the relocation lock
    void AddRelocationListener(FMetalView* View);
    void RemoveRelocationListener(FMetalView* View);

    void NotifyRelocated(EMetalRelocation Relocation);
    void Relocate(FMetalResourceStorage& NewPlacement);

    FCriticalSection& GetRelocationLock()
    {
        return RelocationListenersCS;
    }

protected:
    virtual FMetalResourceStorage& GetRelocatableStorage() = 0;
    virtual void OnStorageSwapped() = 0;

    // The owner calls this first in its destructor
    void NotifyReleased();

private:
    TArray<FMetalView*> RelocationListeners;
    FCriticalSection    RelocationListenersCS;
};
