#pragma once
#include "RenderingCore.h"

#include "Core/RefCountedObject.h"

#include "Containers/TArray.h"

class Texture;
class Texture1D;
class Texture1DArray;
class Texture2D;
class Texture2DArray;
class Texture3D;
class TextureCube;
class TextureCubeArray;
class Buffer;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;
class RayTracingGeometry;
class RayTracingScene;

class PipelineResource : public RefCountedObject
{
public: 
    virtual ~PipelineResource()	= default;

    virtual void SetName(const std::string& Name)
    {
        UNREFERENCED_VARIABLE(Name);
    }

    virtual Void* GetNativeResource() const
    {
        return nullptr;
    }
};

struct ResourceData
{
    ResourceData() = default;

    ResourceData(const Void* InData)
        : Data(InData)
        , Pitch(0)
        , SlicePitch(0)
    {
    }

    ResourceData(const Void* InData, UInt32 InPitch)
        : Data(InData)
        , Pitch(InPitch)
        , SlicePitch(0)
    {
    }

    ResourceData(const Void* InData, UInt32 InPitch, UInt32 InSlicePitch)
        : Data(InData)
        , Pitch(InPitch)
        , SlicePitch(InSlicePitch)
    {
    }

    const Void* Data  = nullptr;
    UInt32 Pitch      = 0;
    UInt32 SlicePitch = 0;
};

class Resource : public PipelineResource
{
public:
    virtual ~Resource()	= default;

    virtual Texture* AsTexture()
    {
        return nullptr;
    }

    virtual const Texture* AsTexture() const
    {
        return nullptr;
    }

    virtual Buffer* AsBuffer()
    {
        return nullptr;
    }

    virtual const Buffer* AsBuffer() const
    {
        return nullptr;
    }

    virtual RayTracingGeometry* AsRayTracingGeometry()
    {
        return nullptr;
    }

    virtual const RayTracingGeometry* AsRayTracingGeometry() const
    {
        return nullptr;
    }

    virtual RayTracingScene* AsRayTracingScene()
    {
        return nullptr;
    }

    virtual const RayTracingScene* AsRayTracingScene() const
    {
        return nullptr;
    }
};

enum EBufferUsage : UInt32
{
    BufferUsage_None    = 0,
    BufferUsage_Default = FLAG(1), // GPU Memory
    BufferUsage_Dynamic = FLAG(2), // CPU Memory
    BufferUsage_UAV     = FLAG(3), // Can be used in UnorderedAccessViews
    BufferUsage_SRV     = FLAG(4), // Can be used in ShaderResourceViews
};

struct Range
{
    Range() = default;

    Range(UInt32 InOffset, UInt32 InSize)
        : Offset(InOffset)
        , Size(InSize)
    {
    }

    UInt32 Offset = 0;
    UInt32 Size   = 0;
};

class Buffer : public Resource
{
public:
    Buffer(UInt32 InSizeInBytes, UInt32 InUsage)
        : SizeInBytes(InSizeInBytes)
        , Usage(InUsage)
    {
    }

    virtual Buffer* AsBuffer() override
    {
        return this;
    }

    virtual const Buffer* AsBuffer() const override
    {
        return this;
    }

    virtual VertexBuffer* AsVertexBuffer()
    {
        return nullptr;
    }

    virtual const VertexBuffer* AsVertexBuffer() const
    {
        return nullptr;
    }

    virtual IndexBuffer* AsIndexBuffer()
    {
        return nullptr;
    }

    virtual const IndexBuffer* AsIndexBuffer() const
    {
        return nullptr;
    }

    virtual ConstantBuffer* AsConstantBuffer()
    {
        return nullptr;
    }

    virtual const ConstantBuffer* AsConstantBuffer() const
    {
        return nullptr;
    }

    virtual StructuredBuffer* AsStructuredBuffer()
    {
        return nullptr;
    }

    virtual const StructuredBuffer* AsStructuredBuffer() const
    {
        return nullptr;
    }

    virtual UInt64 GetSizeInBytes() const
    {
        return SizeInBytes;
    }

