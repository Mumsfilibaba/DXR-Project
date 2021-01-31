#pragma once
#include "Resources.h"

#include "Memory/Memory.h"

struct ShaderResourceViewCreateInfo
{
    enum class EType
    {
        Texture2D        = 1,
        Texture2DArray   = 2,
        TextureCube      = 3,
        TextureCubeArray = 4,
        Texture3D        = 5,
        VertexBuffer     = 6,
        IndexBuffer      = 7,
        StructuredBuffer = 8,
    };

    struct Texture2D
    {
        Texture2D* Texture   = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  MipLevel     = 0;
        UInt32  NumMipLevels = 0;
        Float   MinMipBias   = 0.0f;
    };

    struct Texture2DArray
    {
        Texture2DArray* Texture = nullptr;
        EFormat Format          = EFormat::Unknown;
        UInt32  MipLevel        = 0;
        UInt32  NumMipLevels    = 0;
        UInt32  ArraySlice      = 0;
        UInt32  NumArraySlices  = 0;
        Float   MinMipBias      = 0.0f;
    };

    struct TextureCube
    {
        TextureCube* Texture = nullptr;
        EFormat Format       = EFormat::Unknown;
        UInt32  MipLevel     = 0;
        UInt32  NumMipLevels = 0;
        Float   MinMipBias   = 0.0f;
    };

    struct TextureCubeArray
    {
        TextureCubeArray* Texture = nullptr;
        EFormat Format            = EFormat::Unknown;
        UInt32  MipLevel          = 0;
        UInt32  NumMipLevels      = 0;
        Float   MinMipBias        = 0.0f;
    };

    struct Texture3D
    {
        Texture3D* Texture     = nullptr;
        EFormat Format         = EFormat::Unknown;
        UInt32  MipLevel       = 0;
        UInt32  NumMipLevels   = 0;
        UInt32  DepthSlice     = 0;
        UInt32  NumDepthSlices = 0;
        Float   MinMipBias     = 0.0f;
    };

    struct VertexBuffer
    {
        VertexBuffer* Buffer = nullptr;
        UInt32 FirstElement  = 0;
        UInt32 NumElements   = 0;
    };

    struct IndexBuffer
    {
        IndexBuffer* Buffer = nullptr;
        UInt32 FirstElement = 0;
        UInt32 NumElements  = 0;
    };

    struct StructuredBuffer
    {
        StructuredBuffer* Buffer = nullptr;
        UInt32 FirstElement = 0;
        UInt32 NumElements  = 0;
    };




