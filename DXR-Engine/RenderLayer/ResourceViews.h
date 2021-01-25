#pragma once
#include "Resource.h"

enum EShaderResourceViewType
{
    ShaderResourceViewType_Texture          = 0,
    ShaderResourceViewType_VertexBuffer     = 1,
    ShaderResourceViewType_IndexBuffer      = 2,
    ShaderResourceViewType_StructuredBuffer = 3,
};

struct TextureShaderResourceViewCreateInfo
{
    Texture* Texture        = nullptr;
    EFormat  Format         = EFormat::Format_Unknown;
    UInt32   MipLevel       = 0;
    UInt32   NumMipLevels   = 0;
    UInt32   ArraySlice     = 0;
    UInt32   NumArraySlices = 0;
    Float    MipBias        = 0.0f;
};

struct VertexBufferShaderResourceViewCreateInfo
{
    VertexBuffer* Buffer = nullptr;
    UInt32 FirstElement  = 0;
    UInt32 NumElements   = 0;
};

struct IndexBufferShaderResourceViewCreateInfo
{
    IndexBuffer* Buffer = nullptr;
    UInt32 FirstElement = 0;
    UInt32 NumElements  = 0;
};

struct StructuredBufferShaderResourceViewCreateInfo
{
    StructuredBuffer* Buffer = nullptr;
    UInt32 FirstElement = 0;
    UInt32 NumElements  = 0;
};

struct ShaderResourceViewCreateInfo
{
    ShaderResourceViewCreateInfo(Texture* InTexture)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = InTexture->GetFormat();
        Texture.MipBias        = 0.0f;
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    ShaderResourceViewCreateInfo(VertexBuffer* InVertexBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = 0;
        VertexBuffer.NumElements  = InVertexBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(IndexBuffer* InIndexBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = 0;
        IndexBuffer.NumElements  = InIndexBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(StructuredBuffer* InStructuredBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = 0;
        StructuredBuffer.NumElements  = InStructuredBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(Texture* InTexture, EFormat Format)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = Format;
        Texture.MipBias        = 0.0f;
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    ShaderResourceViewCreateInfo(Texture* InTexture, EFormat Format, UInt32 ArraySlice, UInt32 NumArraySlices, UInt32 MipLevel, UInt32 NumMipLevels, Float MipBias)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = ArraySlice;
        Texture.NumArraySlices = NumArraySlices;
        Texture.Format         = Format;
        Texture.MipBias        = MipBias;
        Texture.MipLevel       = MipLevel;
        Texture.NumMipLevels   = NumMipLevels;
    }

    ShaderResourceViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = FirstElement;
        VertexBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = FirstElement;
        IndexBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = FirstElement;
        StructuredBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(const ShaderResourceViewCreateInfo& Other)
        : Type(Other.Type)
    {
        if (Type == EShaderResourceViewType::ShaderResourceViewType_Texture)
        {
            Texture = Other.Texture;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }
    }

    TextureShaderResourceViewCreateInfo* AsTextureShaderResourceView()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_Texture);
        return &Texture;
    }

    const TextureShaderResourceViewCreateInfo* AsTextureShaderResourceView() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_Texture);
        return &Texture;
    }

    VertexBufferShaderResourceViewCreateInfo* VertexBufferShaderResourceView()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer);
        return &VertexBuffer;
    }

    const VertexBufferShaderResourceViewCreateInfo* VertexBufferShaderResourceView() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer);
        return &VertexBuffer;
    }

    IndexBufferShaderResourceViewCreateInfo* IndexBufferShaderResourceView()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer);
        return &IndexBuffer;
    }

    const IndexBufferShaderResourceViewCreateInfo* IndexBufferShaderResourceView() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer);
        return &IndexBuffer;
    }
    
    StructuredBufferShaderResourceViewCreateInfo* StructuredBufferShaderResourceView()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    const StructuredBufferShaderResourceViewCreateInfo* StructuredBufferShaderResourceView() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    ShaderResourceViewCreateInfo& operator=(const ShaderResourceViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == EShaderResourceViewType::ShaderResourceViewType_Texture)
        {
            Texture = Other.Texture;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }

        return *this;
    }

    EShaderResourceViewType Type;
    union
    {
        TextureShaderResourceViewCreateInfo          Texture;
        VertexBufferShaderResourceViewCreateInfo     VertexBuffer;
        IndexBufferShaderResourceViewCreateInfo      IndexBuffer;
        StructuredBufferShaderResourceViewCreateInfo StructuredBuffer;
    };
};

