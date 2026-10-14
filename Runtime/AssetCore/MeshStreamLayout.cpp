#include "AssetCore/MeshStreamLayout.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/Asserts.h"

struct FVertexElementTemplate
{
    EMeshVertexElement         Element;
    EMeshVertexAttributes  Attribute;
    EMeshElementEncoding Encoding;
    uint8                  StreamIndex;
};

static constexpr FVertexElementTemplate GVertexElementTemplates[] =
{
    { EMeshVertexElement::Position,  EMeshVertexAttributes::Position,     EMeshElementEncoding::Float3,    EMeshStream::Position   },
    { EMeshVertexElement::Normal,    EMeshVertexAttributes::TangentBasis, EMeshElementEncoding::Snorm16x4, EMeshStream::Attributes },
    { EMeshVertexElement::Tangent,   EMeshVertexAttributes::TangentBasis, EMeshElementEncoding::Snorm16x4, EMeshStream::Attributes },
    { EMeshVertexElement::TexCoord0, EMeshVertexAttributes::TexCoord0,    EMeshElementEncoding::Float2,    EMeshStream::Attributes },
    { EMeshVertexElement::Color,     EMeshVertexAttributes::Color,        EMeshElementEncoding::Unorm8x4,  EMeshStream::Color      },
};

static_assert(ARRAY_COUNT(GVertexElementTemplates) == FMeshStreamLayout::MAX_ELEMENTS, "FMeshStreamLayout::MAX_ELEMENTS must hold every element a layout can contain");

const FMeshStreamLayout& FMeshStreamLayout::GetStandardStaticMesh()
{
    return GetOrCreate(EMeshVertexAttributes::Position | EMeshVertexAttributes::TangentBasis | EMeshVertexAttributes::TexCoord0);
}

const FMeshStreamLayout& FMeshStreamLayout::GetOrCreate(EMeshVertexAttributes InAttributes)
{
    const uint16 Mask = UnderlyingTypeValue(InAttributes & MESH_VERTEX_ATTRIBUTES_ALL);
    CHECK(Mask == UnderlyingTypeValue(InAttributes));

    return GetRegistry()[Mask];
}

uint8 FMeshStreamLayout::GetEncodingSize(EMeshElementEncoding Encoding)
{
    switch (Encoding)
    {
        case EMeshElementEncoding::Float3:    return 12;
        case EMeshElementEncoding::Snorm16x4: return 8;
        case EMeshElementEncoding::Float2:    return 8;
        case EMeshElementEncoding::Unorm8x4:  return 4;
        default:                                return 0;
    }
}

const FMeshStreamLayout* FMeshStreamLayout::GetRegistry()
{
    static const FMeshStreamLayout* Registry = []() -> const FMeshStreamLayout*
    {
        static FMeshStreamLayout Layouts[NUM_MESH_VERTEX_ATTRIBUTE_SETS];
        for (uint16 Mask = 0; Mask < NUM_MESH_VERTEX_ATTRIBUTE_SETS; ++Mask)
        {
            Layouts[Mask].Build(static_cast<EMeshVertexAttributes>(Mask));
        }

        return Layouts;
    }();

    return Registry;
}

FMeshStreamLayout::FMeshStreamLayout()
    : Elements()
    , StreamStrides()
    , AttributeFlags(EMeshVertexAttributes::None)
    , NumElements(0)
    , NumStreams(0)
{
}

FMeshStreamLayout::~FMeshStreamLayout() = default;

void FMeshStreamLayout::Build(EMeshVertexAttributes InAttributes)
{
    AttributeFlags = InAttributes;
    NumElements    = 0;
    NumStreams     = 0;

    Memory::Memzero(StreamStrides, sizeof(StreamStrides));

    for (const FVertexElementTemplate& Template : GVertexElementTemplates)
    {
        if (!IsEnumFlagSet(InAttributes, Template.Attribute))
        {
            continue;
        }

        FMeshStreamElement& Element = Elements[NumElements++];
        Element.Element     = Template.Element;
        Element.Attribute   = Template.Attribute;
        Element.Encoding    = Template.Encoding;
        Element.StreamIndex = Template.StreamIndex;
        Element.ByteOffset  = static_cast<uint8>(StreamStrides[Template.StreamIndex]);

        StreamStrides[Template.StreamIndex] += GetEncodingSize(Template.Encoding);
        NumStreams = Math::Max<uint8>(NumStreams, Template.StreamIndex + 1);
    }
}
