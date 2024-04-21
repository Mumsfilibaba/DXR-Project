#pragma once
#include "MetalObject.h"
#include "MetalRefCounted.h"
#include "RHI/RHIResources.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalBuffer> FMetalBufferRef;

class FMetalBuffer : public FRHIBuffer, public FMetalObject, public FMetalRefCounted
{
public:
    FMetalBuffer(FMetalDeviceContext* DeviceContext, const FRHIBufferInfo& InBufferInfo);
    ~FMetalBuffer();

    bool Initialize(EResourceAccess InInitialAccess, const void* InInitialData);

    virtual int32 AddRef() override final { return FMetalRefCounted::AddRef(); }
    
    virtual int32 Release() override final { return FMetalRefCounted::Release(); }
    
    virtual int32 GetRefCount() const override final { return FMetalRefCounted::GetRefCount(); }

    virtual void* GetRHIBaseBuffer() override final { return reinterpret_cast<void*>(static_cast<FMetalBuffer*>(this)); }
    
    virtual void* GetRHIBaseResource() const override final { return reinterpret_cast<void*>(GetMTLBuffer()); }
    
    virtual FRHIDescriptorHandle GetBindlessHandle() const override final { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) override final;

    virtual FString GetDebugName() const override final;
    
    FORCEINLINE id<MTLBuffer> GetMTLBuffer() const 
    { 
        return Buffer; 
    }

    FORCEINLINE void SetMTLBuffer(id<MTLBuffer> InBuffer) 
    { 
        Buffer = [InBuffer retain]; 
    } 
    
private:
    id<MTLBuffer> Buffer;
};

inline FMetalBuffer* GetMetalBuffer(FRHIBuffer* Buffer)
{
    return Buffer ? reinterpret_cast<FMetalBuffer*>(Buffer->GetRHIBaseBuffer()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