class ShaderResourceView : public PipelineResource
{
};

enum EUnorderedAccessViewType
{
    UnorderedAccessViewType_Texture = 0,
    UnorderedAccessViewType_VertexBuffer = 1,
    UnorderedAccessViewType_IndexBuffer = 2,
    UnorderedAccessViewType_StructuredBuffer = 3,
};

struct TextureUnorderedAccessViewCreateInfo
{
    Texture* Texture        = nullptr;
    EFormat  Format         = EFormat::Format_Unknown;
    UInt32   MipLevel       = 0;
    UInt32   NumMipLevels   = 0;
    UInt32   ArraySlice     = 0;
    UInt32   NumArraySlices = 0;
};

struct VertexBufferUnorderedAccessViewCreateInfo
{
    VertexBuffer* Buffer = nullptr;
    UInt32 FirstElement  = 0;
    UInt32 NumElements   = 0;
};

struct IndexBufferUnorderedAccessViewCreateInfo
{
    IndexBuffer* Buffer = nullptr;
    UInt32 FirstElement = 0;
    UInt32 NumElements  = 0;
};

struct StructuredBufferUnorderedAccessViewCreateInfo
{
    StructuredBuffer* Buffer = nullptr;
    UInt32 FirstElement      = 0;
    UInt32 NumElements       = 0;
};