    virtual UInt64 GetRequiredAlignment() const
    {
        return 0;
    }

    virtual Void* Map(const Range* MappedRange) = 0;
    virtual void  Unmap(const Range* WrittenRange) = 0;

    FORCEINLINE UInt32 GetUsage() const
    {
        return Usage;
    }

    FORCEINLINE bool HasShaderResourceUsage() const
    {
        return Usage & BufferUsage_SRV;
    }

    FORCEINLINE bool HasUnorderedAccessUsage() const
    {
        return Usage & BufferUsage_UAV;
    }

    FORCEINLINE bool HasDynamicUsage() const
    {
        return Usage & BufferUsage_Dynamic;
    }

protected:
    UInt32 SizeInBytes;
    UInt32 Usage;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(UInt32 InSizeInBytes, UInt32 InStride, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , Stride(InStride)
        , NumElements(InSizeInBytes / InStride)
    {
    }

    virtual VertexBuffer* AsVertexBuffer() override
    {
        return this;
    }

    virtual const VertexBuffer* AsVertexBuffer() const override
    {
        return this;
    }

    FORCEINLINE UInt32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    UInt32 Stride;
    Int32 NumElements;
};

enum class EIndexFormat
{
    IndexFormat_UInt16 = 1,
    IndexFormat_UInt32 = 2
};

class IndexBuffer : public Buffer
{
public:
    inline IndexBuffer(UInt32 InSizeInBytes, EIndexFormat InIndexFormat, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , IndexFormat(InIndexFormat)
        , NumElements(InSizeInBytes / (InIndexFormat == EIndexFormat::IndexFormat_UInt32 ? 4 : 2))
    {
    }

    virtual IndexBuffer* AsIndexBuffer() override
    {
        return this;
    }

    virtual const IndexBuffer* AsIndexBuffer() const override
    {
        return this;
    }

    FORCEINLINE EIndexFormat GetIndexFormat() const
    {
        return IndexFormat;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    EIndexFormat IndexFormat;
    UInt32 NumElements;
};

class ConstantBuffer : public Buffer
{
public:
    ConstantBuffer(UInt32 SizeInBytes, UInt32 Usage)
        : Buffer(SizeInBytes, Usage)
    {
    }

    virtual ConstantBuffer* AsConstantBuffer() override
    {
        return this;
    }

    virtual const ConstantBuffer* AsConstantBuffer() const override
    {
        return this;
    }
};

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(UInt32 InSizeInBytes, UInt32 InStride, UInt32 Usage)
        : Buffer(InSizeInBytes, Usage)
        , Stride(InStride)
        , NumElements(InSizeInBytes / InStride)
    {
    }

    virtual StructuredBuffer* AsStructuredBuffer() override
    {
        return this;
    }

    virtual const StructuredBuffer* AsStructuredBuffer() const override
    {
        return this;
    }

    FORCEINLINE UInt32 GetStride() const
    {
        return Stride;
    }

    FORCEINLINE UInt32 GetNumElements() const
    {
        return NumElements;
    }

protected:
    UInt32 Stride;
    UInt32 NumElements;
};

enum ETextureUsage
{
    TextureUsage_None    = 0,
    TextureUsage_RTV     = FLAG(1), // RenderTargetView
    TextureUsage_DSV     = FLAG(2), // DepthStencilView
    TextureUsage_UAV     = FLAG(3), // UnorderedAccessView
    TextureUsage_SRV     = FLAG(4), // ShaderResourceView
    TextureUsage_Default = FLAG(5), // Default memory
    TextureUsage_Dynamic = FLAG(6), // CPU memory

