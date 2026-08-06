#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/Asserts.h"
#include "RendererCore/VertexDeclaration.h"

struct FVertexElementTemplate
{
    EVertexElement        Element;
    EVertexAttributeFlags Attribute;
    EFormat               Format;
    const CHAR*           Semantic;
    uint32                SemanticIndex;
    uint8                 StreamIndex;
};

static constexpr FVertexElementTemplate GVertexElementTemplates[] =
{
    { EVertexElement::Position,  EVertexAttributeFlags::Position,     EFormat::R32G32B32_Float,    "POSITION", 0, EVertexStreamIndex::Position   },
    { EVertexElement::Normal,    EVertexAttributeFlags::TangentBasis, EFormat::R16G16B16A16_Snorm, "NORMAL",   0, EVertexStreamIndex::Attributes },
    { EVertexElement::Tangent,   EVertexAttributeFlags::TangentBasis, EFormat::R16G16B16A16_Snorm, "TANGENT",  0, EVertexStreamIndex::Attributes },
    { EVertexElement::TexCoord0, EVertexAttributeFlags::TexCoord0,    EFormat::R32G32_Float,       "TEXCOORD", 0, EVertexStreamIndex::Attributes },
    { EVertexElement::Color,     EVertexAttributeFlags::Color,        EFormat::R8G8B8A8_Unorm,     "COLOR",    0, EVertexStreamIndex::Color      },
};

static_assert(ARRAY_COUNT(GVertexElementTemplates) == FVertexDeclaration::MAX_ATTRIBUTES,
    "FVertexDeclaration::MAX_ATTRIBUTES must hold every element a declaration can contain");

FVertexDeclaration::FVertexDeclaration()
    : Attributes()
    , StreamStrides()
    , AttributeFlags(EVertexAttributeFlags::None)
    , NumAttributes(0)
    , NumStreams(0)
    , ID(0)
{
}

FVertexDeclaration::~FVertexDeclaration() = default;

void FVertexDeclaration::Build(EVertexAttributeFlags InAttributes)
{
    AttributeFlags = InAttributes;
    NumAttributes  = 0;
    NumStreams     = 0;

    Memory::Memzero(StreamStrides, sizeof(StreamStrides));

    for (const FVertexElementTemplate& Element : GVertexElementTemplates)
    {
        if (!IsEnumFlagSet(InAttributes, Element.Attribute))
        {
            continue;
        }

        FVertexAttributeInfo& Info = Attributes[NumAttributes];
        Info.Element       = Element.Element;
        Info.Attribute     = Element.Attribute;
        Info.Format        = Element.Format;
        Info.Semantic      = Element.Semantic;
        Info.SemanticIndex = Element.SemanticIndex;
        Info.StreamIndex   = Element.StreamIndex;
        Info.ByteOffset    = static_cast<uint8>(StreamStrides[Element.StreamIndex]);

        StreamStrides[Element.StreamIndex] += static_cast<uint16>(GetByteStrideFromFormat(Element.Format));
        NumStreams = Math::Max<uint8>(NumStreams, Element.StreamIndex + 1);

        NumAttributes++;
    }

    // The attribute set is small enough to index the registry directly, so it doubles as the ID.
    ID = static_cast<uint8>(UnderlyingTypeValue(InAttributes));
}

const FVertexDeclaration* FVertexDeclaration::GetRegistry()
{
    static const FVertexDeclaration* Registry = []() -> const FVertexDeclaration*
    {
        static FVertexDeclaration Declarations[NUM_VERTEX_DECLARATIONS];
        for (uint16 Mask = 0; Mask < NUM_VERTEX_DECLARATIONS; ++Mask)
        {
            Declarations[Mask].Build(static_cast<EVertexAttributeFlags>(Mask));
        }

        return Declarations;
    }();

    return Registry;
}

const FVertexDeclaration& FVertexDeclaration::GetStandardStaticMesh()
{
    return GetOrCreate(EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis | EVertexAttributeFlags::TexCoord0);
}

const FVertexDeclaration& FVertexDeclaration::GetOrCreate(EVertexAttributeFlags InAttributes)
{
    const uint16 Mask = UnderlyingTypeValue(InAttributes & VERTEX_ATTRIBUTES_ALL);
    CHECK(Mask == UnderlyingTypeValue(InAttributes));

    return GetRegistry()[Mask];
}

const FVertexDeclaration& FVertexDeclaration::GetByID(uint8 InID)
{
    CHECK(InID < NUM_VERTEX_DECLARATIONS);
    return GetRegistry()[InID];
}
