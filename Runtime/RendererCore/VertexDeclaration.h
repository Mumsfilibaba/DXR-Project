#pragma once
#include "Core/Core.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "RHI/RHITypes.h"

constexpr uint8 VERTEX_MAX_STREAMS = 4;

struct EVertexStreamIndex
{
    enum Type : uint8
    {
        Position   = 0,
        Attributes = 1,
        Color      = 2,
    };
};

enum class EVertexAttributeFlags : uint16
{
    None         = 0,
    Position     = FLAG(0),
    TangentBasis = FLAG(1),
    TexCoord0    = FLAG(2),
    Color        = FLAG(3),
};

ENUM_CLASS_OPERATORS(EVertexAttributeFlags);

constexpr EVertexAttributeFlags VERTEX_ATTRIBUTES_ALL =
    EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis | EVertexAttributeFlags::TexCoord0 | EVertexAttributeFlags::Color;

constexpr uint16 NUM_VERTEX_DECLARATIONS = UnderlyingTypeValue(VERTEX_ATTRIBUTES_ALL) + 1;

enum class EVertexElement : uint8
{
    Position,
    Normal,
    Tangent,
    TexCoord0,
    Color,
};

struct FVertexAttributeInfo
{
    EVertexElement        Element       = EVertexElement::Position;
    EVertexAttributeFlags Attribute     = EVertexAttributeFlags::None;
    EFormat               Format        = EFormat::Unknown;
    const CHAR*           Semantic      = nullptr;
    uint32                SemanticIndex = 0;
    uint8                 StreamIndex   = 0;
    uint8                 ByteOffset    = 0;
};

class RENDERERCORE_API FVertexDeclaration
{
public:

    /** @brief Position, Normal, Tangent, TexCoord0 and Color: the widest element list a declaration can hold. */
    static constexpr uint8 MAX_ATTRIBUTES = 5;

    /** @return The declaration every importer produces today: Position, TangentBasis and TexCoord0. */
    static const FVertexDeclaration& GetStandardStaticMesh();

    /**
    * @brief Looks up the interned declaration for an attribute set.
    * @param Attributes Attributes the mesh carries.
    * @return The interned declaration. The reference is stable for the lifetime of the process.
    */
    static const FVertexDeclaration& GetOrCreate(EVertexAttributeFlags Attributes);

    /**
    * @brief Looks up an interned declaration by the ID that was folded into a key.
    * @param InID ID previously returned by GetID().
    * @return The interned declaration.
    */
    static const FVertexDeclaration& GetByID(uint8 InID);

    FVertexDeclaration();
    ~FVertexDeclaration();

    /**
     * @brief Tests whether this declaration carries everything a caller needs.
     * @param Required Attributes the caller intends to read.
     * @return True when every required attribute is present.
     */
    bool HasAttributes(EVertexAttributeFlags Required) const
    {
        return (AttributeFlags & Required) == Required;
    }

    /**
     * @brief Returns the byte stride of one stream.
     * @param StreamIndex Stream to query.
     * @return The stride, or zero when this declaration does not use the stream.
     */
    uint16 GetStreamStride(uint8 StreamIndex) const
    {
        return StreamIndex < VERTEX_MAX_STREAMS ? StreamStrides[StreamIndex] : uint16(0);
    }

    EVertexAttributeFlags GetAttributeFlags() const
    {
        return AttributeFlags;
    }

    /** @return The number of streams this declaration uses, counting unused leading streams. */
    uint8 GetNumStreams() const
    {
        return NumStreams;
    }

    /** @return A stable ID, small enough to pack into a pipeline-state key. */
    uint8 GetID() const
    {
        return ID;
    }

    TArrayView<const FVertexAttributeInfo> GetAttributes() const
    {
        return TArrayView<const FVertexAttributeInfo>(Attributes, NumAttributes);
    }

    bool operator==(const FVertexDeclaration& Other) const
    {
        return AttributeFlags == Other.AttributeFlags;
    }

    bool operator!=(const FVertexDeclaration& Other) const
    {
        return !(*this == Other);
    }

private:
    static const FVertexDeclaration* GetRegistry();
    
    void Build(EVertexAttributeFlags InAttributes);

    FVertexAttributeInfo  Attributes[MAX_ATTRIBUTES];
    uint16                StreamStrides[VERTEX_MAX_STREAMS];
    EVertexAttributeFlags AttributeFlags;
    uint8                 NumAttributes;
    uint8                 NumStreams;
    uint8                 ID;
};