    TextureUsage_RWTexture    = TextureUsage_UAV | TextureUsage_SRV,
    TextureUsage_RenderTarget = TextureUsage_RTV | TextureUsage_SRV,
    TextureUsage_ShadowMap    = TextureUsage_DSV | TextureUsage_SRV,
};

class Texture : public Resource
{
public:
    Texture(EFormat InFormat, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Format(InFormat)
        , Usage(InUsage)
        , OptimizedClearValue(InOptimizedClearValue)
    {
    }

    virtual Texture* AsTexture() override
    {
        return this;
    }

    virtual const Texture* AsTexture() const override
    {
        return this;
    }

    virtual Texture1D* AsTexture1D()
    {
        return nullptr;
    }

    virtual const Texture1D* AsTexture1D() const
    {
        return nullptr;
    }

    virtual Texture1DArray* AsTexture1DArray()
    {
        return nullptr;
    }

    virtual const Texture1DArray* AsTexture1DArray() const
    {
        return nullptr;
    }

    virtual Texture2D* AsTexture2D()
    {
        return nullptr;
    }

    virtual const Texture2D* AsTexture2D() const
    {
        return nullptr;
    }

    virtual Texture2DArray* AsTexture2DArray()
    {
        return nullptr;
    }

    virtual const Texture2DArray* AsTexture2DArray() const
    {
        return nullptr;
    }

    virtual Texture3D* AsTexture3D()
    {
        return nullptr;
    }

    virtual const Texture3D* AsTexture3D() const
    {
        return nullptr;
    }

    virtual TextureCube* AsTextureCube()
    {
        return nullptr;
    }

    virtual const TextureCube* AsTextureCube() const
    {
        return nullptr;
    }

    virtual TextureCubeArray* AsTextureCubeArray()
    {
        return nullptr;
    }

    virtual const TextureCubeArray* AsTextureCubeArray() const
    {
        return nullptr;
    }

    virtual UInt32 GetWidth() const
    {
        return 0;
    }

    virtual UInt32 GetHeight() const
    {
        return 1;
    }

    virtual UInt16 GetDepth() const
    {
        return 1;
    }

    virtual UInt16 GetArrayCount() const
    {
        return 1;
    }

    virtual UInt32 GetMipLevels() const
    {
        return 1;
    }

    virtual UInt32 GetSampleCount() const
    {
        return 1;
    }

    virtual Bool IsMultiSampled() const
    {
        return false;
    }

    FORCEINLINE EFormat GetFormat() const
    {
        return Format;
    }

    FORCEINLINE UInt32 GetUsage() const
    {
        return Usage;
    }

    FORCEINLINE Bool HasShaderResourceUsage() const
    {
        return Usage & TextureUsage_SRV;
    }

    FORCEINLINE Bool HasUnorderedAccessUsage() const
    {
        return Usage & TextureUsage_UAV;
    }

    FORCEINLINE Bool HasRenderTargetUsage() const
    {
        return Usage & TextureUsage_RTV;
    }

    FORCEINLINE Bool HasDepthStencilUsage() const
    {
        return Usage & TextureUsage_DSV;
    }

    FORCEINLINE const ClearValue& GetOptimizedClearValue() const
    {
        return OptimizedClearValue;
    }

protected:
    EFormat Format;
    UInt32    Usage;
    ClearValue OptimizedClearValue;
};

class Texture1D : public Texture
{
public:
    Texture1D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , MipLevels(InMipLevels)
    {
    }

    virtual Texture1D* AsTexture1D() override
    {
        return this;
    }

    virtual const Texture1D* AsTexture1D() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Width;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

protected:
    UInt32 Width;
    UInt32 MipLevels;
};

class Texture1DArray : public Texture
{
public:
    Texture1DArray(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, UInt16 InArrayCount, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , MipLevels(InMipLevels)
        , ArrayCount(InArrayCount)
    {
    }

    virtual Texture1DArray* AsTexture1DArray() override
    {
        return this;
    }

    virtual const Texture1DArray* AsTexture1DArray() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Width;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

    virtual UInt16 GetArrayCount() const override
    {
        return ArrayCount;
    }

protected:
    UInt32 Width;
    UInt32 MipLevels;
    UInt16 ArrayCount;
};

class Texture2D : public Texture
{
public:
    Texture2D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevels(InMipLevels)
        , SampleCount(InSampleCount)
    {
    }

