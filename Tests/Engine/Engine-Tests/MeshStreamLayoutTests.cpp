#include "MeshStreamLayoutTests.h"

#include <Engine/Assets/MeshStreamLayouts.h>

#include "TestCommon/TestMacros.h"

bool MeshStreamLayouts_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The renderer's vertex declarations read every attribute set exactly as mesh assets store it"));
    {
        for (uint16 Mask = 0; Mask < NUM_MESH_VERTEX_ATTRIBUTE_SETS; ++Mask)
        {
            const EMeshVertexAttributes  Attributes  = static_cast<EMeshVertexAttributes>(Mask);
            const FMeshStreamLayout&     Layout      = FMeshStreamLayout::GetOrCreate(Attributes);
            const FVertexDeclaration&    Declaration = FVertexDeclaration::GetOrCreate(MeshStreamLayouts::ToVertexAttributes(Attributes));

            TEST_EXPECT(MeshStreamLayouts::MatchesDeclaration(Layout, Declaration));
        }
    }

    TEST_SECTION(TEXT("A layout that differs from the declaration is caught"));
    {
        const FMeshStreamLayout&  Standard    = FMeshStreamLayout::GetStandardStaticMesh();
        const FVertexDeclaration& WithColor   = FVertexDeclaration::GetOrCreate(VERTEX_ATTRIBUTES_ALL);
        const FVertexDeclaration& NoTexCoords = FVertexDeclaration::GetOrCreate(EVertexAttributeFlags::Position | EVertexAttributeFlags::TangentBasis);

        TEST_EXPECT(!MeshStreamLayouts::MatchesDeclaration(Standard, WithColor));
        TEST_EXPECT(!MeshStreamLayouts::MatchesDeclaration(Standard, NoTexCoords));
    }

    TEST_END();
}
