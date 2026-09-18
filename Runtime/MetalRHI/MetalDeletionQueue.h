#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIResource.h"
#include "MetalRHI/MetalCore.h"

struct FMetalDeferredObject
{
    static void ProcessItems(const TArray<FMetalDeferredObject>& Items);

    enum class EType : uint8
    {
        RHIResource  = 1,
        MTLResource  = 2,
        MTLHeap      = 3,
    };

    FMetalDeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
    {
        CHECK(InResource != nullptr);
        RHIResource = InResource;
    }

    FMetalDeferredObject(id<MTLResource> InResource)
        : Type(EType::MTLResource)
    {
        CHECK(InResource != nil);
        Resource = [InResource retain];
    }

    FMetalDeferredObject(id<MTLHeap> InHeap)
        : Type(EType::MTLHeap)
    {
        CHECK(InHeap != nil);
        Heap = [InHeap retain];
    }

    EType const Type;

    union
    {
        FRHIResource*   RHIResource;
        id<MTLResource> Resource;
        id<MTLHeap>     Heap;
    };
};
