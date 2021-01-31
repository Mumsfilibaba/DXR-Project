#pragma once
#include "RenderingCore.h"

#include "Core/RefCountedObject.h"

#include <Containers/TArray.h>

enum class EIndexFormat
{
    UInt16 = 1,
    UInt32 = 2,
};

inline const Char* ToString(EIndexFormat IndexFormat)
{
    switch (IndexFormat)
    {
    case EIndexFormat::UInt16: return "UInt16";
    case EIndexFormat::UInt32: return "UInt32";
    default: return "Unknown";
    }
}

enum EBufferUsage : UInt32
{
    BufferUsage_None    = 0,
    BufferUsage_Default = FLAG(1), // Default Device Memory
    BufferUsage_Upload  = FLAG(2), // Upload Memory
    BufferUsage_UAV     = FLAG(3), // Can be used in UnorderedAccessViews
    BufferUsage_SRV     = FLAG(4), // Can be used in ShaderResourceViews
};

enum ETextureUsage
{
    TextureUsage_None         = 0,
    TextureUsage_RTV          = FLAG(1), // RenderTargetView
    TextureUsage_DSV          = FLAG(2), // DepthStencilView
    TextureUsage_UAV          = FLAG(3), // UnorderedAccessView
    TextureUsage_SRV          = FLAG(4), // ShaderResourceView
    TextureUsage_Default      = FLAG(5), // Default Heap
    TextureUsage_Upload       = FLAG(6), // Upload Heap
    TextureUsage_RWTexture    = TextureUsage_UAV | TextureUsage_SRV,
    TextureUsage_RenderTarget = TextureUsage_RTV | TextureUsage_SRV,
    TextureUsage_ShadowMap    = TextureUsage_DSV | TextureUsage_SRV,
};

enum class ESamplerMode : Byte
{
    Unknown    = 0,
    Wrap       = 1,
    Mirror     = 2,
    Clamp      = 3,
    Border     = 4,
    MirrorOnce = 5,
};

inline const Char* ToString(ESamplerMode SamplerMode)
{
    switch (SamplerMode)
    {
    case ESamplerMode::Wrap:       return "Wrap";
    case ESamplerMode::Mirror:     return "Mirror";
    case ESamplerMode::Clamp:      return "Clamp";
    case ESamplerMode::Border:     return "Border";
    case ESamplerMode::MirrorOnce: return "MirrorOnce";
    default: return "Unknown";
    }
}

enum class ESamplerFilter : Byte
{
   Unknown                                 = 0,
   MinMagMipPoint                          = 1,
   MinMagPoint_MipLinear                   = 2,
   MinPoint_MagLinear_MipPoint             = 3,
   MinPoint_MagMipLinear                   = 4,
   MinLinear_MagMipPoint                   = 5,
   MinLinear_MagPoint_MipLinear            = 6,
   MinMagLinear_MipPoint                   = 7,
   MinMagMipLinear                         = 8,
   Anistrotopic                            = 9,
   Comparison_MinMagMipPoint               = 10,
   Comparison_MinMagPoint_MipLinear        = 11,
   Comparison_MinPoint_MagLinear_MipPoint  = 12,
   Comparison_MinPoint_MagMipLinear        = 13,
   Comparison_MinLinear_MagMipPoint        = 14,
   Comparison_MinLinear_MagPoint_MipLinear = 15,
   Comparison_MinMagLinear_MipPoint        = 16,
   Comparison_MinMagMipLinear              = 17,
   Comparison_Anistrotopic                 = 18,
};

