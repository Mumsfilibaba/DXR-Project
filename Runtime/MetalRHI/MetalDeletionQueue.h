#pragma once
#include "Core/Containers/Array.h"
#include "RHI/RHIResource.h"
#include "RHI/RHITypes.h"
#include "MetalRHI/MetalCore.h"

class FMetalBindlessDescriptorManager;

struct FMetalDeferredObject
{
    static void ProcessItems(const TArray<FMetalDeferredObject>& Items);

    enum class EType : uint8
    {
        RHIResource  = 1,
        MTLResource  = 2,
        MTLHeap      = 3,
        MTLObject    = 4,
        BindlessSlot = 5,
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

    FMetalDeferredObject(id<MTLRenderPipelineState> InPipelineState)
        : Type(EType::MTLObject)
    {
        CHECK(InPipelineState != nil);
        Object = [InPipelineState retain];
    }

    FMetalDeferredObject(id<MTLComputePipelineState> InPipelineState)
        : Type(EType::MTLObject)
    {
        CHECK(InPipelineState != nil);
        Object = [InPipelineState retain];
    }

    FMetalDeferredObject(id<MTLDepthStencilState> InDepthStencilState)
        : Type(EType::MTLObject)
    {
        CHECK(InDepthStencilState != nil);
        Object = [InDepthStencilState retain];
    }

    FMetalDeferredObject(MTLVertexDescriptor* InVertexDescriptor)
        : Type(EType::MTLObject)
    {
        CHECK(InVertexDescriptor != nil);
        Object = [InVertexDescriptor retain];
    }

    FMetalDeferredObject(FMetalBindlessDescriptorManager* InManager, FRHIDescriptorHandle InHandle)
        : Type(EType::BindlessSlot)
    {
        CHECK(InManager != nullptr);
        CHECK(InHandle.IsValid());
        BindlessSlot.Manager = InManager;
        BindlessSlot.Handle  = InHandle;
    }

    EType const Type;

    struct FBindlessSlotData
    {
        FMetalBindlessDescriptorManager* Manager = nullptr;
        FRHIDescriptorHandle             Handle;
    };

    union
    {
        FRHIResource*     RHIResource;
        id<MTLResource>   Resource;
        id<MTLHeap>       Heap;
        NSObject*         Object;
        FBindlessSlotData BindlessSlot;
    };
};
