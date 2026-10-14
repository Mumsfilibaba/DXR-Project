#include "Engine/Assets/MeshStreamLayouts.h"
#include "RHI/RHITypes.h"

static EVertexElement ToVertexElement(EMeshVertexElement Element)
{
    switch (Element)
    {
        case EMeshVertexElement::Position:  return EVertexElement::Position;
        case EMeshVertexElement::Normal:    return EVertexElement::Normal;
        case EMeshVertexElement::Tangent:   return EVertexElement::Tangent;
        case EMeshVertexElement::TexCoord0: return EVertexElement::TexCoord0;
        default:                            return EVertexElement::Color;
    }
}

EVertexAttributeFlags MeshStreamLayouts::ToVertexAttributes(EMeshVertexAttributes Attributes)
{
    EVertexAttributeFlags Result = EVertexAttributeFlags::None;
    if (IsEnumFlagSet(Attributes, EMeshVertexAttributes::Position))
    {
        Result |= EVertexAttributeFlags::Position;
    }

    if (IsEnumFlagSet(Attributes, EMeshVertexAttributes::TangentBasis))
    {
        Result |= EVertexAttributeFlags::TangentBasis;
    }

    if (IsEnumFlagSet(Attributes, EMeshVertexAttributes::TexCoord0))
    {
        Result |= EVertexAttributeFlags::TexCoord0;
    }

    if (IsEnumFlagSet(Attributes, EMeshVertexAttributes::Color))
    {
        Result |= EVertexAttributeFlags::Color;
    }

    return Result;
}

bool MeshStreamLayouts::MatchesDeclaration(const FMeshStreamLayout& Layout, const FVertexDeclaration& Declaration)
{
    if (Layout.GetNumStreams() != Declaration.GetNumStreams() || Layout.GetElements().Size() != Declaration.GetAttributes().Size())
    {
        return false;
    }

    for (uint8 StreamIndex = 0; StreamIndex < MESH_MAX_STREAMS; ++StreamIndex)
    {
        if (Layout.GetStreamStride(StreamIndex) != Declaration.GetStreamStride(StreamIndex))
        {
            return false;
        }
    }

    for (const FMeshStreamElement& Element : Layout.GetElements())
    {
        bool bFound = false;
        for (const FVertexAttributeInfo& Attribute : Declaration.GetAttributes())
        {
            if (Attribute.Element == ToVertexElement(Element.Element))
            {
                bFound = Attribute.StreamIndex == Element.StreamIndex && Attribute.ByteOffset == Element.ByteOffset
                    && GetByteStrideFromFormat(Attribute.Format) == FMeshStreamLayout::GetEncodingSize(Element.Encoding);
                break;
            }
        }

        if (!bFound)
        {
            return false;
        }
    }

    return true;
}