inline const Char* ToString(ESamplerFilter SamplerFilter)
{
    switch (SamplerFilter)
    {
    case ESamplerFilter::MinMagMipPoint:                          return "MinMagMipPoint";
    case ESamplerFilter::MinMagPoint_MipLinear:                   return "MinMagPoint_MipLinear";
    case ESamplerFilter::MinPoint_MagLinear_MipPoint:             return "MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::MinPoint_MagMipLinear:                   return "MinPoint_MagMipLinear";
    case ESamplerFilter::MinLinear_MagMipPoint:                   return "MinLinear_MagMipPoint";
    case ESamplerFilter::MinLinear_MagPoint_MipLinear:            return "MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::MinMagLinear_MipPoint:                   return "MinMagLinear_MipPoint";
    case ESamplerFilter::MinMagMipLinear:                         return "MinMagMipLinear";
    case ESamplerFilter::Anistrotopic:                            return "Anistrotopic";
    case ESamplerFilter::Comparison_MinMagMipPoint:               return "Comparison_MinMagMipPoint";
    case ESamplerFilter::Comparison_MinMagPoint_MipLinear:        return "Comparison_MinMagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinPoint_MagLinear_MipPoint:  return "Comparison_MinPoint_MagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinPoint_MagMipLinear:        return "Comparison_MinPoint_MagMipLinear";
    case ESamplerFilter::Comparison_MinLinear_MagMipPoint:        return "Comparison_MinLinear_MagMipPoint";
    case ESamplerFilter::Comparison_MinLinear_MagPoint_MipLinear: return "Comparison_MinLinear_MagPoint_MipLinear";
    case ESamplerFilter::Comparison_MinMagLinear_MipPoint:        return "Comparison_MinMagLinear_MipPoint";
    case ESamplerFilter::Comparison_MinMagMipLinear:              return "Comparison_MinMagMipLinear";
    case ESamplerFilter::Comparison_Anistrotopic:                 return "Comparison_Anistrotopic";
    default: return "Unknown";
    }
}

struct SamplerStateCreateInfo
{
    ESamplerMode    AddressU       = ESamplerMode::Clamp;
    ESamplerMode    AddressV       = ESamplerMode::Clamp;
    ESamplerMode    AddressW       = ESamplerMode::Clamp;
    ESamplerFilter  Filter         = ESamplerFilter::MinMagMipLinear;
    EComparisonFunc ComparisonFunc = EComparisonFunc::Never;
    Float           MipLODBias     = 0.0f;
    UInt32          MaxAnisotropy  = 1;
    Float           BorderColor[4] = { 0.0f,0.0f, 0.0f, 0.0f };
    Float           MinLOD         = -FLT_MAX;
    Float           MaxLOD         = FLT_MAX;
};

struct ClearValue
{
public:
    enum class EType
    {
        Color        = 1,
        DepthStencil = 2
    };

    struct Color
    {
        Color(Float InR, Float InG, Float InB, Float InA)
            : r(InR), g(InG), b(InB), a(InA)
        {
        }

        Color(const Color& Other)
        {
            Memory::Memcpy(Elements, Other.Elements);
        }

        void Set(Float InR, Float InG, Float InB, Float InA)
        {
            r = InR;
            g = InG;
            b = InB;
            a = InA;
        }

        Color& operator=(const Color& Other)
        {
            Memory::Memcpy(Elements, Other.Elements);
            return *this;
        }

        union
        {
            Float Elements[4];
            struct
            {
                Float r;
                Float g;
                Float b;
                Float a;
            };
        };
    };

    struct DepthStencil
    {
        DepthStencil(Float InDepth, UInt8 InStencil)
            : Depth(InDepth)
            , Stencil(InStencil)
        {
        }

        Float Depth;
        UInt8 Stencil;
    };

    ClearValue(Float Depth, UInt8 Stencil)
        : Type(EType::DepthStencil)
        , DepthStencil(Depth, Stencil)
    {
    }

    ClearValue(Float r, Float g, Float b, Float a)
        : Type(EType::Color)
        , Color(r, g, b, a)
    {
    }

    ClearValue(const ClearValue& Other)
    {
        if (Other.Type == EType::Color)
        {
            Color = Other.Color;
        }
        else if (Other.Type == EType::DepthStencil)
        {
            DepthStencil = Other.DepthStencil;
        }
    }