    virtual Texture2D* AsTexture2D() override
    {
        return this;
    }

    virtual const Texture2D* AsTexture2D() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Width;
    }

    virtual UInt32 GetHeight() const override
    {
        return Height;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

    virtual UInt32 GetSampleCount() const override
    {
        return SampleCount;
    }

    virtual Bool IsMultiSampled() const override
    {
        return (SampleCount > 1);
    }

protected:
    UInt32 Width;
    UInt32 Height;
    UInt32 MipLevels;
    UInt32 SampleCount;
};

class Texture2DArray : public Texture
{
public:
    Texture2DArray(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt16 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , MipLevels(InMipLevels)
        , ArrayCount(InArrayCount)
        , SampleCount(InSampleCount)
    {
    }

    virtual Texture2DArray* AsTexture2DArray() override
    {
        return this;
    }

    virtual const Texture2DArray* AsTexture2DArray() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Width;
    }

    virtual UInt32 GetHeight() const override
    {
        return Height;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

    virtual UInt16 GetArrayCount() const override
    {
        return ArrayCount;
    }

    virtual UInt32 GetSampleCount() const override
    {
        return SampleCount;
    }

    virtual Bool IsMultiSampled() const override
    {
        return (SampleCount > 1);
    }

protected:
    UInt32 Width;
    UInt32 Height;
    UInt32 MipLevels;
    UInt16 ArrayCount;
    UInt32 SampleCount;
};

class TextureCube : public Texture
{
public:
    TextureCube(EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Size(InSize)
        , MipLevels(InMipLevels)
        , SampleCount(InSampleCount)
    {
    }

    virtual TextureCube* AsTextureCube() override
    {
        return this;
    }

    virtual const TextureCube* AsTextureCube() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Size;
    }

    virtual UInt32 GetHeight() const override
    {
        return Size;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

    virtual UInt16 GetArrayCount() const override
    {
        constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;
        return TEXTURE_CUBE_FACE_COUNT;
    }

    virtual UInt32 GetSampleCount() const override
    {
        return SampleCount;
    }

    virtual Bool IsMultiSampled() const override
    {
        return (SampleCount > 1);
    }

protected:
    UInt32 Size;
    UInt32 MipLevels;
    UInt32 SampleCount;
};

class TextureCubeArray : public Texture
{
public:
    TextureCubeArray(EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt16 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Size(InSize)
        , MipLevels(InMipLevels)
        , ArrayCount(InArrayCount)
        , SampleCount(InSampleCount)
    {
    }

    virtual TextureCubeArray* AsTextureCubeArray() override
    {
        return this;
    }

    virtual const TextureCubeArray* AsTextureCubeArray() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Size;
    }

    virtual UInt32 GetHeight() const override
    {
        return Size;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

    virtual UInt16 GetArrayCount() const override
    {
        constexpr UInt32 TEXTURE_CUBE_FACE_COUNT = 6;
        return ArrayCount * TEXTURE_CUBE_FACE_COUNT;
    }

    virtual UInt32 GetSampleCount() const override
    {
        return SampleCount;
    }

    virtual Bool IsMultiSampled() const override
    {
        return (SampleCount > 1);
    }

protected:
    UInt32 Size;
    UInt32 MipLevels;
    UInt16 ArrayCount;
    UInt32 SampleCount;
};

class Texture3D : public Texture
{
public:
    Texture3D(EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt16 InDepth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
        , MipLevels(InMipLevels)
    {
    }

    virtual Texture3D* AsTexture3D() override
    {
        return this;
    }

    virtual const Texture3D* AsTexture3D() const override
    {
        return this;
    }

    virtual UInt32 GetWidth() const override
    {
        return Width;
    }

    virtual UInt32 GetHeight() const override
    {
        return Height;
    }

    virtual UInt16 GetDepth() const override
    {
        return Depth;
    }

    virtual UInt32 GetMipLevels() const override
    {
        return MipLevels;
    }

protected:
    UInt32 Width;
    UInt32 Height;
    UInt16 Depth;
    UInt32 MipLevels;
};