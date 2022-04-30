#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIVertexBuffer

class CNullRHIVertexBuffer : public CRHIVertexBuffer
{
public:
    CNullRHIVertexBuffer(EBufferUsageFlags InFlags, uint32 InNumVertices, uint32 InStride)
        : CRHIVertexBuffer(InFlags, InNumVertices, InStride)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIIndexBuffer

class CNullRHIIndexBuffer : public CRHIIndexBuffer
{
public:
    CNullRHIIndexBuffer(EBufferUsageFlags InFlags, EIndexFormat InIndexFormat, uint32 InNumIndices)
        : CRHIIndexBuffer(InFlags, InIndexFormat, InNumIndices)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIConstantBuffer

class CNullRHIConstantBuffer : public CRHIConstantBuffer
{
public:
    CNullRHIConstantBuffer(EBufferUsageFlags InFlags, uint32 InSizeInBytes)
        : CRHIConstantBuffer(InFlags, InSizeInBytes)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNullRHIGenericBuffer

class CNullRHIGenericBuffer : public CRHIGenericBuffer
{
public:
    CNullRHIGenericBuffer(EBufferUsageFlags InFlags, uint32 InSizeInBytes, uint32 InStride)
        : CRHIGenericBuffer(InFlags, InSizeInBytes, InStride)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TNullRHIBuffer

template<typename BaseBufferType>
class TNullRHIBuffer : public BaseBufferType
{
public:

    template<typename... ArgTypes>
    TNullRHIBuffer(ArgTypes&&... Args)
        : BaseBufferType(Forward<ArgTypes>(Args)...)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual void* GetRHIResourceHandle() const override final { return nullptr; }

    virtual void* GetRHIBaseBuffer() override final { return this; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // Deprecated

    virtual void* Map(uint32 Offset, uint32 InSize) override final { return nullptr; }

    virtual void Unmap(uint32 Offset, uint32 InSize) override final { }

    virtual void SetName(const String& InName) override final { CRHIResource::SetName(InName); }

    virtual bool IsValid() const override final { return true; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif