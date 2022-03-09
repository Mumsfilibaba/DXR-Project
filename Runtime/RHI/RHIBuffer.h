#pragma once
#include "RHIResourceBase.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedef

typedef TSharedRef<class CRHIBuffer> CRHIBufferRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexFormat 

enum class ERHIIndexFormat
{
    Unknown = 0,
    uint16  = 1,
    uint32  = 2,
};

inline const char* ToString(ERHIIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case ERHIIndexFormat::uint16: return "uint16";
    case ERHIIndexFormat::uint32: return "uint32";
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIIndexFormat helpers

inline ERHIIndexFormat GetIndexFormatFromStride(uint32 StrideInBytes)
{
    if (StrideInBytes == 2)
    {
        return ERHIIndexFormat::uint16;
    }
    else if (StrideInBytes == 4)
    {
        return ERHIIndexFormat::uint32;
    }
    else
    {
        return ERHIIndexFormat::Unknown;
    }
}

inline uint32 GetStrideFromIndexFormat(ERHIIndexFormat IndexFormat)
{
    if (IndexFormat == ERHIIndexFormat::uint16)
    {
        return 2;
    }
    else if (IndexFormat == ERHIIndexFormat::uint32)
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIBufferFlags

enum ERHIBufferFlags : uint32
{
    BufferFlag_None            = 0,
    BufferFlag_Default         = FLAG(1), // Default Device Memory
	BufferFlag_Readback        = FLAG(2), // CPU readable Memory
    BufferFlag_Dynamic         = FLAG(3), // Dynamic Memory
	BufferFlag_ConstantBuffer  = FLAG(3), // Can be used as constant buffer
	BufferFlag_VertexBuffer    = FLAG(4), // Can be used as vertex buffer
	BufferFlag_IndexBuffer     = FLAG(5), // Can be used as index buffer
    BufferFlag_UnorderedAccess = FLAG(6), // Can be used in UnorderedAccessViews
    BufferFlag_ShaderResource  = FLAG(7), // Can be used in ShaderResourceViews

    BufferFlags_RWBuffer = BufferFlag_UnorderedAccess | BufferFlag_ShaderResource
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBufferDesc

class CRHIBufferDesc
{
public:
	
	static CRHIBufferDesc CreateConstantBuffer(uint32 InSizeInBytes, uint32 InFlags)
	{
		return CRHIBufferDesc(InSizeInBytes, 1, InFlags | BufferFlag_ConstantBuffer);
	}
	
	static CRHIBufferDesc CreateVertexBuffer(uint32 NumVertices, uint32 VertexStride, uint32 InFlags)
	{
		return CRHIBufferDesc(NumVertices * VertexStride, VertexStride, InFlags | BufferFlag_VertexBuffer);
	}
	
	static CRHIBufferDesc CreateIndexBuffer(uint32 NumIndices, ERHIIndexFormat IndexFormat, uint32 InFlags)
	{
		return CRHIBufferDesc(NumIndices * GetStrideFromIndexFormat(IndexFormat), GetStrideFromIndexFormat(IndexFormat), InFlags | BufferFlag_IndexBuffer);
	}
	
	static CRHIBufferDesc CreateStructuredBuffer(uint32 NumElements, uint32 Stride, uint32 InFlags)
	{
		return CRHIBufferDesc(NumElements * Stride, Stride, InFlags);
	}
	
	static CRHIBufferDesc CreateBufferSRV(uint32 NumElements, uint32 Stride, uint32 InFlags)
	{
		return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlag_UnorderedAccess);
	}
	
	static CRHIBufferDesc CreateBufferUAV(uint32 NumElements, uint32 Stride, uint32 InFlags)
	{
		return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlag_ShaderResource);
	}
	
	static CRHIBufferDesc CreateRWBuffer(uint32 NumElements, uint32 Stride, uint32 InFlags)
	{
		return CRHIBufferDesc(NumElements * Stride, Stride, InFlags | BufferFlags_RWBuffer);
	}
	
	CRHIBufferDesc() = default;
	
	CRHIBufferDesc(uint32 InSizeInBytes, uint32 InStrideInBytes, uint32 InFlags)
		: SizeInBytes(InSizeInBytes)
		, StrideInBytes(InStrideInBytes)
		, Flags(InFlags)
	{
	}
	
	bool IsShaderWriteable() const
	{
		return (Flags & BufferFlag_UnorderedAccess);
	}
	
	bool IsShaderReadable() const
	{
		return (Flags & BufferFlag_ShaderResource);
	}
	
	bool IsRWBuffer() const
	{
		return (Flags & BufferFlags_RWBuffer);
	}

	bool IsDynamic() const
	{
		return (Flags & BufferFlag_Dynamic);
	}
	
	bool operator==(const CRHIBufferDesc& RHS) const
	{
		return (Flags == RHS.Flags) && (SizeInBytes == RHS.SizeInBytes) && (StrideInBytes == RHS.StrideInBytes);
	}
	
	bool operator!=(const CRHIBufferDesc& RHS) const
	{
		return !(*this == RHS);
	}
	
	uint32 SizeInBytes   = 0;
	uint32 StrideInBytes = 0;
	uint32 Flags         = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIBuffer

class CRHIBuffer : public CRHIResource
{
public:

    /**
     * Constructor
     *
	 * @param InBufferDesc: Description of the buffer
     */
    CRHIBuffer(const CRHIBufferDesc& InBufferDesc)
        : CRHIResource()
		, BufferDesc(InBufferDesc)
    {
    }

    /**
     * Cast resource to a Buffer
     * 
     * @return: Returns a pointer to a buffer interface if the object implements it
     */
    virtual CRHIBuffer* AsBuffer() { return this; }

    /**
     * Retrieve the flags that the buffer was created with
     * 
     * @return: Returns the flags that the buffer was created with
     */
    FORCEINLINE uint32 GetFlags() const
    {
        return BufferDesc.Flags;
    }
	
	/**
	 * Retrieve the size in bytes of the buffer
	 *
	 * @return: Returns the size in bytes of the buffer
	 */
	FORCEINLINE uint32 GetSize() const
	{
		return BufferDesc.SizeInBytes;
	}
	
	/**
	 * Retrieve the stride in bytes of the buffer
	 *
	 * @return: Returns the stride in bytes of the buffer
	 */
	FORCEINLINE uint32 GetStride() const
	{
		return BufferDesc.StrideInBytes;
	}

protected:
	CRHIBufferDesc BufferDesc;
};
