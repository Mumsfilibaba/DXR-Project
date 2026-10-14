#pragma once
#include "Core/Core.h"
#include "Core/Templates/Utility.h"
#include "Core/Templates/ObjectHandling.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Templates/Utility/EnumOperators.h"

constexpr uint8 MESH_MAX_STREAMS = 4;

struct EMeshStream
{
    enum Type : uint8
    {
        Position   = 0,
        Attributes = 1,
        Color      = 2,
    };
};

enum class EMeshVertexAttributes : uint16
{
    None         = 0,
    Position     = FLAG(0),
    TangentBasis = FLAG(1),
    TexCoord0    = FLAG(2),
    Color        = FLAG(3),
};

ENUM_CLASS_OPERATORS(EMeshVertexAttributes);

constexpr EMeshVertexAttributes MESH_VERTEX_ATTRIBUTES_ALL =
    EMeshVertexAttributes::Position | 
    EMeshVertexAttributes::TangentBasis | 
    EMeshVertexAttributes::TexCoord0 | 
    EMeshVertexAttributes::Color;

constexpr uint16 NUM_MESH_VERTEX_ATTRIBUTE_SETS = UnderlyingTypeValue(MESH_VERTEX_ATTRIBUTES_ALL) + 1;

enum class EMeshVertexElement : uint8
{
    Position,
    Normal,
    Tangent,
    TexCoord0,
    Color,
};

enum class EMeshElementEncoding : uint8
{
    Float3,
    Snorm16x4,
    Float2,
    Unorm8x4,
};

struct FMeshStreamElement
{
    EMeshVertexElement    Element     = EMeshVertexElement::Position;
    EMeshVertexAttributes Attribute   = EMeshVertexAttributes::None;
    EMeshElementEncoding  Encoding    = EMeshElementEncoding::Float3;
    uint8                 StreamIndex = 0;
    uint8                 ByteOffset  = 0;
};

class ASSETCORE_API FMeshStreamLayout
{
public:

    /** @brief Position, Normal, Tangent, TexCoord0 and Color: the widest element list a layout can hold */
    static constexpr uint8 MAX_ELEMENTS = 5;

    /** @return Returns the layout every importer produces today: Position, TangentBasis and TexCoord0 */
    static const FMeshStreamLayout& GetStandardStaticMesh();

    /** @return Returns the interned layout for an attribute set, stable for the lifetime of the process */
    static const FMeshStreamLayout& GetOrCreate(EMeshVertexAttributes Attributes);

    /** @return Returns the bytes an element of the encoding takes in its stream */
    NODISCARD static uint8 GetEncodingSize(EMeshElementEncoding Encoding);

public:
    FMeshStreamLayout();
    ~FMeshStreamLayout();

    /** @return Returns the byte stride of one stream, or zero when the layout doesn't use it */
    FORCEINLINE uint16 GetStreamStride(uint8 StreamIndex) const
    {
        return StreamIndex < MESH_MAX_STREAMS ? StreamStrides[StreamIndex] : uint16(0);
    }

    FORCEINLINE EMeshVertexAttributes GetAttributeFlags() const
    {
        return AttributeFlags;
    }

    /** @return Returns the number of streams the layout uses, counting unused leading streams */
    FORCEINLINE uint8 GetNumStreams() const
    {
        return NumStreams;
    }

    FORCEINLINE TArrayView<const FMeshStreamElement> GetElements() const
    {
        return TArrayView<const FMeshStreamElement>(Elements, NumElements);
    }

private:
    static const FMeshStreamLayout* GetRegistry();

    void Build(EMeshVertexAttributes InAttributes);

    FMeshStreamElement  Elements[MAX_ELEMENTS];
    uint16                StreamStrides[MESH_MAX_STREAMS];
    EMeshVertexAttributes AttributeFlags;
    uint8                 NumElements;
    uint8                 NumStreams;
};
