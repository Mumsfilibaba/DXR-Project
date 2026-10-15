#pragma once
#include "Core/Containers/String.h"
#include "RHI/RHICore.h"

class FRHIResource;

struct RHI_API RHIMemoryTracker
{
    /**
     * @brief Records a resource that has just been created, at the size it reports, billed to the calling thread's tag
     * and scope. The RHI facade calls it, so it runs where the resource was asked for.
     *
     * @param Resource The resource, whose type decides what kind of record it gets
     */
    static void OnResourceCreated(const FRHIResource* Resource);

    /**
     * @brief Reads the size of a resource again after the backend changed the memory behind it
     *
     * @param Resource The resource
     */
    static void OnResourceResized(const FRHIResource* Resource);

    /**
     * @brief Marks a resource whose last reference is gone as pending deletion, since the GPU can still be using it
     *
     * @param Resource The resource
     */
    static void OnResourceReleased(const FRHIResource* Resource);

    /**
     * @brief Forgets a resource as it is destroyed, giving its memory back
     *
     * @param Resource The resource
     */
    static void OnResourceDestroyed(const FRHIResource* Resource);

    /**
     * @brief Sets the debug name a resource is listed under
     *
     * @param Resource The resource
     * @param Name The name
     */
    static void OnResourceNamed(const FRHIResource* Resource, const String& Name);
};