struct UnorderedAccessViewCreateInfo
{
    UnorderedAccessViewCreateInfo(Texture* InTexture)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = InTexture->GetFormat();
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    UnorderedAccessViewCreateInfo(VertexBuffer* InVertexBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = 0;
        VertexBuffer.NumElements  = InVertexBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(IndexBuffer* InIndexBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = 0;
        IndexBuffer.NumElements  = InIndexBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(StructuredBuffer* InStructuredBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = 0;
        StructuredBuffer.NumElements  = InStructuredBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(Texture* InTexture, EFormat Format)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = Format;
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    UnorderedAccessViewCreateInfo(Texture* InTexture, EFormat Format, UInt32 ArraySlice, UInt32 NumArraySlices, UInt32 MipLevel, UInt32 NumMipLevels)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
    {
        Texture.Texture        = InTexture;
        Texture.ArraySlice     = ArraySlice;
        Texture.NumArraySlices = NumArraySlices;
        Texture.Format         = Format;
        Texture.MipLevel       = MipLevel;
        Texture.NumMipLevels   = NumMipLevels;
    }

    UnorderedAccessViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = FirstElement;
        VertexBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = FirstElement;
        IndexBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = FirstElement;
        StructuredBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(const UnorderedAccessViewCreateInfo& Other)
        : Type(Other.Type)
    {
        if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
        {
            Texture = Other.Texture;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }
    }

    TextureUnorderedAccessViewCreateInfo* AsTextureUnorderedAccessView()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture);
        return &Texture;
    }

    const TextureUnorderedAccessViewCreateInfo* AsTextureUnorderedAccessView() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture);
        return &Texture;
    }

    VertexBufferUnorderedAccessViewCreateInfo* VertexBufferUnorderedAccessView()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer);
        return &VertexBuffer;
    }

    const VertexBufferUnorderedAccessViewCreateInfo* VertexBufferUnorderedAccessView() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer);
        return &VertexBuffer;
    }

    IndexBufferUnorderedAccessViewCreateInfo* IndexBufferUnorderedAccessView()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer);
        return &IndexBuffer;
    }

    const IndexBufferUnorderedAccessViewCreateInfo* IndexBufferUnorderedAccessView() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer);
        return &IndexBuffer;
    }

    StructuredBufferUnorderedAccessViewCreateInfo* StructuredBufferUnorderedAccessView()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    const StructuredBufferUnorderedAccessViewCreateInfo* StructuredBufferUnorderedAccessView() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    UnorderedAccessViewCreateInfo& operator=(const UnorderedAccessViewCreateInfo& Other)
    {
        Type = Other.Type;
        if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
        {
            Texture = Other.Texture;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
        {
            VertexBuffer = Other.VertexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
        {
            IndexBuffer = Other.IndexBuffer;
        }
        else if (Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
        {
            StructuredBuffer = Other.StructuredBuffer;
        }

        return *this;
    }

    EUnorderedAccessViewType Type;
    union
    {
        TextureUnorderedAccessViewCreateInfo          Texture;
        VertexBufferUnorderedAccessViewCreateInfo     VertexBuffer;
        IndexBufferUnorderedAccessViewCreateInfo      IndexBuffer;
        StructuredBufferUnorderedAccessViewCreateInfo StructuredBuffer;
    };
};

class UnorderedAccessView : public PipelineResource
{
};

struct DepthStencilViewCreateInfo
{
    DepthStencilViewCreateInfo(Texture* InTexture)
        : Texture(InTexture)
        , Format(InTexture->GetFormat())
        , MipLevel(0)
        , ArraySlice(0)
        , NumArraySlices(InTexture->GetArrayCount())
    {
    }

    DepthStencilViewCreateInfo(Texture* InTexture, EFormat InFormat)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(0)
        , ArraySlice(0)
        , NumArraySlices(InTexture->GetArrayCount())
    {
    }

    DepthStencilViewCreateInfo(Texture* InTexture, EFormat InFormat, UInt32 InArraySlice, UInt32 InNumArraySlices, UInt32 MipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(MipLevel)
        , ArraySlice(InArraySlice)
        , NumArraySlices(InNumArraySlices)
    {
    }

    DepthStencilViewCreateInfo(const DepthStencilViewCreateInfo& Other)
        : Texture(Other.Texture)
        , Format(Other.Format)
        , MipLevel(Other.MipLevel)
        , ArraySlice(Other.ArraySlice)
        , NumArraySlices(Other.NumArraySlices)
    {
    }

    DepthStencilViewCreateInfo& operator=(const DepthStencilViewCreateInfo& Other)
    {
        Memory::Memcpy(this, &Other);
        return *this;
    }

    Texture* Texture        = nullptr;
    EFormat  Format         = EFormat::Format_Unknown;
    UInt32   MipLevel       = 0;
    UInt32   ArraySlice     = 0;
    UInt32   NumArraySlices = 0;
};

class DepthStencilView : public PipelineResource
{
};

// TODO: Add support for texelbuffers
struct RenderTargetViewCreateInfo
{
    RenderTargetViewCreateInfo(Texture* InTexture)
        : Texture(InTexture)
        , Format(InTexture->GetFormat())
        , MipLevel(0)
        , ArraySlice(0)
        , NumArraySlices(InTexture->GetArrayCount())
    {
    }

    RenderTargetViewCreateInfo(Texture* InTexture, EFormat InFormat)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(0)
        , ArraySlice(0)
        , NumArraySlices(InTexture->GetArrayCount())
    {
    }

    RenderTargetViewCreateInfo(Texture* InTexture, EFormat InFormat, UInt32 InArraySlice, UInt32 InNumArraySlices, UInt32 MipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(MipLevel)
        , ArraySlice(InArraySlice)
        , NumArraySlices(InNumArraySlices)
    {
    }

    RenderTargetViewCreateInfo(const RenderTargetViewCreateInfo& Other)
        : Texture(Other.Texture)
        , Format(Other.Format)
        , MipLevel(Other.MipLevel)
        , ArraySlice(Other.ArraySlice)
        , NumArraySlices(Other.NumArraySlices)
    {
    }

    RenderTargetViewCreateInfo& operator=(const RenderTargetViewCreateInfo& Other)
    {
        Memory::Memcpy(this, &Other);
        return *this;
    }

    Texture* Texture = nullptr;
    EFormat  Format = EFormat::Format_Unknown;
    UInt32   MipLevel = 0;
    UInt32   ArraySlice = 0;
    UInt32   NumArraySlices = 0;
};

class RenderTargetView : public PipelineResource
{
};