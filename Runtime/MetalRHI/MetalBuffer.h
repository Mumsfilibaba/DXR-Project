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
// CMetalVertexBuffer

class CMetalVertexBuffer : public CRHIVertexBuffer
{
public:

    CMetalVertexBuffer(const CRHIVertexBufferInitializer& Initializer)
        : CRHIVertexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalIndexBuffer

class CMetalIndexBuffer : public CRHIIndexBuffer
{
public:
    
    CMetalIndexBuffer(const CRHIIndexBufferInitializer& Initializer)
        : CRHIIndexBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalConstantBuffer

class CMetalConstantBuffer : public CRHIConstantBuffer
{
public:
    
    CMetalConstantBuffer(const CRHIConstantBufferInitializer& Initializer)
        : CRHIConstantBuffer(Initializer)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIConstantBuffer Interface
    virtual CRHIDescriptorHandle GetBindlessHandle() const override final { return CRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalGenericBuffer

class CMetalGenericBuffer : public CRHIGenericBuffer
{
public:
    
    CMetalGenericBuffer(const CRHIGenericBufferInitializer& Initializer)
        : CRHIGenericBuffer(Initializer)
    { }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TMetalBuffer

template<typename BaseBufferType>
class TMetalBuffer : public BaseBufferType
{
public:

    template<typename... ArgTypes>
    TMetalBuffer(ArgTypes&&... Args)
        : BaseBufferType(Forward<ArgTypes>(Args)...)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIBuffer Interface

    virtual void* GetRHIBaseResource() const override final { return nullptr; }

    virtual void* GetRHIBaseBuffer() override final { return this; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif