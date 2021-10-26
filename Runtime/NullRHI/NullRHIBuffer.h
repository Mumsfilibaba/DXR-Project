#pragma once
#include "RHI/RHIResources.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullRHIVertexBuffer : public CRHIVertexBuffer
{
public:
    CNullRHIVertexBuffer( uint32 InNumVertices, uint32 InStride, uint32 InFlags )
        : CRHIVertexBuffer( InNumVertices, InStride, InFlags )
    {
    }
};

class CNullRHIIndexBuffer : public CRHIIndexBuffer
{
public:
    CNullRHIIndexBuffer( EIndexFormat InIndexFormat, uint32 InNumIndices, uint32 InFlags )
        : CRHIIndexBuffer( InIndexFormat, InNumIndices, InFlags )
    {
    }
};

class CNullRHIConstantBuffer : public CRHIConstantBuffer
{
public:
    CNullRHIConstantBuffer( uint32 InSizeInBytes, uint32 InFlags )
        : CRHIConstantBuffer( InSizeInBytes, InFlags )
    {
    }
};

class CNullRHIStructuredBuffer : public CRHIStructuredBuffer
{
public:
    CNullRHIStructuredBuffer( uint32 InSizeInBytes, uint32 InStride, uint32 InFlags )
        : CRHIStructuredBuffer( InSizeInBytes, InStride, InFlags )
    {
    }
};

template<typename TBaseBuffer>
class TNullRHIBuffer : public TBaseBuffer
{
public:

    template<typename... ArgTypes>
    TNullRHIBuffer( ArgTypes&&... Args )
        : TBaseBuffer( Forward<ArgTypes>( Args )... )
    {
    }

    virtual void* Map( uint32 Offset, uint32 InSize ) override
    {
        return nullptr;
    }

    virtual void Unmap( uint32 Offset, uint32 InSize ) override final
    {
    }

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif