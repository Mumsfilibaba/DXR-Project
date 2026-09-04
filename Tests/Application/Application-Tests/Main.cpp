#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/Misc/ConsoleManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include "ApplicationRendererTests.h"
#include "ConsoleCommandLineTests.h"
#include "ConsoleTests.h"
#include "ControlTests.h"
#include "DockingTests.h"
#include "EditableTextTests.h"
#include "FontTests.h"
#include "GizmoTests.h"
#include "GraphTests.h"
#include "ImageDrawTests.h"
#include "InteractionTests.h"
#include "ItemViewTests.h"
#include "LayoutTests.h"
#include "MenuTests.h"
#include "OutputLogTests.h"
#include "PropertyTableTests.h"
#include "DrawTests.h"
#include "StyleTests.h"
#include "TextLayoutTests.h"
#include "ToolBarTests.h"
#include "UIDrawDataTests.h"
#include "VectorDrawTests.h"
#include "WindowTests.h"

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
    RUN_TEST("ScrollBoxHitTestClipping", ScrollBoxHitTestClipping_Test());
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
    RUN_TEST("UIDrawDataSiblingClips", UIDrawDataSiblingClips_Test());

    RUN_TEST("CornerRadiiTypes", CornerRadiiTypes_Test());
    RUN_TEST("VectorDrawCommands", VectorDrawCommands_Test());
    RUN_TEST("VectorDrawPolyline", VectorDrawPolyline_Test());
    RUN_TEST("VectorDrawConvexPolygon", VectorDrawConvexPolygon_Test());
    RUN_TEST("VectorDrawCircles", VectorDrawCircles_Test());
    RUN_TEST("VectorDrawBezier", VectorDrawBezier_Test());
    RUN_TEST("VectorDrawPerCornerRounding", VectorDrawPerCornerRounding_Test());
    RUN_TEST("VectorDrawBoxOutline", VectorDrawBoxOutline_Test());
    RUN_TEST("VectorDrawClipCulling", VectorDrawClipCulling_Test());

    RUN_TEST("ImageBrush", ImageBrush_Test());
    RUN_TEST("ImageDrawQuad", ImageDrawQuad_Test());
    RUN_TEST("ImageNineSlice", ImageNineSlice_Test());
    RUN_TEST("ImageBatching", ImageBatching_Test());

    RUN_TEST("StyleDefaults", StyleDefaults_Test());
    RUN_TEST("StyleControlColor", StyleControlColor_Test());
    RUN_TEST("StyleOverride", StyleOverride_Test());

    RUN_TEST("TextLayoutBasics", TextLayoutBasics_Test());
    RUN_TEST("TextLayoutNewlines", TextLayoutNewlines_Test());
    RUN_TEST("TextLayoutWrapping", TextLayoutWrapping_Test());
    RUN_TEST("TextLayoutHitTesting", TextLayoutHitTesting_Test());
    RUN_TEST("TextLayoutDraw", TextLayoutDraw_Test());

    RUN_TEST("InteractionState", InteractionState_Test());
    RUN_TEST("InteractionClick", InteractionClick_Test());
    RUN_TEST("InteractionCapture", InteractionCapture_Test());
    RUN_TEST("InteractionKeyboard", InteractionKeyboard_Test());

    RUN_TEST("ButtonControl", ButtonControl_Test());
    RUN_TEST("CheckBoxControl", CheckBoxControl_Test());
    RUN_TEST("SliderControl", SliderControl_Test());
    RUN_TEST("SpinBoxControl", SpinBoxControl_Test());
    RUN_TEST("ScrollBarControl", ScrollBarControl_Test());
    RUN_TEST("ScrollBoxScrollBar", ScrollBoxScrollBar_Test());
    RUN_TEST("OverlayControl", OverlayControl_Test());
    RUN_TEST("SpacerSeparatorControl", SpacerSeparatorControl_Test());
    RUN_TEST("ExpanderControl", ExpanderControl_Test());
    RUN_TEST("SearchBoxControl", SearchBoxControl_Test());
    RUN_TEST("NumericEntryControl", NumericEntryControl_Test());
    RUN_TEST("ProgressBarControl", ProgressBarControl_Test());
    RUN_TEST("HistogramControl", HistogramControl_Test());

    RUN_TEST("TreeViewModel", TreeViewModel_Test());
    RUN_TEST("TreeViewSelection", TreeViewSelection_Test());
    RUN_TEST("TreeViewFiltering", TreeViewFiltering_Test());
    RUN_TEST("TreeViewKeyboard", TreeViewKeyboard_Test());
    RUN_TEST("TreeViewScrolling", TreeViewScrolling_Test());
    RUN_TEST("TileViewLayout", TileViewLayout_Test());
    RUN_TEST("TileViewSelection", TileViewSelection_Test());
    RUN_TEST("TileViewDrag", TileViewDrag_Test());

    RUN_TEST("PropertyTableRows", PropertyTableRows_Test());
    RUN_TEST("PropertyTableColumnDrag", PropertyTableColumnDrag_Test());
    RUN_TEST("PropertyTableToolTips", PropertyTableToolTips_Test());
    RUN_TEST("PropertyTableLayout", PropertyTableLayout_Test());

    RUN_TEST("MenuStackPlacement", MenuStackPlacement_Test());
    RUN_TEST("MenuStackDepth", MenuStackDepth_Test());
    RUN_TEST("MenuItemLayout", MenuItemLayout_Test());
    RUN_TEST("MenuItemActivation", MenuItemActivation_Test());
    RUN_TEST("MenuKeyboard", MenuKeyboard_Test());
    RUN_TEST("MenuBarSwitching", MenuBarSwitching_Test());
    RUN_TEST("MenuBarInTitleBar", MenuBarInTitleBar_Test());
    RUN_TEST("ToolTipService", ToolTipService_Test());
    RUN_TEST("ComboBoxControl", ComboBoxControl_Test());
    RUN_TEST("DragDropService", DragDropService_Test());

    RUN_TEST("TitleBarMetrics", TitleBarMetrics_Test());
    RUN_TEST("TitleBarRegions", TitleBarRegions_Test());
    RUN_TEST("CaptionButtons", CaptionButtons_Test());
    RUN_TEST("FloatingWindow", FloatingWindow_Test());

    RUN_TEST("DockNodeMinimumSize", DockNodeMinimumSize_Test());
    RUN_TEST("DockNodeCollapse", DockNodeCollapse_Test());
    RUN_TEST("SplitterLayout", SplitterLayout_Test());
    RUN_TEST("SplitterDrag", SplitterDrag_Test());
    RUN_TEST("SplitterSeededDesc", SplitterSeededDesc_Test());
    RUN_TEST("TabStripReorder", TabStripReorder_Test());
    RUN_TEST("TabStripTearOut", TabStripTearOut_Test());
    RUN_TEST("DockingAreaDockUndock", DockingAreaDockUndock_Test());
    RUN_TEST("DockingAreaHitTest", DockingAreaHitTest_Test());
    RUN_TEST("DockingAreaPersistence", DockingAreaPersistence_Test());
    RUN_TEST("DockDragState", DockDragState_Test());

    RUN_TEST("RichTextLayout", RichTextLayout_Test());
    RUN_TEST("RichTextSelection", RichTextSelection_Test());
    RUN_TEST("RichTextSearch", RichTextSearch_Test());
    RUN_TEST("RichTextCulling", RichTextCulling_Test());
    RUN_TEST("LogViewLogging", LogViewLogging_Test());
    RUN_TEST("LogViewFiltering", LogViewFiltering_Test());
    RUN_TEST("LogViewAutoScroll", LogViewAutoScroll_Test());

    RUN_TEST("ToolBarComposition", ToolBarComposition_Test());
    RUN_TEST("ToolBarInteraction", ToolBarInteraction_Test());
    RUN_TEST("ToolBarGroups", ToolBarGroups_Test());
    RUN_TEST("ToolBarFlexibleSpace", ToolBarFlexibleSpace_Test());
    RUN_TEST("ToolBarDropDown", ToolBarDropDown_Test());

    RUN_TEST("GraphModelEditing", GraphModelEditing_Test());
    RUN_TEST("GraphLayoutLayered", GraphLayoutLayered_Test());
    RUN_TEST("GraphCanvasView", GraphCanvasView_Test());
    RUN_TEST("GraphCanvasInteraction", GraphCanvasInteraction_Test());
    RUN_TEST("GraphNodeStyle", GraphNodeStyle_Test());
    RUN_TEST("GraphStackedPins", GraphStackedPins_Test());
    RUN_TEST("GraphViewerMode", GraphViewerMode_Test());

    RUN_TEST("GizmoProjection", GizmoProjection_Test());
    RUN_TEST("GizmoHitTest", GizmoHitTest_Test());
    RUN_TEST("GizmoDrag", GizmoDrag_Test());
    RUN_TEST("GizmoModes", GizmoModes_Test());

    RUN_TEST("ApplicationRendererWindowPass", ApplicationRendererWindowPass_Test());
    RUN_TEST("ApplicationRendererExternalSurface", ApplicationRendererExternalSurface_Test());
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
    RUN_TEST("ConsoleCandidateDirectSelection", ConsoleCandidateDirectSelection_Test());
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