    ClearValue& operator=(const ClearValue& Other)
    {
        if (Other.Type == EType::Color)
        {
            Color = Other.Color;
        }
        else if (Other.Type == EType::DepthStencil)
        {
            DepthStencil = Other.DepthStencil;
        }

        return *this;
    }

    EType GetType() const { return Type; }
    
    Color& AsColor()
    {
        VALIDATE(Type == EType::Color);
        return Color;
    }

    DepthStencil& AsDepthStencil()
    {
        VALIDATE(Type == EType::DepthStencil);
        return DepthStencil;
    }

private:
    EType Type;
    union
    {
        Color        Color;
        DepthStencil DepthStencil;
    };
};

class Resource : public RefCountedObject
{
public:
    Resource()  = default;
    ~Resource() = default;

    virtual void SetName(const std::string& InName)
    {
        Name = InName;
    }

    virtual void* GetNativeResource() const { return nullptr; }

    const std::string& GetName() const { return Name; }

private:
    std::string Name;
};

class Texture : public Resource
{
public:
    Texture(EFormat InFormat, UInt32 InNumMipLevels, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Resource()
        , Format(InFormat)
        , NumMipLevels(InNumMipLevels)
        , Usage(InUsage)
        , OptimizedClearValue(InOptimizedClearValue)
    {
    }

    ~Texture() = default;

    virtual class Texture2D* AsTexture2D() { return nullptr; }
    virtual class Texture2DArray* AsTexture2DArray() { return nullptr; }
    virtual class TextureCube* AsTextureCube() { return nullptr; }
    virtual class TextureCubeArray* AsTextureCubeArray() { return nullptr; }
    virtual class Texture3D* AsTexture3D() { return nullptr; }

    EFormat GetFormat() const { return Format; }

    UInt32 GetNumMiplevels() const { return NumMipLevels; }
   
    UInt32 GetUsage() const { return Usage; }

    Bool IsUpload() const { return (Usage & TextureUsage_Upload); }
    Bool IsUAV() const { return (Usage & TextureUsage_UAV); }
    Bool IsSRV() const { return (Usage & TextureUsage_SRV); }
    Bool IsRTV() const { return (Usage & TextureUsage_RTV); }
    Bool IsDSV() const { return (Usage & TextureUsage_SRV); }

protected:
    EFormat Format;
    UInt32  NumMipLevels;
    UInt32  Usage;
    ClearValue OptimizedClearValue;
};

class Texture2D : public Texture
{
public:
    Texture2D(EFormat InFormat, UInt32 InWidth, UInt32 InHeight, UInt32 InNumMipLevels, UInt32 InNumSamples, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMipLevels, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , NumSamples(InNumSamples)
    {
    }

    ~Texture2D() = default;

    virtual Texture2D* AsTexture2D() override { return this; }

    UInt32 GetWidth() const { return Width; }
    UInt32 GetHeight() const { return Height; }

    UInt32 GetNumSamples() const { return NumSamples; }

    Bool IsMultiSampled() const { return NumSamples > 1; }

protected:
    UInt32 Width;
    UInt32 Height;
    UInt32 NumSamples;
};

class Texture2DArray : public Texture2D
{
public:
    Texture2DArray(EFormat InFormat, UInt32 InWidth, UInt32 InHeight, UInt32 InNumMipLevels, UInt32 InNumSamples, UInt32 InNumArraySlices, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Texture2D(InFormat, InWidth, InHeight, InNumMipLevels, InNumSamples, InUsage, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    ~Texture2DArray() = default;

    virtual Texture2D* AsTexture2D() override { return nullptr; }
    virtual Texture2DArray* AsTexture2DArray() override { return this; }

    UInt32 GetNumArraySlices() const { return NumArraySlices; }

protected:
    UInt32 NumArraySlices;
};

class TextureCube : public Texture
{
public:
    TextureCube(EFormat InFormat, UInt32 InSize, UInt32 InNumMipLevels, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMipLevels, InUsage, InOptimizedClearValue)
        , Size(InSize)
    {
    }

    ~TextureCube() = default;

    virtual TextureCube* AsTextureCube() override { return this; }

    UInt32 GetSize() const { return Size; }

protected:
    UInt32 Size;
};

class TextureCubeArray : public TextureCube
{
public:
    TextureCubeArray(EFormat InFormat, UInt32 InSize, UInt32 InNumMipLevels, UInt32 InNumArraySlices, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : TextureCube(InFormat, InSize, InNumMipLevels, InUsage, InOptimizedClearValue)
        , NumArraySlices(InNumArraySlices)
    {
    }

    ~TextureCubeArray() = default;

    virtual TextureCube* AsTextureCube() override { return nullptr; }
    virtual TextureCubeArray* AsTextureCubeArray() override { return this; }

    UInt32 GetNumArraySlices() const { return NumArraySlices; }

protected:
    UInt32 NumArraySlices;
};

class Texture3D : public Texture
{
    Texture3D(EFormat InFormat, UInt32 InWidth, UInt32 InHeight, UInt32 InDepth, UInt32 InNumMipLevels, UInt32 InUsage, const ClearValue& InOptimizedClearValue)
        : Texture(InFormat, InNumMipLevels, InUsage, InOptimizedClearValue)
        , Width(InWidth)
        , Height(InHeight)
        , Depth(InDepth)
    {
    }

    ~Texture3D() = default;

    virtual Texture3D* AsTexture3D() override { return this; }

    UInt32 GetWidth() const { return Width; }
    UInt32 GetHeight() const { return Height; }
    UInt32 GetDepth() const { return Depth; }

protected:
    UInt32 Width;
    UInt32 Height;
    UInt32 Depth;
};

class Buffer : public Resource
{
public:
    Buffer(UInt32 InUsage)
        : Resource()
        , Usage(InUsage)
    {
    }

    ~Buffer() = default;

    virtual class VertexBuffer* AsVertexBuffer() { return nullptr; }
    virtual class IndexBuffer* AsIndexBuffer() { return nullptr; }
    virtual class StructuredBuffer* AsStructuredBuffer() { return nullptr; }

    virtual void* Map(UInt32 Offset, UInt32 Size)   = 0;
    virtual void  Unmap(UInt32 Offset, UInt32 Size) = 0;

    UInt32 GetUsage() const { return Usage; }

    Bool IsUpload() const { return (Usage & BufferUsage_Upload); }
    Bool IsUAV() const { return (Usage & BufferUsage_UAV); }
    Bool IsSRV() const { return (Usage & BufferUsage_SRV); }

protected:
    UInt32 Usage;
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer(UInt32 InUsage, UInt32 InStride, UInt32 InNumVertices)
        : Buffer(InUsage)
        , Stride(InStride)
        , NumVertices(InNumVertices)
    {
    }

    ~VertexBuffer() = default;

    virtual VertexBuffer* AsVertexBuffer() override { return this; }

    UInt32 GetStride() const { return Stride; }
    UInt32 GetNumVertices() const { return NumVertices; }

protected:
    UInt32 Stride;
    UInt32 NumVertices;
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer(UInt32 InUsage, EIndexFormat InFormat, UInt32 InNumIndicies)
        : Buffer(InUsage)
        , Format(InFormat)
        , NumIndicies(InNumIndicies)
    {
    }

    ~IndexBuffer() = default;

    virtual IndexBuffer* AsIndexBuffer() override { return this; }

    EIndexFormat GetFormat() const { return Format; }
    UInt32 GetNumIndicies() const { return NumIndicies; }

protected:
    EIndexFormat Format;
    UInt32 NumIndicies;
};

class ConstantBuffer : public Resource
{
public:
    ConstantBuffer(UInt32 InSizeInBytes)
        : Resource()
        , SizeInBytes(InSizeInBytes)
    {
    }

    UInt32 GetSizeInBytes() const { return SizeInBytes; }

protected:
    UInt32 SizeInBytes;
};

class StructuredBuffer : public Buffer
{
public:
    StructuredBuffer(UInt32 InUsage, UInt32 InStride, UInt32 InNumElements)
        : Buffer(InUsage)
        , Stride(InStride)
        , NumElements(InNumElements)
    {
    }

    ~StructuredBuffer() = default;

    virtual StructuredBuffer* AsStructuredBuffer() override { return this; }

    UInt32 GetStride() const { return Stride; }
    UInt32 GetNumElements() const { return NumElements; }

protected:
    UInt32 Stride;
    UInt32 NumElements;
};

class SamplerState : public Resource
{
};

enum class EShaderStage
{
    Vertex        = 1,
    Hull          = 2,
    Domain        = 3,
    Geometry      = 4,
    Mesh          = 5,
    Amplification = 6,
    Pixel         = 7,
    Compute       = 8,
    RayGen        = 9,
    RayAnyHit     = 10,
    RayClosestHit = 11,
    RayMiss       = 12,
};

class Shader : public Resource
{
public:
    Shader()  = default;
    ~Shader() = default;

    virtual VertexShader* AsVertexShader() { return nullptr; }
    virtual PixelShader* AsPixelShader() { return nullptr; }

    virtual ComputeShader* AsComputeShader() { return nullptr; }
};

class ComputeShader : public Shader
{
};

class VertexShader : public Shader
{
};

class HullShader : public Shader
{
};

class DomainShader : public Shader
{
};

class GeometryShader : public Shader
{
};

class MeshShader : public Shader
{
};

class AmplificationShader : public Shader
{
};

class PixelShader : public Shader
{
};

class RayGenShader : public Shader
{
};

class RayHitShader : public Shader
{
};

class RayMissShader : public Shader
{
};

class PipelineState : public Resource
{
public:
    PipelineState()  = default;
    ~PipelineState() = default;

    virtual GraphicsPipelineState* AsGraphics() { return nullptr; }
    virtual ComputePipelineState* AsCompute() { return nullptr; }
    virtual RayTracingPipelineState* AsRayTracing() { return nullptr; }
};

enum class EDepthWriteMask
{
    Zero = 0,
    All  = 1
};

inline const Char* ToString(EDepthWriteMask DepthWriteMask)
{
    switch (DepthWriteMask)
    {
    case EDepthWriteMask::Zero: return "Zero";
    case EDepthWriteMask::All:  return "All";
    default: return "Unknown";
    }
}

enum class EStencilOp
{
    Keep    = 1,
    Zero    = 2,
    Replace = 3,
    IncrSat = 4,
    DecrSat = 5,
    Invert  = 6,
    Incr    = 7,
    Decr    = 8
};

inline const Char* ToString(EStencilOp StencilOp)
{
    switch (StencilOp)
    {
    case EStencilOp::Keep:    return "Keep";
    case EStencilOp::Zero:    return "Zero";
    case EStencilOp::Replace: return "Replace";
    case EStencilOp::IncrSat: return "IncrSat";
    case EStencilOp::DecrSat: return "DecrSat";
    case EStencilOp::Invert:  return "Invert";
    case EStencilOp::Incr:    return "Incr";
    case EStencilOp::Decr:    return "Decr";
    default: return "Unknown";
    }
}

struct DepthStencilOp
{
    EStencilOp      StencilFailOp      = EStencilOp::Keep;
    EStencilOp      StencilDepthFailOp = EStencilOp::Keep;
    EStencilOp      StencilPassOp      = EStencilOp::Keep;
    EComparisonFunc StencilFunc        = EComparisonFunc::Always;
};

struct DepthStencilStateCreateInfo
{
    EDepthWriteMask DepthWriteMask   = EDepthWriteMask::All;
    EComparisonFunc DepthFunc        = EComparisonFunc::Less;
    Bool            DepthEnable      = true;
    UInt8           StencilReadMask  = 0xff;
    UInt8           StencilWriteMask = 0xff;
    Bool            StencilEnable    = false;
    DepthStencilOp  FrontFace        = DepthStencilOp();
    DepthStencilOp  BackFace         = DepthStencilOp();
};

class DepthStencilState : public Resource
{
};

enum class ECullMode
{
    None  = 1,
    Front = 2,
    Back  = 3
};

inline const Char* ToString(ECullMode CullMode)
{
    switch (CullMode)
    {
    case ECullMode::None:  return "None";
    case ECullMode::Front: return "Front";
    case ECullMode::Back:  return "Back";
    default: return "Unknown";
    }
}

enum class EFillMode
{
    WireFrame = 1,
    Solid     = 2
};

inline const Char* ToString(EFillMode FillMode)
{
    switch (FillMode)
    {
    case EFillMode::WireFrame: return "WireFrame";
    case EFillMode::Solid:     return "Solid";
    default: return "Unknown";
    }
}

struct RasterizerStateCreateInfo
{
    EFillMode FillMode = EFillMode::Solid;
    ECullMode CullMode = ECullMode::Back;
    Bool   FrontCounterClockwise = false;
    Int32  DepthBias = 0;
    Float  DepthBiasClamp = 0.0f;
    Float  SlopeScaledDepthBias = 0.0f;
    Bool   DepthClipEnable = true;
    Bool   MultisampleEnable = false;
    Bool   AntialiasedLineEnable = false;
    UInt32 ForcedSampleCount = 0;
    Bool   EnableConservativeRaster = false;
};

class RasterizerState : public Resource
{
};

enum class EBlend
{
   Zero           = 1,
   One            = 2,
   SrcColor       = 3,
   InvSrcColor    = 4,
   SrcAlpha       = 5,
   InvSrcAlpha    = 6,
   DestAlpha      = 7,
   InvDestAlpha   = 8,
   DestColor      = 9,
   InvDestColor   = 10,
   SrcAlphaSat    = 11,
   BlendFactor    = 12,
   InvBlendFactor = 13,
   Src1Color      = 14,
   InvSrc1Color   = 15,
   Src1Alpha      = 16,
   InvSrc1Alpha   = 17
};

inline const Char* ToString(EBlend Blend)
{
    switch (Blend)
    {
    case EBlend::Zero:           return "Zero";
    case EBlend::One:            return "One";
    case EBlend::SrcColor:       return "SrcColor";
    case EBlend::InvSrcColor:    return "InvSrcColor";
    case EBlend::SrcAlpha:       return "SrcAlpha";
    case EBlend::InvSrcAlpha:    return "InvSrcAlpha";
    case EBlend::DestAlpha:      return "DestAlpha";
    case EBlend::InvDestAlpha:   return "InvDestAlpha";
    case EBlend::DestColor:      return "DestColor";
    case EBlend::InvDestColor:   return "InvDestColor";
    case EBlend::SrcAlphaSat:    return "SrcAlphaSat";
    case EBlend::BlendFactor:    return "BlendFactor";
    case EBlend::InvBlendFactor: return "InvBlendFactor";
    case EBlend::Src1Color:      return "Src1Color";
    case EBlend::InvSrc1Color:   return "InvSrc1Color";
    case EBlend::Src1Alpha:      return "Src1Alpha";
    case EBlend::InvSrc1Alpha:   return "InvSrc1Alpha";
    default: return "Unknown";
    }
}

enum class EBlendOp
{
    Add         = 1,
    Subtract    = 2,
    RevSubtract = 3,
    Min         = 4,
    Max         = 5
};

inline const Char* ToString(EBlendOp BlendOp)
{
    switch (BlendOp)
    {
    case EBlendOp::Add:         return "Add";
    case EBlendOp::Subtract:    return "Subtract";
    case EBlendOp::RevSubtract: return "RevSubtract";
    case EBlendOp::Min:         return "Min";
    case EBlendOp::Max:         return "Max";
    default: return "Unknown";
    }
}

enum class ELogicOp
{
    Clear        = 0,
    Set          = 1,
    Copy         = 2,
    CopyInverted = 3,
    Noop         = 4,
    Invert       = 5,
    And          = 6,
    Nand         = 7,
    Or           = 8,
    Nor          = 9,
    Xor          = 10,
    Equiv        = 11,
    AndReverse   = 12,
    AndInverted  = 13,
    OrReverse    = 14,
    OrInverted   = 15
};

inline const Char* ToString(ELogicOp LogicOp)
{
    switch (LogicOp)
    {
    case ELogicOp::Clear:        return "Clear";
    case ELogicOp::Set:          return "Set";
    case ELogicOp::Copy:         return "Copy";
    case ELogicOp::CopyInverted: return "CopyInverted";
    case ELogicOp::Noop:         return "Noop";
    case ELogicOp::Invert:       return "Invert";
    case ELogicOp::And:          return "And";
    case ELogicOp::Nand:         return "Nand";
    case ELogicOp::Or:           return "Or";
    case ELogicOp::Nor:          return "Nor";
    case ELogicOp::Xor:          return "Xor";
    case ELogicOp::Equiv:        return "Equiv";
    case ELogicOp::AndReverse:   return "AndReverse";
    case ELogicOp::AndInverted:  return "AndInverted";
    case ELogicOp::OrReverse:    return "OrReverse";
    case ELogicOp::OrInverted:   return "OrInverted";
    default: return "Unknown";
    }
}

enum EColorWriteFlag : UInt8
{
    ColorWriteFlag_None  = 0,
    ColorWriteFlag_Red   = 1,
    ColorWriteFlag_Green = 2,
    ColorWriteFlag_Blue  = 4,
    ColorWriteFlag_Alpha = 8,
    ColorWriteFlag_All = (((ColorWriteFlag_Red | ColorWriteFlag_Green) | ColorWriteFlag_Blue) | ColorWriteFlag_Alpha)
};

struct RenderTargetWriteState
{
    RenderTargetWriteState() = default;

    RenderTargetWriteState(UInt32 InMask)
        : Mask(InMask)
    {
    }

    Bool WriteNone() const { return Mask == ColorWriteFlag_None; }
    Bool WriteRed() const { return (Mask & ColorWriteFlag_Red); }
    Bool WriteGreen() const { return (Mask & ColorWriteFlag_Green); }
    Bool WriteBlue() const { return (Mask & ColorWriteFlag_Blue); }
    Bool WriteAlpha() const { return (Mask & ColorWriteFlag_Alpha); }
    Bool WriteAll() const { return Mask == ColorWriteFlag_All; }

    UInt8 Mask = ColorWriteFlag_All;
};

struct RenderTargetBlendState
{
    EBlend   SrcBlend       = EBlend::One;
    EBlend   DestBlend      = EBlend::Zero;
    EBlendOp BlendOp        = EBlendOp::Add;
    EBlend   SrcBlendAlpha  = EBlend::One;
    EBlend   DestBlendAlpha = EBlend::Zero;
    EBlendOp BlendOpAlpha   = EBlendOp::Add;;
    ELogicOp LogicOp        = ELogicOp::Noop;
    Bool     BlendEnable    = false;
    Bool     LogicOpEnable  = false;
    RenderTargetWriteState RenderTargetWriteMask;
};

struct BlendStateCreateInfo
{
    Bool AlphaToCoverageEnable  = false;
    Bool IndependentBlendEnable = false;
    RenderTargetBlendState RenderTarget[8];
};

class BlendState : public Resource
{
};

enum class EInputClassification
{
    Vertex   = 0,
    Instance = 1,
};

inline const Char* ToString(EInputClassification BlendOp)
{
    switch (BlendOp)
    {
    case EInputClassification::Vertex:   return "Vertex";
    case EInputClassification::Instance: return "Instance";
    default: return "Unknown";
    }
}

struct InputElement
{
    std::string          Semantic            = "";
    UInt32               SemanticIndex       = 0;
    EFormat              Format              = EFormat::Unknown;
    UInt32               InputSlot           = 0;
    UInt32               ByteOffset          = 0;
    EInputClassification InputClassification = EInputClassification::Vertex;
    UInt32               InstanceStepRate    = 0;
};

struct InputLayoutStateCreateInfo
{
    InputLayoutStateCreateInfo() = default;

    InputLayoutStateCreateInfo(const TArray<InputElement>& InElements)
        : Elements(InElements)
    {
    }

    InputLayoutStateCreateInfo(std::initializer_list<InputElement> InList)
        : Elements(InList)
    {
    }

    TArray<InputElement> Elements;
};

class InputLayoutState : public Resource
{
};

enum class EIndexBufferStripCutValue
{
    Disabled    = 0,
    _0xffff     = 1,
    _0xffffffff = 2
};

inline const Char* ToString(EIndexBufferStripCutValue IndexBufferStripCutValue)
{
    switch (IndexBufferStripCutValue)
    {
    case EIndexBufferStripCutValue::Disabled:    return "Disabled";
    case EIndexBufferStripCutValue::_0xffff:     return "0xffff";
    case EIndexBufferStripCutValue::_0xffffffff: return "0xffffffff";
    default: return "";
    }
}

struct PipelineRenderTargetFormats
{
    EFormat RenderTargetFormats[8];
    UInt32  NumRenderTargets   = 0;
    EFormat DepthStencilFormat = EFormat::Unknown;
};

struct GraphicsPipelineShaderState
{
    GraphicsPipelineShaderState() = default;

    GraphicsPipelineShaderState(VertexShader* InVertexShader, PixelShader* InPixelShader)
        : VertexShader(InVertexShader)
        , PixelShader(InPixelShader)
    {
    }

    VertexShader* VertexShader = nullptr;
    PixelShader*  PixelShader  = nullptr;
};

struct GraphicsPipelineStateCreateInfo
{
    InputLayoutState*  InputLayoutState  = nullptr;
    DepthStencilState* DepthStencilState = nullptr;
    RasterizerState*   RasterizerState   = nullptr;
    BlendState*        BlendState        = nullptr;
    UInt32                      SampleCount   = 1;
    UInt32                      SampleQuality = 0;
    UInt32                      SampleMask    = 0xffffffff;
    EIndexBufferStripCutValue   IBStripCutValue       = EIndexBufferStripCutValue::Disabled;
    EPrimitiveTopologyType      PrimitiveTopologyType = EPrimitiveTopologyType::Triangle;
    GraphicsPipelineShaderState ShaderState;
    PipelineRenderTargetFormats PipelineFormats;
};

class GraphicsPipelineState : public PipelineState
{
public:
    GraphicsPipelineState()  = default;
    ~GraphicsPipelineState() = default;

    virtual GraphicsPipelineState* AsGraphics() override { return this; }
};

struct ComputePipelineStateCreateInfo
{
    ComputePipelineStateCreateInfo() = default;

    ComputePipelineStateCreateInfo(ComputeShader* InShader)
        : Shader(InShader)
    {
    }

    ComputeShader* Shader = nullptr;
};

class ComputePipelineState : public PipelineState
{
public:
    ComputePipelineState()  = default;
    ~ComputePipelineState() = default;
    
    virtual ComputePipelineState* AsCompute() override { return this; }
};

class RayTracingPipelineState : public PipelineState
{
public:
    RayTracingPipelineState()  = default;
    ~RayTracingPipelineState() = default;
    
    virtual RayTracingPipelineState* AsRayTracing() override { return this; }
};