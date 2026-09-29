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
        RHIResource        = 1,
        ObjCObject         = 2,
        BindlessSlot       = 3,
        StandaloneResource = 4,
    };

    FMetalDeferredObject(FRHIResource* InResource)
        : Type(EType::RHIResource)
    {
        CHECK(InResource != nullptr);
        RHIResource = InResource;
    }

    explicit FMetalDeferredObject(id<NSObject> InObject)
        : Type(EType::ObjCObject)
    {
        CHECK(InObject != nil);
        Object = [InObject retain];
    }

    FMetalDeferredObject(FMetalBindlessDescriptorManager* InManager, FRHIDescriptorHandle InHandle)
        : Type(EType::BindlessSlot)
    {
        CHECK(InManager != nullptr);
        CHECK(InHandle.IsValid());
        BindlessSlot.Manager = InManager;
        BindlessSlot.Handle  = InHandle;
    }

    FMetalDeferredObject(EType InType, id<MTLResource> InResource)
        : Type(InType)
    {
        CHECK(InType == EType::StandaloneResource);
        CHECK(InResource != nil);
        Resource = [InResource retain];
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
        id<NSObject>      Object;
        id<MTLResource>   Resource;
        FBindlessSlotData BindlessSlot;
    };
};
