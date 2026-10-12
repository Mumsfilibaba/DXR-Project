#include <Core/CoreTypes.h>
#include <Core/CoreDefines.h>
#include <Core/Misc/ConsoleManager.h>

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"
#include "LaunchProgram/ProgramEntry.h"

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
#include "PanelChromeTests.h"
#include "PropertyTableTests.h"
#include "DrawCacheTests.h"
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

static int32 RunApplicationTests()
{
    TestHarness::Initialize(TEXT("TestResults_Application.log"));
    LOG_INFO("=== Application Tests ===");

    RUN_TEST(TEXT("Margin"), Margin_Test());
    RUN_TEST(TEXT("Rectangle"), Rectangle_Test());
    RUN_TEST(TEXT("FixedWidthFontFace"), FixedWidthFontFace_Test());
    RUN_TEST(TEXT("TextBlockDesiredSize"), TextBlockDesiredSize_Test());
    RUN_TEST(TEXT("VerticalBoxLayout"), VerticalBoxLayout_Test());
    RUN_TEST(TEXT("HorizontalBoxLayout"), HorizontalBoxLayout_Test());
    RUN_TEST(TEXT("BoxSlotAlignment"), BoxSlotAlignment_Test());
    RUN_TEST(TEXT("ScrollBoxClamping"), ScrollBoxClamping_Test());
    RUN_TEST(TEXT("ScrollBoxScrollIntoView"), ScrollBoxScrollIntoView_Test());
    RUN_TEST(TEXT("ScrollBoxHitTestClipping"), ScrollBoxHitTestClipping_Test());
    RUN_TEST(TEXT("HitTestPruning"), HitTestPruning_Test());
    RUN_TEST(TEXT("ArrangeSkip"), ArrangeSkip_Test());
    RUN_TEST(TEXT("UIAnimation"), UIAnimation_Test());
    RUN_TEST(TEXT("PopupPlacement"), PopupPlacement_Test());
    RUN_TEST(TEXT("WindowLayoutOrigin"), WindowLayoutOrigin_Test());
    RUN_TEST(TEXT("WindowOverlayMeasure"), WindowOverlayMeasure_Test());

    RUN_TEST(TEXT("DrawCommandList"), DrawCommandList_Test());
    RUN_TEST(TEXT("HairlineCoverage"), HairlineCoverage_Test());
    RUN_TEST(TEXT("DrawClipNesting"), DrawClipNesting_Test());
    RUN_TEST(TEXT("BorderDraw"), BorderDraw_Test());
    RUN_TEST(TEXT("BoxLayerSequencing"), BoxLayerSequencing_Test());
    RUN_TEST(TEXT("LogSeverityColors"), LogSeverityColors_Test());

    RUN_TEST(TEXT("FontAtlasPacking"), FontAtlasPacking_Test());
    RUN_TEST(TEXT("FontGlyphLookup"), FontGlyphLookup_Test());
    RUN_TEST(TEXT("FontAtlasIncremental"), FontAtlasIncremental_Test());
    RUN_TEST(TEXT("FontKerning"), FontKerning_Test());
    RUN_TEST(TEXT("FontMeasurement"), FontMeasurement_Test());
    RUN_TEST(TEXT("FontElision"), FontElision_Test());
    RUN_TEST(TEXT("FontUnicodeShaping"), FontUnicodeShaping_Test());
    RUN_TEST(TEXT("FontFallback"), FontFallback_Test());

    RUN_TEST(TEXT("UIDrawDataLayerOrder"), UIDrawDataLayerOrder_Test());
    RUN_TEST(TEXT("UIDrawDataBatching"), UIDrawDataBatching_Test());
    RUN_TEST(TEXT("UIDrawDataRoundedBox"), UIDrawDataRoundedBox_Test());
    RUN_TEST(TEXT("UIDrawDataRoundedBottomBar"), UIDrawDataRoundedBottomBar_Test());
    RUN_TEST(TEXT("UIDrawDataRoundedAccentRing"), UIDrawDataRoundedAccentRing_Test());
    RUN_TEST(TEXT("UIDrawDataText"), UIDrawDataText_Test());
    RUN_TEST(TEXT("UIDrawDataClipCulling"), UIDrawDataClipCulling_Test());
    RUN_TEST(TEXT("UIDrawDataSiblingClips"), UIDrawDataSiblingClips_Test());
    RUN_TEST(TEXT("UIDrawDataAntiAliasing"), UIDrawDataAntiAliasing_Test());
    RUN_TEST(TEXT("UIDrawDataTextCacheTrim"), UIDrawDataTextCacheTrim_Test());

    RUN_TEST(TEXT("DrawCacheEquivalence"), DrawCacheEquivalence_Test());
    RUN_TEST(TEXT("DrawCacheGuards"), DrawCacheGuards_Test());
    RUN_TEST(TEXT("DrawCacheNesting"), DrawCacheNesting_Test());
    RUN_TEST(TEXT("DrawCacheInvalidation"), DrawCacheInvalidation_Test());
    RUN_TEST(TEXT("DrawCacheAtlasRevision"), DrawCacheAtlasRevision_Test());
    RUN_TEST(TEXT("DrawCacheGeometry"), DrawCacheGeometry_Test());
    RUN_TEST(TEXT("DrawCacheMenuHost"), DrawCacheMenuHost_Test());
    RUN_TEST(TEXT("DrawCacheWindowDeferred"), DrawCacheWindowDeferred_Test());
    RUN_TEST(TEXT("DrawCacheMenuBar"), DrawCacheMenuBar_Test());
    RUN_TEST(TEXT("DrawCacheToolTip"), DrawCacheToolTip_Test());
    RUN_TEST(TEXT("DrawCacheLayerOrder"), DrawCacheLayerOrder_Test());
    RUN_TEST(TEXT("DrawCacheTranslation"), DrawCacheTranslation_Test());
    RUN_TEST(TEXT("DrawCommandPayload"), DrawCommandPayload_Test());

    RUN_TEST(TEXT("CornerRadiiTypes"), CornerRadiiTypes_Test());
    RUN_TEST(TEXT("VectorDrawCommands"), VectorDrawCommands_Test());
    RUN_TEST(TEXT("VectorDrawPolyline"), VectorDrawPolyline_Test());
    RUN_TEST(TEXT("VectorDrawConvexPolygon"), VectorDrawConvexPolygon_Test());
    RUN_TEST(TEXT("VectorDrawCircles"), VectorDrawCircles_Test());
    RUN_TEST(TEXT("VectorDrawBezier"), VectorDrawBezier_Test());
    RUN_TEST(TEXT("VectorDrawPerCornerRounding"), VectorDrawPerCornerRounding_Test());
    RUN_TEST(TEXT("VectorDrawBoxOutline"), VectorDrawBoxOutline_Test());
    RUN_TEST(TEXT("VectorDrawClipCulling"), VectorDrawClipCulling_Test());

    RUN_TEST(TEXT("ImageBrush"), ImageBrush_Test());
    RUN_TEST(TEXT("ImageDrawQuad"), ImageDrawQuad_Test());
    RUN_TEST(TEXT("ImageNineSlice"), ImageNineSlice_Test());
    RUN_TEST(TEXT("ImageBatching"), ImageBatching_Test());

    RUN_TEST(TEXT("StyleDefaults"), StyleDefaults_Test());
    RUN_TEST(TEXT("StyleControlColor"), StyleControlColor_Test());
    RUN_TEST(TEXT("StyleOverride"), StyleOverride_Test());
    RUN_TEST(TEXT("PopupCornerRounding"), PopupCornerRounding_Test());

    RUN_TEST(TEXT("TextLayoutBasics"), TextLayoutBasics_Test());
    RUN_TEST(TEXT("TextLayoutNewlines"), TextLayoutNewlines_Test());
    RUN_TEST(TEXT("TextLayoutWrapping"), TextLayoutWrapping_Test());
    RUN_TEST(TEXT("TextLayoutUnicodeWrapping"), TextLayoutUnicodeWrapping_Test());
    RUN_TEST(TEXT("TextLayoutHitTesting"), TextLayoutHitTesting_Test());
    RUN_TEST(TEXT("TextLayoutDraw"), TextLayoutDraw_Test());

    RUN_TEST(TEXT("InteractionState"), InteractionState_Test());
    RUN_TEST(TEXT("InteractionClick"), InteractionClick_Test());
    RUN_TEST(TEXT("InteractionCapture"), InteractionCapture_Test());
    RUN_TEST(TEXT("InteractionKeyboard"), InteractionKeyboard_Test());

    RUN_TEST(TEXT("ButtonControl"), ButtonControl_Test());
    RUN_TEST(TEXT("GhostButtonControl"), GhostButtonControl_Test());
    RUN_TEST(TEXT("CheckBoxControl"), CheckBoxControl_Test());
    RUN_TEST(TEXT("SliderControl"), SliderControl_Test());
    RUN_TEST(TEXT("SpinBoxControl"), SpinBoxControl_Test());
    RUN_TEST(TEXT("ScrollBarControl"), ScrollBarControl_Test());
    RUN_TEST(TEXT("ScrollBoxScrollBar"), ScrollBoxScrollBar_Test());
    RUN_TEST(TEXT("ScrollBarAutoHide"), ScrollBarAutoHide_Test());
    RUN_TEST(TEXT("OverlayControl"), OverlayControl_Test());
    RUN_TEST(TEXT("SpacerSeparatorControl"), SpacerSeparatorControl_Test());
    RUN_TEST(TEXT("ExpanderControl"), ExpanderControl_Test());
    RUN_TEST(TEXT("ExpanderAnimation"), ExpanderAnimation_Test());
    RUN_TEST(TEXT("IndexedPathMove"), IndexedPathMove_Test());
    RUN_TEST(TEXT("SearchBoxControl"), SearchBoxControl_Test());
    RUN_TEST(TEXT("SearchBoxClearStyle"), SearchBoxClearStyle_Test());
    RUN_TEST(TEXT("NumericEntryControl"), NumericEntryControl_Test());
    RUN_TEST(TEXT("ProgressBarControl"), ProgressBarControl_Test());
    RUN_TEST(TEXT("HistogramControl"), HistogramControl_Test());
    RUN_TEST(TEXT("ProfilerTimelineControl"), ProfilerTimelineControl_Test());

    RUN_TEST(TEXT("TreeViewModel"), TreeViewModel_Test());
    RUN_TEST(TEXT("TreeViewSelection"), TreeViewSelection_Test());
    RUN_TEST(TEXT("TreeViewFiltering"), TreeViewFiltering_Test());
    RUN_TEST(TEXT("TreeViewKeyboard"), TreeViewKeyboard_Test());
    RUN_TEST(TEXT("TreeViewScrolling"), TreeViewScrolling_Test());
    RUN_TEST(TEXT("TreeViewColumnsAndIndent"), TreeViewColumnsAndIndent_Test());
    RUN_TEST(TEXT("TreeViewHeaderToolTips"), TreeViewHeaderToolTips_Test());
    RUN_TEST(TEXT("TreeViewHoverAndRowClick"), TreeViewHoverAndRowClick_Test());
    RUN_TEST(TEXT("TileViewLayout"), TileViewLayout_Test());
    RUN_TEST(TEXT("TileViewSelection"), TileViewSelection_Test());
    RUN_TEST(TEXT("TileViewDrag"), TileViewDrag_Test());

    RUN_TEST(TEXT("PropertyTableRows"), PropertyTableRows_Test());
    RUN_TEST(TEXT("PropertyTableRowContext"), PropertyTableRowContext_Test());
    RUN_TEST(TEXT("PropertyTableColumnDrag"), PropertyTableColumnDrag_Test());
    RUN_TEST(TEXT("PropertyTableRevertCursor"), PropertyTableRevertCursor_Test());
    RUN_TEST(TEXT("PropertyTableFixedColumn"), PropertyTableFixedColumn_Test());
    RUN_TEST(TEXT("PropertyTableToolTips"), PropertyTableToolTips_Test());
    RUN_TEST(TEXT("PropertyTableLayout"), PropertyTableLayout_Test());

    RUN_TEST(TEXT("MenuStackPlacement"), MenuStackPlacement_Test());
    RUN_TEST(TEXT("MenuStackDepth"), MenuStackDepth_Test());
    RUN_TEST(TEXT("MenuItemLayout"), MenuItemLayout_Test());
    RUN_TEST(TEXT("MenuStyle"), MenuStyle_Test());
    RUN_TEST(TEXT("MenuMinimumWidth"), MenuMinimumWidth_Test());
    RUN_TEST(TEXT("MenuItemActivation"), MenuItemActivation_Test());
    RUN_TEST(TEXT("MenuKeyboard"), MenuKeyboard_Test());
    RUN_TEST(TEXT("MenuBarSwitching"), MenuBarSwitching_Test());
    RUN_TEST(TEXT("MenuBlocksHoverBehindIt"), MenuBlocksHoverBehindIt_Test());
    RUN_TEST(TEXT("MenuBarHighlight"), MenuBarHighlight_Test());
    RUN_TEST(TEXT("MenuBarTallStrip"), MenuBarTallStrip_Test());
    RUN_TEST(TEXT("MenuBarInTitleBar"), MenuBarInTitleBar_Test());
    RUN_TEST(TEXT("ToolTipService"), ToolTipService_Test());
    RUN_TEST(TEXT("ComboBoxControl"), ComboBoxControl_Test());
    RUN_TEST(TEXT("DragDropService"), DragDropService_Test());

    RUN_TEST(TEXT("TitleBarMetrics"), TitleBarMetrics_Test());
    RUN_TEST(TEXT("TitleBarRegions"), TitleBarRegions_Test());
    RUN_TEST(TEXT("CaptionButtons"), CaptionButtons_Test());
    RUN_TEST(TEXT("FloatingWindow"), FloatingWindow_Test());

    RUN_TEST(TEXT("DockNodeMinimumSize"), DockNodeMinimumSize_Test());
    RUN_TEST(TEXT("DockNodeCollapse"), DockNodeCollapse_Test());
    RUN_TEST(TEXT("SplitterLayout"), SplitterLayout_Test());
    RUN_TEST(TEXT("SplitterDrag"), SplitterDrag_Test());
    RUN_TEST(TEXT("SplitterSeededDesc"), SplitterSeededDesc_Test());
    RUN_TEST(TEXT("TabStripReorder"), TabStripReorder_Test());
    RUN_TEST(TEXT("TabStripScroll"), TabStripScroll_Test());
    RUN_TEST(TEXT("TabStripStyle"), TabStripStyle_Test());
    RUN_TEST(TEXT("TabMinimumWidth"), TabMinimumWidth_Test());
    RUN_TEST(TEXT("TabStripTearOut"), TabStripTearOut_Test());
    RUN_TEST(TEXT("DockingAreaDockUndock"), DockingAreaDockUndock_Test());
    RUN_TEST(TEXT("DockingAreaHitTest"), DockingAreaHitTest_Test());
    RUN_TEST(TEXT("DockingAreaPersistence"), DockingAreaPersistence_Test());
    RUN_TEST(TEXT("DockingAreaTabReorder"), DockingAreaTabReorder_Test());
    RUN_TEST(TEXT("DockDragState"), DockDragState_Test());
    RUN_TEST(TEXT("DockWindowManagerTearOut"), DockWindowManagerTearOut_Test());
    RUN_TEST(TEXT("DockDecoratorDrag"), DockDecoratorDrag_Test());
    RUN_TEST(TEXT("DockDecoratorSnapshot"), DockDecoratorSnapshot_Test());
    RUN_TEST(TEXT("DockDropPreview"), DockDropPreview_Test());
    RUN_TEST(TEXT("DockHostNativeDrag"), DockHostNativeDrag_Test());
    RUN_TEST(TEXT("DockLayoutFileMultiWindow"), DockLayoutFileMultiWindow_Test());
    RUN_TEST(TEXT("DockHostAdaptiveChrome"), DockHostAdaptiveChrome_Test());
    RUN_TEST(TEXT("DockHostCaptionTabDrop"), DockHostCaptionTabDrop_Test());
    RUN_TEST(TEXT("DockTabDragIntoExistingHost"), DockTabDragIntoExistingHost_Test());

    RUN_TEST(TEXT("PanelChromeGeometry"), PanelChromeGeometry_Test());
    RUN_TEST(TEXT("PanelChromeGap"), PanelChromeGap_Test());
    RUN_TEST(TEXT("SplitterHintThickness"), SplitterHintThickness_Test());
    RUN_TEST(TEXT("TabStripBlendsIntoPanel"), TabStripBlendsIntoPanel_Test());
    RUN_TEST(TEXT("PanelChromeFocusStroke"), PanelChromeFocusStroke_Test());
    RUN_TEST(TEXT("PanelChromeSnapshots"), PanelChromeSnapshots_Test());

    RUN_TEST(TEXT("RichTextLayout"), RichTextLayout_Test());
    RUN_TEST(TEXT("RichTextSelection"), RichTextSelection_Test());
    RUN_TEST(TEXT("RichTextSearch"), RichTextSearch_Test());
    RUN_TEST(TEXT("RichTextCulling"), RichTextCulling_Test());
    RUN_TEST(TEXT("LogViewLogging"), LogViewLogging_Test());
    RUN_TEST(TEXT("LogViewFiltering"), LogViewFiltering_Test());
    RUN_TEST(TEXT("LogViewAutoScroll"), LogViewAutoScroll_Test());

    RUN_TEST(TEXT("ToolBarComposition"), ToolBarComposition_Test());
    RUN_TEST(TEXT("ToolBarInteraction"), ToolBarInteraction_Test());
    RUN_TEST(TEXT("ToolBarGroups"), ToolBarGroups_Test());
    RUN_TEST(TEXT("ToolBarFlexibleSpace"), ToolBarFlexibleSpace_Test());
    RUN_TEST(TEXT("ToolBarDropDown"), ToolBarDropDown_Test());

    RUN_TEST(TEXT("GraphModelEditing"), GraphModelEditing_Test());
    RUN_TEST(TEXT("GraphLayoutLayered"), GraphLayoutLayered_Test());
    RUN_TEST(TEXT("GraphCanvasView"), GraphCanvasView_Test());
    RUN_TEST(TEXT("GraphCanvasInteraction"), GraphCanvasInteraction_Test());
    RUN_TEST(TEXT("GraphNodeStyle"), GraphNodeStyle_Test());
    RUN_TEST(TEXT("GraphStackedPins"), GraphStackedPins_Test());
    RUN_TEST(TEXT("GraphViewerMode"), GraphViewerMode_Test());

    RUN_TEST(TEXT("GizmoProjection"), GizmoProjection_Test());
    RUN_TEST(TEXT("GizmoHitTest"), GizmoHitTest_Test());
    RUN_TEST(TEXT("GizmoDrag"), GizmoDrag_Test());
    RUN_TEST(TEXT("GizmoDeltaContract"), GizmoDeltaContract_Test());
    RUN_TEST(TEXT("GizmoModes"), GizmoModes_Test());

    RUN_TEST(TEXT("ApplicationRendererWindowPass"), ApplicationRendererWindowPass_Test());
    RUN_TEST(TEXT("ApplicationRendererExternalSurface"), ApplicationRendererExternalSurface_Test());
    RUN_TEST(TEXT("ApplicationRendererWindowLifetime"), ApplicationRendererWindowLifetime_Test());
    RUN_TEST(TEXT("ApplicationRendererPopupSurface"), ApplicationRendererPopupSurface_Test());

    RUN_TEST(TEXT("EditableTextEditing"), EditableTextEditing_Test());
    RUN_TEST(TEXT("EditableTextCursor"), EditableTextCursor_Test());
    RUN_TEST(TEXT("EditableTextUnicode"), EditableTextUnicode_Test());
    RUN_TEST(TEXT("EditableTextKeyInterceptor"), EditableTextKeyInterceptor_Test());
    RUN_TEST(TEXT("EditableTextSelection"), EditableTextSelection_Test());
    RUN_TEST(TEXT("EditableTextMouseSelection"), EditableTextMouseSelection_Test());
    RUN_TEST(TEXT("EditableTextWordNavigation"), EditableTextWordNavigation_Test());
    RUN_TEST(TEXT("EditableTextCommandChord"), EditableTextCommandChord_Test());
    RUN_TEST(TEXT("EditableTextCaretBlink"), EditableTextCaretBlink_Test());
    RUN_TEST(TEXT("EditableTextBandAlignment"), EditableTextBandAlignment_Test());
    RUN_TEST(TEXT("EditableTextDraw"), EditableTextDraw_Test());
    RUN_TEST(TEXT("EditableTextAlignment"), EditableTextAlignment_Test());

    RUN_TEST(TEXT("ConsoleWordRange"), ConsoleWordRange_Test());
    RUN_TEST(TEXT("ConsoleCandidates"), ConsoleCandidates_Test());
    RUN_TEST(TEXT("ConsoleCandidateSelection"), ConsoleCandidateSelection_Test());
    RUN_TEST(TEXT("ConsoleCandidateDirectSelection"), ConsoleCandidateDirectSelection_Test());
    RUN_TEST(TEXT("ConsoleCompletion"), ConsoleCompletion_Test());
    RUN_TEST(TEXT("ConsoleHistory"), ConsoleHistory_Test());
    RUN_TEST(TEXT("ConsoleSubmit"), ConsoleSubmit_Test());
    RUN_TEST(TEXT("ConsoleLogBufferRing"), ConsoleLogBufferRing_Test());

    RUN_TEST(TEXT("ConsoleToggle"), ConsoleToggle_Test());
    RUN_TEST(TEXT("ConsoleLayout"), ConsoleLayout_Test());
    RUN_TEST(TEXT("ConsoleLogDraw"), ConsoleLogDraw_Test());
    RUN_TEST(TEXT("ConsoleCandidateDraw"), ConsoleCandidateDraw_Test());
    RUN_TEST(TEXT("ConsoleCandidateHighlight"), ConsoleCandidateHighlight_Test());
    RUN_TEST(TEXT("ConsoleCandidateColumns"), ConsoleCandidateColumns_Test());
    RUN_TEST(TEXT("ConsoleInputChrome"), ConsoleInputChrome_Test());
    RUN_TEST(TEXT("ConsoleCursorShape"), ConsoleCursorShape_Test());
    RUN_TEST(TEXT("ConsoleInputFieldSurvives"), ConsoleInputFieldSurvives_Test());
    RUN_TEST(TEXT("ConsoleModalInput"), ConsoleModalInput_Test());
    RUN_TEST(TEXT("ConsoleClickFocus"), ConsoleClickFocus_Test());
    RUN_TEST(TEXT("ConsoleInWindow"), ConsoleInWindow_Test());
    RUN_TEST(TEXT("ConsoleTypeAndExecute"), ConsoleTypeAndExecute_Test());

    const int32 ExitCode = TestHarness::Report();
    TestHarness::Shutdown();
    return ExitCode;
}

IMPLEMENT_PROGRAM_MAIN(TEXT("Application-Tests"), RunApplicationTests);
