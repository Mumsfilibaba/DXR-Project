#include "PrimitiveTopologyTests.h"

#include <Core/Templates/CString.h>
#include <Core/Templates/Utility/UnderlyingTypeValue.h>
#include <RHI/RHICore.h>
#include <RHI/RHITypes.h>

#include "TestCommon/TestMacros.h"

bool PrimitiveTopology_Test()
{
    TEST_BEGIN();

    TEST_SECTION("ToString covers every enumerator");
    TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology::Undefined), "Undefined") == 0);
    TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology::TriangleList), "TriangleList") == 0);
    TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology::TriangleListAdjacency), "TriangleListAdjacency") == 0);
    TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology::PatchList_1), "PatchList_1") == 0);
    TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology::PatchList_32), "PatchList_32") == 0);
    for (uint8 Value = 0; Value <= UnderlyingTypeValue(EPrimitiveTopology::PatchList_32); ++Value)
    {
        TEST_EXPECT(TCString<CHAR>::Strcmp(ToString(EPrimitiveTopology(Value)), "Unknown") != 0);
    }

    TEST_SECTION("Topology classification");
    TEST_EXPECT(IsPatchTopology(EPrimitiveTopology::PatchList_1));
    TEST_EXPECT(IsPatchTopology(EPrimitiveTopology::PatchList_32));
    TEST_EXPECT(!IsPatchTopology(EPrimitiveTopology::Undefined));
    TEST_EXPECT(!IsPatchTopology(EPrimitiveTopology::TriangleList));
    TEST_EXPECT(!IsPatchTopology(EPrimitiveTopology::TriangleStripAdjacency));
    TEST_EXPECT(IsAdjacencyTopology(EPrimitiveTopology::LineListAdjacency));
    TEST_EXPECT(IsAdjacencyTopology(EPrimitiveTopology::LineStripAdjacency));
    TEST_EXPECT(IsAdjacencyTopology(EPrimitiveTopology::TriangleListAdjacency));
    TEST_EXPECT(IsAdjacencyTopology(EPrimitiveTopology::TriangleStripAdjacency));
    TEST_EXPECT(!IsAdjacencyTopology(EPrimitiveTopology::TriangleList));
    TEST_EXPECT(!IsAdjacencyTopology(EPrimitiveTopology::PatchList_16));
    TEST_EXPECT(IsStripTopology(EPrimitiveTopology::LineStrip));
    TEST_EXPECT(IsStripTopology(EPrimitiveTopology::TriangleStrip));
    TEST_EXPECT(IsStripTopology(EPrimitiveTopology::LineStripAdjacency));
    TEST_EXPECT(IsStripTopology(EPrimitiveTopology::TriangleStripAdjacency));
    TEST_EXPECT(!IsStripTopology(EPrimitiveTopology::LineList));
    TEST_EXPECT(!IsStripTopology(EPrimitiveTopology::TriangleList));
    TEST_EXPECT(!IsStripTopology(EPrimitiveTopology::PatchList_4));

    TEST_SECTION("Patch control-point round trip");
    TEST_EXPECT_EQ(GetNumPatchControlPoints(EPrimitiveTopology::TriangleList), 0u);
    TEST_EXPECT_EQ(GetNumPatchControlPoints(EPrimitiveTopology::TriangleStripAdjacency), 0u);
    TEST_EXPECT_EQ(GetNumPatchControlPoints(EPrimitiveTopology::PatchList_1), 1u);
    TEST_EXPECT_EQ(GetNumPatchControlPoints(EPrimitiveTopology::PatchList_32), uint32(RHI_MAX_PATCH_CONTROL_POINTS));
    for (uint32 NumControlPoints = 1; NumControlPoints <= RHI_MAX_PATCH_CONTROL_POINTS; ++NumControlPoints)
    {
        const EPrimitiveTopology Topology = CreatePatchListTopology(NumControlPoints);
        TEST_EXPECT(IsPatchTopology(Topology));
        TEST_EXPECT_EQ(GetNumPatchControlPoints(Topology), NumControlPoints);
    }

    TEST_EXPECT_EQ(CreatePatchListTopology(1), EPrimitiveTopology::PatchList_1);
    TEST_EXPECT_EQ(CreatePatchListTopology(RHI_MAX_PATCH_CONTROL_POINTS), EPrimitiveTopology::PatchList_32);
    TEST_EXPECT_EQ(CreatePatchListTopology(0), EPrimitiveTopology::Undefined);
    TEST_EXPECT_EQ(CreatePatchListTopology(RHI_MAX_PATCH_CONTROL_POINTS + 1), EPrimitiveTopology::Undefined);

    TEST_END();
}
