#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/Misc/ConsoleManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "ConsoleCommandLineTests.h"
#include "ConsoleElementTests.h"
#include "EditableTextTests.h"
#include "LayoutTests.h"
#include "DrawTests.h"

#define ENABLE_CUSTOM_MEMORY (1)

#if ENABLE_CUSTOM_MEMORY
#include <Core/Memory/NewOperators.h>
IMPLEMENT_NEW_AND_DELETE_OPERATORS();
#endif

int main(int Argc, const CHAR* Argv[])
{
    UNREFERENCED_VARIABLE(Argc);
    UNREFERENCED_VARIABLE(Argv);

    TestHarness::Initialize("TestResults_Application.log");
    LOG_INFO("=== Application Tests ===");

    RUN_TEST("Margin", Margin_Test());
    RUN_TEST("Rectangle", Rectangle_Test());
    RUN_TEST("FixedWidthFontFace", FixedWidthFontFace_Test());
    RUN_TEST("TextBlockDesiredSize", TextBlockDesiredSize_Test());
    RUN_TEST("VerticalBoxLayout", VerticalBoxLayout_Test());
    RUN_TEST("HorizontalBoxLayout", HorizontalBoxLayout_Test());
    RUN_TEST("BoxSlotAlignment", BoxSlotAlignment_Test());
    RUN_TEST("ScrollBoxClamping", ScrollBoxClamping_Test());
    RUN_TEST("ScrollBoxScrollIntoView", ScrollBoxScrollIntoView_Test());

    RUN_TEST("DrawCommandList", DrawCommandList_Test());
    RUN_TEST("DrawClipNesting", DrawClipNesting_Test());
    RUN_TEST("BorderDraw", BorderDraw_Test());
    RUN_TEST("LogSeverityColors", LogSeverityColors_Test());

    RUN_TEST("EditableTextEditing", EditableTextEditing_Test());
    RUN_TEST("EditableTextCursor", EditableTextCursor_Test());
    RUN_TEST("EditableTextKeyInterceptor", EditableTextKeyInterceptor_Test());
    RUN_TEST("EditableTextDraw", EditableTextDraw_Test());

    RUN_TEST("ConsoleWordRange", ConsoleWordRange_Test());
    RUN_TEST("ConsoleCandidates", ConsoleCandidates_Test());
    RUN_TEST("ConsoleCandidateSelection", ConsoleCandidateSelection_Test());
    RUN_TEST("ConsoleCompletion", ConsoleCompletion_Test());
    RUN_TEST("ConsoleHistory", ConsoleHistory_Test());
    RUN_TEST("ConsoleSubmit", ConsoleSubmit_Test());
    RUN_TEST("ConsoleLogBufferRing", ConsoleLogBufferRing_Test());

    RUN_TEST("ConsoleElementToggle", ConsoleElementToggle_Test());
    RUN_TEST("ConsoleElementLayout", ConsoleElementLayout_Test());
    RUN_TEST("ConsoleElementLogDraw", ConsoleElementLogDraw_Test());
    RUN_TEST("ConsoleElementCandidateDraw", ConsoleElementCandidateDraw_Test());
    RUN_TEST("ConsoleElementTypeAndExecute", ConsoleElementTypeAndExecute_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
