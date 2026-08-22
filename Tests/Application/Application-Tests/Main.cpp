#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/Misc/ConsoleManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "ApplicationRendererTests.h"
#include "ConsoleCommandLineTests.h"
#include "ConsoleTests.h"
#include "EditableTextTests.h"
#include "FontTests.h"
#include "LayoutTests.h"
#include "DrawTests.h"
#include "UIDrawDataTests.h"

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
    RUN_TEST("WindowLayoutOrigin", WindowLayoutOrigin_Test());
    RUN_TEST("WindowOverlayMeasure", WindowOverlayMeasure_Test());

    RUN_TEST("DrawCommandList", DrawCommandList_Test());
    RUN_TEST("DrawClipNesting", DrawClipNesting_Test());
    RUN_TEST("BorderDraw", BorderDraw_Test());
    RUN_TEST("BoxLayerSequencing", BoxLayerSequencing_Test());
    RUN_TEST("LogSeverityColors", LogSeverityColors_Test());

    RUN_TEST("FontAtlasPacking", FontAtlasPacking_Test());
    RUN_TEST("FontGlyphLookup", FontGlyphLookup_Test());
    RUN_TEST("FontMeasurement", FontMeasurement_Test());

    RUN_TEST("UIDrawDataLayerOrder", UIDrawDataLayerOrder_Test());
    RUN_TEST("UIDrawDataBatching", UIDrawDataBatching_Test());
    RUN_TEST("UIDrawDataRoundedBox", UIDrawDataRoundedBox_Test());
    RUN_TEST("UIDrawDataText", UIDrawDataText_Test());
    RUN_TEST("UIDrawDataClipCulling", UIDrawDataClipCulling_Test());

    RUN_TEST("ApplicationRendererWindowPass", ApplicationRendererWindowPass_Test());
    RUN_TEST("ApplicationRendererWindowLifetime", ApplicationRendererWindowLifetime_Test());

    RUN_TEST("EditableTextEditing", EditableTextEditing_Test());
    RUN_TEST("EditableTextCursor", EditableTextCursor_Test());
    RUN_TEST("EditableTextKeyInterceptor", EditableTextKeyInterceptor_Test());
    RUN_TEST("EditableTextSelection", EditableTextSelection_Test());
    RUN_TEST("EditableTextMouseSelection", EditableTextMouseSelection_Test());
    RUN_TEST("EditableTextWordNavigation", EditableTextWordNavigation_Test());
    RUN_TEST("EditableTextCommandChord", EditableTextCommandChord_Test());
    RUN_TEST("EditableTextCaretBlink", EditableTextCaretBlink_Test());
    RUN_TEST("EditableTextBandAlignment", EditableTextBandAlignment_Test());
    RUN_TEST("EditableTextDraw", EditableTextDraw_Test());

    RUN_TEST("ConsoleWordRange", ConsoleWordRange_Test());
    RUN_TEST("ConsoleCandidates", ConsoleCandidates_Test());
    RUN_TEST("ConsoleCandidateSelection", ConsoleCandidateSelection_Test());
    RUN_TEST("ConsoleCompletion", ConsoleCompletion_Test());
    RUN_TEST("ConsoleHistory", ConsoleHistory_Test());
    RUN_TEST("ConsoleSubmit", ConsoleSubmit_Test());
    RUN_TEST("ConsoleLogBufferRing", ConsoleLogBufferRing_Test());

    RUN_TEST("ConsoleToggle", ConsoleToggle_Test());
    RUN_TEST("ConsoleLayout", ConsoleLayout_Test());
    RUN_TEST("ConsoleLogDraw", ConsoleLogDraw_Test());
    RUN_TEST("ConsoleCandidateDraw", ConsoleCandidateDraw_Test());
    RUN_TEST("ConsoleCandidateHighlight", ConsoleCandidateHighlight_Test());
    RUN_TEST("ConsoleCandidateColumns", ConsoleCandidateColumns_Test());
    RUN_TEST("ConsoleInputChrome", ConsoleInputChrome_Test());
    RUN_TEST("ConsoleCursorShape", ConsoleCursorShape_Test());
    RUN_TEST("ConsoleInputFieldSurvives", ConsoleInputFieldSurvives_Test());
    RUN_TEST("ConsoleModalInput", ConsoleModalInput_Test());
    RUN_TEST("ConsoleClickFocus", ConsoleClickFocus_Test());
    RUN_TEST("ConsoleInWindow", ConsoleInWindow_Test());
    RUN_TEST("ConsoleTypeAndExecute", ConsoleTypeAndExecute_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}