    ShaderResourceViewCreateInfo(Texture* InTexture)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = InTexture->GetFormat();
        Texture.MinMipBias     = 0.0f;
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    ShaderResourceViewCreateInfo(VertexBuffer* InVertexBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
        , Texture()
    {
        VALIDATE(InVertexBuffer != nullptr);

        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = 0;
        VertexBuffer.NumElements  = InVertexBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(IndexBuffer* InIndexBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
        , Texture()
    {
        VALIDATE(InIndexBuffer != nullptr);

        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = 0;
        IndexBuffer.NumElements  = InIndexBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(StructuredBuffer* InStructuredBuffer)
        : Type(EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
        , Texture()
    {
        VALIDATE(InStructuredBuffer != nullptr);

        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = 0;
        StructuredBuffer.NumElements  = InStructuredBuffer->GetNumElements();
    }

    ShaderResourceViewCreateInfo(Texture* InTexture, EFormat Format)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = Format;
        Texture.MinMipBias     = 0.0f;
        Texture.MipLevel       = 0;
        Texture.NumMipLevels   = InTexture->GetMipLevels();
    }

    ShaderResourceViewCreateInfo(Texture* InTexture, EFormat Format, UInt32 MipLevel, UInt32 NumMipLevels, Float MinMipBias)
        : Type(EShaderResourceViewType::ShaderResourceViewType_Texture)
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture        = InTexture;
        Texture.ArraySlice     = 0;
        Texture.NumArraySlices = InTexture->GetArrayCount();
        Texture.Format         = Format;
        Texture.MinMipBias     = MinMipBias;
        Texture.MipLevel       = MipLevel;
        Texture.NumMipLevels   = NumMipLevels;
    }

    ShaderResourceViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_VertexBuffer)
        , Texture()
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = FirstElement;
        VertexBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_IndexBuffer)
        , Texture()
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = FirstElement;
        IndexBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer)
        , Texture()
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = FirstElement;
        StructuredBuffer.NumElements  = NumElements;
    }

    ShaderResourceViewCreateInfo(const ShaderResourceViewCreateInfo& Other)
        : Type(Other.Type)
        , Texture()
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

    TextureShaderResourceViewCreateInfo* GetTextureSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_Texture);
        return &Texture;
    }

    const TextureShaderResourceViewCreateInfo* GetTextureSRV() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_Texture);
        return &Texture;
    }

    VertexBufferShaderResourceViewCreateInfo* GetVertexBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer);
        return &VertexBuffer;
    }

    const VertexBufferShaderResourceViewCreateInfo* GetVertexBufferSRV() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_VertexBuffer);
        return &VertexBuffer;
    }

    IndexBufferShaderResourceViewCreateInfo* GetIndexBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer);
        return &IndexBuffer;
    }

    const IndexBufferShaderResourceViewCreateInfo* GetIndexBufferSRV() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_IndexBuffer);
        return &IndexBuffer;
    }
    
    StructuredBufferShaderResourceViewCreateInfo* GetStructuredBufferSRV()
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    const StructuredBufferShaderResourceViewCreateInfo* GetStructuredBufferSRV() const
    {
        VALIDATE(Type == EShaderResourceViewType::ShaderResourceViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    EShaderResourceViewType GetType() const
    {
        return Type;
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

private:
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

enum class EUnorderedAccessViewType
{
    UnorderedAccessViewType_Texture          = 0,
    UnorderedAccessViewType_VertexBuffer     = 1,
    UnorderedAccessViewType_IndexBuffer      = 2,
    UnorderedAccessViewType_StructuredBuffer = 3,
};

struct TextureUnorderedAccessViewCreateInfo
{
    Texture* Texture               = nullptr;
    EFormat  Format                = EFormat::Format_Unknown;
    UInt32   MipLevel              = 0;
    UInt32   ArrayOrDepthSlice     = 0;
    UInt32   NumArrayOrDepthSlices = 0;
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
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture               = InTexture;
        Texture.ArrayOrDepthSlice     = 0;
        Texture.NumArrayOrDepthSlices = InTexture->GetArrayCount();
        Texture.Format                = InTexture->GetFormat();
        Texture.MipLevel              = 0;
    }

    UnorderedAccessViewCreateInfo(VertexBuffer* InVertexBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
        , Texture()
    {
        VALIDATE(InVertexBuffer != nullptr);

        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = 0;
        VertexBuffer.NumElements  = InVertexBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(IndexBuffer* InIndexBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
        , Texture()
    {
        VALIDATE(InIndexBuffer != nullptr);

        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = 0;
        IndexBuffer.NumElements  = InIndexBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(StructuredBuffer* InStructuredBuffer)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
        , Texture()
    {
        VALIDATE(InStructuredBuffer != nullptr);

        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = 0;
        StructuredBuffer.NumElements  = InStructuredBuffer->GetNumElements();
    }

    UnorderedAccessViewCreateInfo(Texture* InTexture, EFormat Format)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture               = InTexture;
        Texture.ArrayOrDepthSlice     = 0;
        Texture.NumArrayOrDepthSlices = InTexture->GetArrayCount();
        Texture.Format                = Format;
        Texture.MipLevel              = 0;
    }

    UnorderedAccessViewCreateInfo(Texture* InTexture, EFormat Format, UInt32 MipLevel)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
        , Texture()
    {
        VALIDATE(InTexture != nullptr);

        Texture.Texture               = InTexture;
        Texture.ArrayOrDepthSlice     = 0;
        Texture.NumArrayOrDepthSlices = InTexture->GetArrayCount();
        Texture.Format                = Format;
        Texture.MipLevel              = MipLevel;
    }

    UnorderedAccessViewCreateInfo(Texture* InTexture, EFormat Format, UInt32 ArraySlice, UInt32 NumArraySlices, UInt32 MipLevel)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_Texture)
        , Texture()
    {
        Texture.Texture               = InTexture;
        Texture.ArrayOrDepthSlice     = ArraySlice;
        Texture.NumArrayOrDepthSlices = NumArraySlices;
        Texture.Format                = Format;
        Texture.MipLevel              = MipLevel;
    }

    UnorderedAccessViewCreateInfo(VertexBuffer* InVertexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer)
        , Texture()
    {
        VertexBuffer.Buffer       = InVertexBuffer;
        VertexBuffer.FirstElement = FirstElement;
        VertexBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(IndexBuffer* InIndexBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer)
        , Texture()
    {
        IndexBuffer.Buffer       = InIndexBuffer;
        IndexBuffer.FirstElement = FirstElement;
        IndexBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(StructuredBuffer* InStructuredBuffer, UInt32 FirstElement, UInt32 NumElements)
        : Type(EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer)
        , Texture()
    {
        StructuredBuffer.Buffer       = InStructuredBuffer;
        StructuredBuffer.FirstElement = FirstElement;
        StructuredBuffer.NumElements  = NumElements;
    }

    UnorderedAccessViewCreateInfo(const UnorderedAccessViewCreateInfo& Other)
        : Type(Other.Type)
        , Texture()
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

    TextureUnorderedAccessViewCreateInfo* GetTextureUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture);
        return &Texture;
    }

    const TextureUnorderedAccessViewCreateInfo* GetTextureUAV() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_Texture);
        return &Texture;
    }

    VertexBufferUnorderedAccessViewCreateInfo* GetVertexBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer);
        return &VertexBuffer;
    }

    const VertexBufferUnorderedAccessViewCreateInfo* GetVertexBufferUAV() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_VertexBuffer);
        return &VertexBuffer;
    }

    IndexBufferUnorderedAccessViewCreateInfo* GetIndexBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer);
        return &IndexBuffer;
    }

    const IndexBufferUnorderedAccessViewCreateInfo* GetIndexBufferUAV() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_IndexBuffer);
        return &IndexBuffer;
    }

    StructuredBufferUnorderedAccessViewCreateInfo* GetStructuredBufferUAV()
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    const StructuredBufferUnorderedAccessViewCreateInfo* GetStructuredBufferUAV() const
    {
        VALIDATE(Type == EUnorderedAccessViewType::UnorderedAccessViewType_StructuredBuffer);
        return &StructuredBuffer;
    }

    EUnorderedAccessViewType GetType() const
    {
        return Type;
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

private:
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
    DepthStencilViewCreateInfo() = default;

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

    DepthStencilViewCreateInfo(Texture* InTexture, EFormat InFormat, UInt32 InFaceIndex, UInt32 InArraySlice, UInt32 InNumArraySlices, UInt32 MipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(MipLevel)
        , FaceIndex(InFaceIndex)
        , ArraySlice(InArraySlice)
        , NumArraySlices(InNumArraySlices)
    {
    }

    Texture* Texture  = nullptr;
    EFormat  Format   = EFormat::Format_Unknown;
    UInt32   MipLevel = 0;
    // TODO: Use enum instead
    UInt32   FaceIndex      = 0;
    UInt32   ArraySlice     = 0;
    UInt32   NumArraySlices = 0;
};

class DepthStencilView : public PipelineResource
{
};

// TODO: Add support for texelbuffers
struct RenderTargetViewCreateInfo
{
    RenderTargetViewCreateInfo() = default;

    RenderTargetViewCreateInfo(Texture* InTexture)
        : Texture(InTexture)
        , Format(InTexture->GetFormat())
        , MipLevel(0)
        , ArrayOrDepthSlice(0)
        , NumArrayOrDepthSlices(InTexture->GetArrayCount())
    {
    }

    RenderTargetViewCreateInfo(Texture* InTexture, EFormat InFormat)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(0)
        , ArrayOrDepthSlice(0)
        , NumArrayOrDepthSlices(InTexture->GetArrayCount())
    {
    }

    RenderTargetViewCreateInfo(Texture* InTexture, EFormat InFormat, UInt32 InFaceIndex, UInt32 InArraySlice, UInt32 InNumArraySlices, UInt32 InMipLevel)
        : Texture(InTexture)
        , Format(InFormat)
        , MipLevel(InMipLevel)
        , FaceIndex(InFaceIndex)
        , ArrayOrDepthSlice(InArraySlice)
        , NumArrayOrDepthSlices(InNumArraySlices)
    {
    }

    Texture* Texture  = nullptr;
    EFormat  Format   = EFormat::Format_Unknown;
    UInt32   MipLevel = 0;
    // TODO: Use enum instead
    UInt32   FaceIndex             = 0;
    UInt32   ArrayOrDepthSlice     = 0;
    UInt32   NumArrayOrDepthSlices = 0;
};

class RenderTargetView : public PipelineResource
{
};