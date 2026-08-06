#include "VertexDeclarationTests.h"

#include <RendererCore/VertexDeclaration.h>

#include "TestCommon/TestMacros.h"

namespace
{
    const FVertexAttributeInfo* FindElement(const FVertexDeclaration& Declaration, EVertexElement Element)
    {
        for (const FVertexAttributeInfo& Attribute : Declaration.GetAttributes())
        {
            if (Attribute.Element == Element)
            {
                return &Attribute;
            }
        }

        return nullptr;
    }
}

bool VertexDeclaration_Test()
{
    TEST_BEGIN();

    const FVertexDeclaration& Standard = FVertexDeclaration::GetStandardStaticMesh();

    TEST_SECTION("FVertexDeclaration::GetOrCreate (interning)");
    TEST_EXPECT(&FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position) == &FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position));
    TEST_EXPECT(&FVertexDeclaration::GetByID(Standard.GetID()) == &Standard);
    TEST_EXPECT(FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::None).GetNumStreams() == 0);

    TEST_SECTION("FVertexDeclaration::GetID (distinct per attribute set)");
    TEST_EXPECT(FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position).GetID() != Standard.GetID());
    TEST_EXPECT(FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position | EVertexAttributeFlags::TexCoord0).GetID() != Standard.GetID());

    TEST_SECTION("FVertexDeclaration::HasAttributes");
    TEST_EXPECT(Standard.HasAttributes(EVertexAttributeFlags::Position));
    TEST_EXPECT(Standard.HasAttributes(EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis | EVertexAttributeFlags::TexCoord0));
    TEST_EXPECT(!Standard.HasAttributes(EVertexAttributeFlags::Color));

    TEST_SECTION("FVertexDeclaration::GetStreamStride");
    TEST_EXPECT(Standard.GetStreamStride(EVertexStreamIndex::Position) == 12);
    TEST_EXPECT(Standard.GetStreamStride(EVertexStreamIndex::Attributes) == 24);
    TEST_EXPECT(Standard.GetStreamStride(EVertexStreamIndex::Color) == 0);
    TEST_EXPECT(Standard.GetStreamStride(VERTEX_MAX_STREAMS) == 0);

    TEST_SECTION("FVertexDeclaration::GetAttributes (offsets follow the present elements)");
    {
        const FVertexDeclaration& NoTangents = FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position | EVertexAttributeFlags::TexCoord0);

        TEST_EXPECT(FindElement(Standard, EVertexElement::Normal)->ByteOffset == 0);
        TEST_EXPECT(FindElement(Standard, EVertexElement::Tangent)->ByteOffset == 8);
        TEST_EXPECT(FindElement(Standard, EVertexElement::TexCoord0)->ByteOffset == 16);

        TEST_EXPECT(FindElement(NoTangents, EVertexElement::Normal) == nullptr);
        TEST_EXPECT(FindElement(NoTangents, EVertexElement::TexCoord0)->ByteOffset == 0);
        TEST_EXPECT(NoTangents.GetStreamStride(EVertexStreamIndex::Attributes) == 8);
    }

    TEST_SECTION("FVertexDeclaration::GetAttributes (stream assignment)");
    TEST_EXPECT(FindElement(Standard, EVertexElement::Position)->StreamIndex == EVertexStreamIndex::Position);
    TEST_EXPECT(FindElement(Standard, EVertexElement::Normal)->StreamIndex == EVertexStreamIndex::Attributes);
    TEST_EXPECT(FindElement(Standard, EVertexElement::Tangent)->StreamIndex == EVertexStreamIndex::Attributes);
    TEST_EXPECT(FindElement(Standard, EVertexElement::TexCoord0)->StreamIndex == EVertexStreamIndex::Attributes);

    TEST_SECTION("FVertexDeclaration::operator== / operator!=");
    TEST_EXPECT(Standard == FVertexDeclaration::GetStandardStaticMesh());
    TEST_EXPECT(Standard != FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position));

    TEST_END();
}
