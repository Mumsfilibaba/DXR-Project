#include "DrawCacheTests.h"

#include "StubPlatformApplication.h"
#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Core/Containers/SharedPtr.h>
#include <Core/Misc/ConsoleManager.h>
#include <Core/Misc/Paths.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/DrawCache.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Button.h>
#include <Application/Elements/CheckBox.h>
#include <Application/Elements/MenuHost.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/Window.h>
#include <Application/Input/Keys.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuAnchor.h>
#include <Application/Menus/MenuBar.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Menus/MenuStack.h>
#include <Application/Menus/ToolTipService.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Text/FontAtlas.h>
#include <Application/Text/TrueTypeFontFace.h>

class FScopedMenuServices
{
public:
    FScopedMenuServices() = default;

    ~FScopedMenuServices()
    {
        FMenuStack::Shutdown();
        FToolTipService::Shutdown();
    }

    FScopedMenuServices(const FScopedMenuServices&) = delete;
    FScopedMenuServices& operator=(const FScopedMenuServices&) = delete;
};

class FLayerStepDownElement final : public FVisualElement
{
public:
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        const FRectangle& Bounds = AllottedGeometry.Bounds;

        // Recorded top first, so the block's own layers go down before they go up again
        OutCommandList.AddBox(LayerId + 2, FRectangle(Bounds.Position, 10, 10), FFloatColor(1.0f, 0.0f, 0.0f, 1.0f));
        OutCommandList.AddBox(LayerId, Bounds, FFloatColor(0.0f, 0.0f, 1.0f, 1.0f), FCornerRadii(4.0f));
        OutCommandList.AddBox(LayerId + 1, FRectangle(Bounds.Position + IntVector2(5, 5), 20, 20), FFloatColor(0.0f, 1.0f, 0.0f, 1.0f));
        return LayerId + 2;
    }
};

class FChildPlacementHost final : public FVisualElement
{
public:
    FChildPlacementHost()
        : FVisualElement()
        , Child(nullptr)
        , bPinChild(false)
    {
    }

    virtual void OnArrange(const FRectangle& AllottedBounds) override
    {
        if (Child)
        {
            const IntVector2 ChildPosition = bPinChild ? IntVector2(0, 0) : AllottedBounds.Position;
            Child->Arrange(FRectangle(ChildPosition, 20, 20));
        }
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override
    {
        return Child ? Child->Draw(FDrawGeometry(Child->GetContentRectangle(), AllottedGeometry.Scale), OutCommandList, LayerId + 1) : LayerId;
    }

    void SetChild(const TSharedPtr<FVisualElement>& InChild)
    {
        Child = InChild;
        Child->SetParentElement(AsWeakPtr());
    }

    TSharedPtr<FVisualElement> Child;
    bool                       bPinChild;

protected:
    virtual EChildVisit VisitChildren(FChildVisitor& Visitor, EChildOrder /*Order*/) const override
    {
        return VisitChild(Visitor, Child);
    }
};

static TSharedPtr<IFontFace> CreateFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static FCursorEvent MakeMoveEvent(const IntVector2& ClientPosition)
{
    return FCursorEvent(EInputEventType::MouseMoved, ClientPosition, IntVector2(0, 0), FModifierKeyState());
}

static FCursorEvent MakeButtonEvent(EInputEventType Type, const IntVector2& ClientPosition)
{
    return FCursorEvent(Type, Keys::MouseButtonLeft, ClientPosition, IntVector2(0, 0), FModifierKeyState(),
        Type == EInputEventType::MouseButtonDown);
}

static void LayoutElement(const TSharedPtr<FVisualElement>& Element, const FRectangle& Bounds)
{
    Element->PrepareDesiredSize();
    Element->Arrange(Bounds);
}

static void Record(const TSharedPtr<FVisualElement>& Element, FDrawCommandList& OutCommandList, int32 LayerId = 0)
{
    Element->Draw(FDrawGeometry(Element->GetContentRectangle(), 1.0f), OutCommandList, LayerId);
}

static bool RecordingsAgree(const FDrawCommandList& Left, const FDrawCommandList& Right)
{
    int32  Index = 0;
    String Reason;

    return !FDrawCommandList::FindFirstDifference(Left, Right, Index, Reason);
}

static bool RecordingsMatch(const FDrawCommandList& Left, const FDrawCommandList& Right)
{
    int32  Index = 0;
    String Reason;

    if (!FDrawCommandList::FindFirstDifference(Left, Right, Index, Reason))
    {
        return true;
    }

    LOG_ERROR("[DrawCache] Command %d differs in %s", Index, Reason.Data());
    return false;
}

static void SettleDrawCache(const TSharedPtr<FVisualElement>& Element, int32 FrameCount = 4)
{
    for (int32 Frame = 0; Frame < FrameCount; ++Frame)
    {
        FDrawCommandList Scratch;
        Record(Element, Scratch);
    }
}

static bool ReplayMatchesFreshWalk(const TSharedPtr<FVisualElement>& Element)
{
    FDrawCommandList Replayed;
    Record(Element, Replayed);

    FDrawCommandList Fresh;
    Fresh.SetDrawCacheSuppressed(true);
    Record(Element, Fresh);

    return RecordingsMatch(Replayed, Fresh);
}

static bool RecordingReplays(const TSharedPtr<FVisualElement>& Element)
{
    FDrawCommandList CommandList;
    Record(Element, CommandList);

    return CommandList.WasFullyReplayed();
}

static TSharedPtr<FMenu> CreateMenu(const TSharedPtr<IFontFace>& Font, const TArray<String>& Labels)
{
    TSharedPtr<FMenu> Menu = FMenu::Create();
    for (const String& Label : Labels)
    {
        FMenuItem::FDesc Desc;
        Desc.SetLabel(Label).SetFont(Font);

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    return Menu;
}

static void RecordWindow(const TSharedPtr<FWindow>& Window, FDrawCommandList& OutCommandList)
{
    const int32 TopLayerId = Window->Draw(FDrawGeometry(Window->GetContentRectangle(), 1.0f), OutCommandList, 0);
    Window->PaintDeferred(OutCommandList, TopLayerId + 1);
}

static void SettleWindowDrawCache(const TSharedPtr<FWindow>& Window, int32 FrameCount = 4)
{
    for (int32 Frame = 0; Frame < FrameCount; ++Frame)
    {
        FDrawCommandList Scratch;
        RecordWindow(Window, Scratch);
    }
}

static TSharedPtr<FMenu> MakeLongMenu(const TSharedPtr<IFontFace>& Font)
{
    TArray<String> Labels;
    Labels.Add("New");
    Labels.Add("Open");
    Labels.Add("Save");

    for (int32 Index = 0; Index < 30; ++Index)
    {
        CHAR Label[32];
        CString::Snprintf(Label, static_cast<int32>(sizeof(Label)), "Recent %d", Index);

        Labels.Add(String(Label));
    }

    return CreateMenu(Font, Labels);
}

static TSharedPtr<FBorder> MakePanel(const TSharedPtr<IFontFace>& Font)
{
    static const CHAR* RowLabels[] = { "Alpha", "Bravo", "Charlie", "Delta" };

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    for (const CHAR* Label : RowLabels)
    {
        FTextBlock::FDesc TextDesc;
        TextDesc.Text = Label;
        TextDesc.Font = Font;
        Column->AddSlot(FTextBlock::Create(TextDesc));
    }

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    BorderDesc.BorderColor     = FFloatColor(0.4f, 0.4f, 0.4f, 1.0f);
    BorderDesc.BorderThickness = 1.0f;
    BorderDesc.Padding         = FMargin(4);
    BorderDesc.Content         = Column;

    return FBorder::Create(BorderDesc);
}

static TSharedPtr<FBorder> MakeBigPanel(const TSharedPtr<IFontFace>& Font)
{
    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(MakePanel(Font));

    for (int32 Index = 0; Index < 40; ++Index)
    {
        CHAR Label[32];
        CString::Snprintf(Label, static_cast<int32>(sizeof(Label)), "Row %d", Index);

        FTextBlock::FDesc TextDesc;
        TextDesc.Text = Label;
        TextDesc.Font = Font;
        Column->AddSlot(FTextBlock::Create(TextDesc));
    }

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    BorderDesc.Padding         = FMargin(4);
    BorderDesc.Content         = Column;

    return FBorder::Create(BorderDesc);
}

bool DrawCacheEquivalence_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font  = CreateFont();
    TSharedPtr<FBorder>         Panel = MakePanel(Font);

    Panel->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Panel, FRectangle(IntVector2(10, 20), 200, 160));

    TEST_SECTION("The first recording is walked rather than replayed, and it is what gets kept");
    FDrawCommandList FirstPass;
    Record(Panel, FirstPass);

    TEST_EXPECT(Panel->HasDrawCache());
    TEST_EXPECT(FirstPass.Size() > 0);

    TEST_SECTION("The next recording replays that block and comes out the same command for command");
    FDrawCommandList ReplayPass;
    Record(Panel, ReplayPass);
    TEST_EXPECT(RecordingsMatch(FirstPass, ReplayPass));
    TEST_EXPECT(ReplayPass.WasFullyReplayed());

    TEST_SECTION("Releasing the block and walking the tree again lands on the same commands once more");
    Panel->ReleaseDrawCache();

    FDrawCommandList FreshPass;
    Record(Panel, FreshPass);

    TEST_EXPECT(!FreshPass.WasFullyReplayed());
    TEST_EXPECT(RecordingsMatch(ReplayPass, FreshPass));

    TEST_SECTION("An overlay holding the panel replays through the same path");
    TSharedPtr<FOverlay> Overlay = FOverlay::Create();
    Overlay->AddSlot(MakePanel(Font));
    Overlay->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Overlay, FRectangle(IntVector2(0, 0), 240, 200));

    FDrawCommandList OverlayFirst;
    Record(Overlay, OverlayFirst);

    FDrawCommandList OverlayReplay;
    Record(Overlay, OverlayReplay);

    TEST_EXPECT(OverlayReplay.WasFullyReplayed());
    TEST_EXPECT(RecordingsMatch(OverlayFirst, OverlayReplay));

    TEST_SECTION("A scroll box clips its content, and the clipped commands survive the round trip");
    TSharedPtr<FScrollBox> Scroller = FScrollBox::Create();
    Scroller->SetContent(MakePanel(Font));
    Scroller->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Scroller, FRectangle(IntVector2(0, 0), 120, 60));

    FDrawCommandList ScrollFirst;
    Record(Scroller, ScrollFirst);

    FDrawCommandList ScrollReplay;
    Record(Scroller, ScrollReplay);

    TEST_EXPECT(RecordingsMatch(ScrollFirst, ScrollReplay));

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheGuards_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    TSharedPtr<FBorder> Panel = MakePanel(CreateFont());
    Panel->SetDrawCachePolicy(EDrawCachePolicy::Always);

    const FRectangle Bounds = FRectangle(IntVector2(10, 20), 200, 160);
    LayoutElement(Panel, Bounds);

    FDrawCommandList Warmup;
    Record(Panel, Warmup);
    TEST_EXPECT(Panel->HasDrawCache());

    TEST_SECTION("Asking for a different layer rejects the block, because every command carries its layer");
    SettleDrawCache(Panel);

    FDrawCommandList OtherLayer;
    Panel->Draw(FDrawGeometry(Bounds, 1.0f), OtherLayer, 7);
    TEST_EXPECT(!OtherLayer.WasFullyReplayed());

    TEST_SECTION("A different DPI scale rejects it too, since the pixels were rounded at the old one");
    SettleDrawCache(Panel);

    FDrawCommandList OtherScale;
    Panel->Draw(FDrawGeometry(Bounds, 2.0f), OtherScale, 0);
    TEST_EXPECT(!OtherScale.WasFullyReplayed());

    TEST_SECTION("A clip that was not open when the block recorded rejects it");
    SettleDrawCache(Panel);

    FDrawCommandList Replayable;
    Record(Panel, Replayable);
    TEST_EXPECT(Replayable.WasFullyReplayed());

    FDrawCommandList Clipped;
    Clipped.PushClip(0, FRectangle(IntVector2(0, 0), 50, 50));
    Panel->Draw(FDrawGeometry(Bounds, 1.0f), Clipped, 0);
    Clipped.PopClip(0);
    TEST_EXPECT(!Clipped.WasFullyReplayed());

    TEST_SECTION("Moving the element rejects it, because the block holds absolute positions");
    SettleDrawCache(Panel);

    FDrawCommandList BeforeMove;
    Record(Panel, BeforeMove);
    TEST_EXPECT(BeforeMove.WasFullyReplayed());

    LayoutElement(Panel, FRectangle(IntVector2(40, 20), 200, 160));

    FDrawCommandList AfterMove;
    Record(Panel, AfterMove);
    TEST_EXPECT(!AfterMove.WasFullyReplayed());

    TEST_SECTION("Advancing the epoch drops every block at once");
    SettleDrawCache(Panel);

    FDrawCommandList BeforeEpoch;
    Record(Panel, BeforeEpoch);
    TEST_EXPECT(BeforeEpoch.WasFullyReplayed());

    DrawCacheEpoch::Advance();
    TEST_EXPECT(!Panel->HasDrawCache());

    FDrawCommandList AfterEpoch;
    Record(Panel, AfterEpoch);
    TEST_EXPECT(!AfterEpoch.WasFullyReplayed());

    TEST_SECTION("Turning the cache off on an element releases what it was holding");
    SettleDrawCache(Panel);
    TEST_EXPECT(Panel->HasDrawCache());

    Panel->SetDrawCachePolicy(EDrawCachePolicy::Never);
    TEST_EXPECT(!Panel->HasDrawCache());

    FDrawCommandList NeverPass;
    Record(Panel, NeverPass);
    TEST_EXPECT(!NeverPass.WasFullyReplayed());
    TEST_EXPECT(!Panel->HasDrawCache());

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheNesting_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font  = CreateFont();
    TSharedPtr<FBorder>         Inner = MakePanel(Font);

    FBorder::FDesc OuterDesc;
    OuterDesc.BackgroundColor = FFloatColor(0.2f, 0.2f, 0.2f, 1.0f);
    OuterDesc.Padding         = FMargin(6);
    OuterDesc.Content         = Inner;

    TSharedPtr<FBorder> Outer = FBorder::Create(OuterDesc);

    Outer->SetDrawCachePolicy(EDrawCachePolicy::Always);
    Inner->SetDrawCachePolicy(EDrawCachePolicy::Always);

    LayoutElement(Outer, FRectangle(IntVector2(0, 0), 240, 200));

    TEST_SECTION("Only the outermost candidate keeps a block, since its own covers everything below");
    FDrawCommandList FirstPass;
    Record(Outer, FirstPass);

    TEST_EXPECT(Outer->HasDrawCache());
    TEST_EXPECT(!Inner->HasDrawCache());

    TEST_SECTION("The outer block replays the whole subtree in one span");
    FDrawCommandList ReplayPass;
    Record(Outer, ReplayPass);
    TEST_EXPECT(ReplayPass.WasFullyReplayed());
    TEST_EXPECT(RecordingsMatch(FirstPass, ReplayPass));

    TEST_SECTION("Dirtying the inner element re-walks the outer one, because invalidation travels upward");
    Inner->InvalidatePaint();
    TEST_EXPECT(Outer->IsPaintDirty());

    FDrawCommandList AfterDirty;
    Record(Outer, AfterDirty);

    TEST_EXPECT(!AfterDirty.WasFullyReplayed());
    TEST_EXPECT(RecordingsMatch(ReplayPass, AfterDirty));

    TEST_SECTION("An element that refuses caching stops its ancestors from keeping it frozen");
    Inner->SetDrawCachePolicy(EDrawCachePolicy::Never);
    Outer->InvalidatePaint();

    FDrawCommandList BlockedPass;
    Record(Outer, BlockedPass);

    TEST_EXPECT(!Outer->HasDrawCache());

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheInvalidation_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font = CreateFont();

    TEST_SECTION("Hovering a button re-records it, so the hover fill is not left out of the replay");
    FButton::FDesc ButtonDesc;
    ButtonDesc.SetText("Save as...").SetFont(Font);

    TSharedPtr<FButton> Button = FButton::Create(ButtonDesc);
    Button->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Button, FRectangle(IntVector2(10, 10), 120, 32));

    SettleDrawCache(Button);

    FDrawCommandList AtRest;
    Record(Button, AtRest);
    TEST_EXPECT(AtRest.WasFullyReplayed());

    Button->OnMouseEntered(MakeMoveEvent(IntVector2(50, 20)));
    TEST_EXPECT(Button->IsPaintDirty());

    FDrawCommandList Hovered;
    Record(Button, Hovered);

    TEST_EXPECT(!Hovered.WasFullyReplayed());
    TEST_EXPECT(!RecordingsAgree(AtRest, Hovered));

    TEST_SECTION("And the hovered look is what a fresh walk of the hovered button produces");
    SettleDrawCache(Button);

    FDrawCommandList HoveredReplay;
    Record(Button, HoveredReplay);
    TEST_EXPECT(HoveredReplay.WasFullyReplayed());

    Button->ReleaseDrawCache();

    FDrawCommandList HoveredFresh;
    Record(Button, HoveredFresh);
    TEST_EXPECT(RecordingsMatch(HoveredReplay, HoveredFresh));

    TEST_SECTION("Checking a box re-records it");
    FCheckBox::FDesc CheckDesc;
    CheckDesc.SetText("Enabled").SetFont(Font);

    TSharedPtr<FCheckBox> CheckBox = FCheckBox::Create(CheckDesc);
    CheckBox->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(CheckBox, FRectangle(IntVector2(0, 0), 200, 20));

    SettleDrawCache(CheckBox);

    FDrawCommandList Unchecked;
    Record(CheckBox, Unchecked);
    TEST_EXPECT(Unchecked.WasFullyReplayed());

    CheckBox->SetCheckState(ECheckBoxState::Checked);
    TEST_EXPECT(CheckBox->IsPaintDirty());

    FDrawCommandList Checked;
    Record(CheckBox, Checked);

    TEST_EXPECT(!Checked.WasFullyReplayed());
    TEST_EXPECT(!RecordingsAgree(Unchecked, Checked));

    TEST_SECTION("Scrolling a box re-records it, because its children move under a clip that does not");
    TSharedPtr<FScrollBox> Scroller = FScrollBox::Create();
    Scroller->SetContent(MakePanel(Font));
    Scroller->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Scroller, FRectangle(IntVector2(0, 0), 120, 40));

    SettleDrawCache(Scroller);

    FDrawCommandList Unscrolled;
    Record(Scroller, Unscrolled);
    TEST_EXPECT(Unscrolled.WasFullyReplayed());

    Scroller->SetScrollOffset(20);
    TEST_EXPECT(Scroller->IsPaintDirty());

    Scroller->Arrange(Scroller->GetContentRectangle());

    FDrawCommandList Scrolled;
    Record(Scroller, Scrolled);
    TEST_EXPECT(!Scrolled.WasFullyReplayed());

    TEST_SECTION("Pressing a button re-records it as well");
    Button->OnMouseLeft(MakeMoveEvent(IntVector2(900, 900)));
    SettleDrawCache(Button);

    FDrawCommandList Released;
    Record(Button, Released);
    TEST_EXPECT(Released.WasFullyReplayed());

    Button->OnMouseButtonDown(MakeButtonEvent(EInputEventType::MouseButtonDown, IntVector2(50, 20)));

    FDrawCommandList Pressed;
    Record(Button, Pressed);
    TEST_EXPECT(!Pressed.WasFullyReplayed());
    TEST_EXPECT(!RecordingsAgree(Released, Pressed));

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheAtlasRevision_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<FTrueTypeFontFace> Font =
        FTrueTypeFontFace::CreateFromFile(Paths::GetAssetDir() + "/Editor/Fonts/consola.ttf", 16);

    if (!Font)
    {
        LOG_WARNING("[DrawCache] Skipping the atlas-revision test, consola.ttf was not found");
        TEST_END();
    }

    TSharedPtr<FBorder> Panel = MakePanel(Font);

    Panel->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Panel, FRectangle(IntVector2(0, 0), 200, 160));

    SettleDrawCache(Panel);

    TEST_SECTION("Text in the block is tracked back to the atlas its glyphs were packed from");
    TEST_EXPECT(Panel->HasDrawCache());

    const FFontAtlas* Atlas = Font->GetAtlas();
    TEST_EXPECT(Atlas != nullptr);

    FDrawCommandList Replayed;
    Record(Panel, Replayed);
    TEST_EXPECT(Replayed.WasFullyReplayed());

    TEST_SECTION("Repacking the atlas moves every glyph, so the block stops being replayable");
    const uint64 RevisionBefore = Atlas ? Atlas->GetRevision() : 0;

    Font->ShapeText("\xC3\xA5\xC3\xA4\xC3\xB6\xC3\x85\xC3\x84\xC3\x96\xC2\xA7\xC2\xB1\xC2\xB5\xC2\xB6");

    if (Atlas && Atlas->GetRevision() != RevisionBefore)
    {
        FDrawCommandList AfterRepack;
        Record(Panel, AfterRepack);
        TEST_EXPECT(!AfterRepack.WasFullyReplayed());
    }

    TEST_SECTION("And the recording it settles back into still matches a walk of the tree");
    SettleDrawCache(Panel);

    FDrawCommandList SettledReplay;
    Record(Panel, SettledReplay);

    Panel->ReleaseDrawCache();

    FDrawCommandList SettledFresh;
    Record(Panel, SettledFresh);

    TEST_EXPECT(RecordingsMatch(SettledReplay, SettledFresh));

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheGeometry_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    IConsoleVariable* GeometryVariable = FConsoleManager::Get().FindConsoleVariable("UI.DrawCache.Geometry");
    TEST_EXPECT(GeometryVariable != nullptr);

    if (!GeometryVariable)
    {
        TEST_END();
    }

    const bool bWasEnabled = GeometryVariable->GetBool();
    GeometryVariable->SetAsBool(true, EConsoleVariableFlags::SetByCode);

    const TSharedPtr<IFontFace> Font  = CreateFont();
    TSharedPtr<FBorder>         Panel = MakePanel(Font);

    Panel->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Panel, FRectangle(IntVector2(10, 20), 200, 160));

    TEST_SECTION("The first build tessellates a fresh recording, which is the answer everything is held to");
    FDrawCommandList FirstCommands;
    Record(Panel, FirstCommands);

    FUIDrawData FirstData;
    FirstData.BuildFromCommandList(FirstCommands);

    const TArray<FUIVertex>            FirstVertices = FirstData.GetVertices();
    const TArray<uint32>               FirstIndices  = FirstData.GetIndices();
    const TArray<FUIShapeInstance>     FirstShapes   = FirstData.GetShapeInstances();
    const TArray<FUITextGlyphInstance> FirstGlyphs   = FirstData.GetTextGlyphInstances();
    const int32                        FirstBatches  = FirstData.GetBatches().Size();

    TEST_EXPECT(FirstVertices.Size() > 0);

    TEST_SECTION("The first build over replayed commands is where the triangles get kept, so it still walks them");
    FDrawCommandList CaptureCommands;
    Record(Panel, CaptureCommands);
    TEST_EXPECT(CaptureCommands.WasFullyReplayed());

    FUIDrawData CaptureData;
    CaptureData.BuildFromCommandList(CaptureCommands);
    TEST_EXPECT(!CaptureData.IsFullyReplayed());

    TEST_SECTION("The build after that splices them back in and lands on the same streams");
    FDrawCommandList ReplayCommands;
    Record(Panel, ReplayCommands);
    TEST_EXPECT(ReplayCommands.WasFullyReplayed());

    FUIDrawData ReplayData;
    ReplayData.BuildFromCommandList(ReplayCommands);

    TEST_EXPECT_EQ(ReplayData.GetVertices().Size(), FirstVertices.Size());
    TEST_EXPECT_EQ(ReplayData.GetIndices().Size(), FirstIndices.Size());
    TEST_EXPECT_EQ(ReplayData.GetShapeInstances().Size(), FirstShapes.Size());
    TEST_EXPECT_EQ(ReplayData.GetTextGlyphInstances().Size(), FirstGlyphs.Size());

    TEST_SECTION("Down to the bytes, so the upload can be skipped outright");
    bool bVerticesMatch = true;
    for (int32 Index = 0; Index < FirstVertices.Size(); ++Index)
    {
        const FUIVertex& Left  = FirstVertices[Index];
        const FUIVertex& Right = ReplayData.GetVertices()[Index];

        if (Left.Position != Right.Position || Left.TexCoord != Right.TexCoord || Left.Color != Right.Color)
        {
            bVerticesMatch = false;
            break;
        }
    }

    TEST_EXPECT(bVerticesMatch);

    bool bIndicesMatch = true;
    for (int32 Index = 0; Index < FirstIndices.Size(); ++Index)
    {
        if (FirstIndices[Index] != ReplayData.GetIndices()[Index])
        {
            bIndicesMatch = false;
            break;
        }
    }

    TEST_EXPECT(bIndicesMatch);

    bool bGlyphsMatch = true;
    for (int32 Index = 0; Index < FirstGlyphs.Size(); ++Index)
    {
        const FUITextGlyphInstance& Left  = FirstGlyphs[Index];
        const FUITextGlyphInstance& Right = ReplayData.GetTextGlyphInstances()[Index];

        if (Left.Position != Right.Position || Left.Size != Right.Size || Left.Color != Right.Color)
        {
            bGlyphsMatch = false;
            break;
        }
    }

    TEST_EXPECT(bGlyphsMatch);

    TEST_SECTION("The batches are rebuilt whole rather than cut at the seam the splice made");
    TEST_EXPECT_EQ(ReplayData.GetBatches().Size(), FirstBatches);

    TEST_SECTION("A fully replayed build says so, which is what lets the geometry hash be skipped");
    TEST_EXPECT(ReplayData.IsFullyReplayed());
    TEST_EXPECT(ReplayData.GetReplayFingerprint() != 0);

    FDrawCommandList SecondReplayCommands;
    Record(Panel, SecondReplayCommands);

    FUIDrawData SecondReplayData;
    SecondReplayData.BuildFromCommandList(SecondReplayCommands);

    TEST_EXPECT(SecondReplayData.IsFullyReplayed());
    TEST_EXPECT_EQ(SecondReplayData.GetReplayFingerprint(), ReplayData.GetReplayFingerprint());

    TEST_SECTION("Something drawn outside the block stops the build counting as fully replayed");
    FDrawCommandList MixedCommands;
    Record(Panel, MixedCommands);
    MixedCommands.AddBox(9, FRectangle(IntVector2(0, 0), 4, 4), FFloatColor::White);

    FUIDrawData MixedData;
    MixedData.BuildFromCommandList(MixedCommands);
    TEST_EXPECT(!MixedData.IsFullyReplayed());

    GeometryVariable->SetAsBool(bWasEnabled, EConsoleVariableFlags::SetByCode);

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheMenuHost_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font = CreateFont();

    TSharedPtr<FMenuHost> Host = FMenuHost::Create();
    TSharedPtr<FMenu>     Menu = MakeLongMenu(Font);

    LayoutElement(Menu, FRectangle(IntVector2(0, 0), 200, 600));

    Host->AddChild(Menu, FRectangle(IntVector2(20, 40), 200, 600));
    LayoutElement(Host, FRectangle(IntVector2(0, 0), 800, 800));

    TEST_SECTION("An open menu is drawn through its host, and settles into a recording worth keeping");
    SettleDrawCache(Host);
    TEST_EXPECT(RecordingReplays(Host));

    FDrawCommandList WhileOpen;
    Record(Host, WhileOpen);
    TEST_EXPECT(WhileOpen.FindTextCommand("Open") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Closing the menu takes it out of the host's recording rather than leaving a ghost behind");
    Host->RemoveChild(Menu);
    LayoutElement(Host, FRectangle(IntVector2(0, 0), 800, 800));

    FDrawCommandList AfterClose;
    Record(Host, AfterClose);

    TEST_EXPECT(Host->IsEmpty());
    TEST_EXPECT_EQ(AfterClose.FindTextCommand("Open"), FDrawCommandList::InvalidIndex);
    TEST_EXPECT(ReplayMatchesFreshWalk(Host));

    TEST_SECTION("The closed menu no longer claims the host as its parent, so it cannot dirty it from off the tree");
    TEST_EXPECT(Menu->GetParentElement() == nullptr);

    TEST_SECTION("Reopening it brings it back, and moving it afterwards takes the move with it");
    Host->AddChild(Menu, FRectangle(IntVector2(20, 40), 200, 600));
    LayoutElement(Host, FRectangle(IntVector2(0, 0), 800, 800));
    SettleDrawCache(Host);
    TEST_EXPECT(RecordingReplays(Host));

    Host->SetChildBounds(Menu, FRectangle(IntVector2(300, 40), 200, 600));
    LayoutElement(Host, FRectangle(IntVector2(0, 0), 800, 800));

    TEST_EXPECT(ReplayMatchesFreshWalk(Host));

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheWindowDeferred_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FScopedMenuServices    MenuServices;

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    Window->SetContent(MakeBigPanel(Font));
    FApplication::LayoutWindow(Window);

    TEST_SECTION("A window still long enough to cache its recording replays it");
    SettleWindowDrawCache(Window);
    TEST_EXPECT(RecordingReplays(Window));

    TEST_SECTION("A menu left open long enough for the window to settle again is still painted every frame");
    TSharedPtr<FMenu> Menu   = CreateMenu(Font, { "New", "Open" });
    FMenuHandle       Handle = FMenuStack::Get().PushMenu(Window, FRectangle(IntVector2(10, 10), 40, 24), EMenuPlacement::BelowLeftAligned, Menu);

    TEST_EXPECT(Handle != nullptr);
    TEST_EXPECT(Handle->bIsInline);

    FApplication::LayoutWindow(Window);

    SettleWindowDrawCache(Window);
    TEST_EXPECT(RecordingReplays(Window));

    FDrawCommandList WithMenu;
    RecordWindow(Window, WithMenu);

    TEST_EXPECT(WithMenu.FindTextCommand("Open") != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(WithMenu.FindTextCommand("Alpha") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Dismissing it takes it away again, however settled the window had become");
    FMenuStack::Get().DismissAll();
    FApplication::LayoutWindow(Window);

    FDrawCommandList AfterDismiss;
    RecordWindow(Window, AfterDismiss);

    TEST_EXPECT_EQ(AfterDismiss.FindTextCommand("Open"), FDrawCommandList::InvalidIndex);
    TEST_EXPECT(AfterDismiss.FindTextCommand("Alpha") != FDrawCommandList::InvalidIndex);

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheMenuBar_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FScopedMenuServices    MenuServices;

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    TSharedPtr<FMenuBar> MenuBar = FMenuBar::Create();

    TSharedPtr<FMenuAnchor> FileAnchor   = MenuBar->AddMenu("File", Font, CreateMenu(Font, { "New", "Open" }));
    TSharedPtr<FMenuAnchor> WindowAnchor = MenuBar->AddMenu("Window", Font, CreateMenu(Font, { "Viewport", "Console" }));

    for (int32 Index = 0; Index < 60; ++Index)
    {
        CHAR Label[32];
        CString::Snprintf(Label, static_cast<int32>(sizeof(Label)), "Filler %d", Index);

        MenuBar->AddMenu(Label, Font, CreateMenu(Font, { "One", "Two" }));
    }

    Window->SetContent(MenuBar);
    FApplication::LayoutWindow(Window);

    TEST_SECTION("A settled bar with nothing open replays what a fresh walk would draw");
    SettleDrawCache(MenuBar);
    TEST_EXPECT(RecordingReplays(MenuBar));
    TEST_EXPECT(ReplayMatchesFreshWalk(MenuBar));

    TEST_SECTION("Opening a menu re-records the button that owns it, rather than replaying its closed chrome");
    SettleDrawCache(MenuBar);
    WindowAnchor->Open();
    FApplication::LayoutWindow(Window);

    TEST_EXPECT(ReplayMatchesFreshWalk(MenuBar));
    TEST_EXPECT(WindowAnchor->IsOpen());

    TEST_SECTION("Switching menus leaves the first button looking closed and the second looking open");
    SettleDrawCache(MenuBar);
    FileAnchor->Open();
    FApplication::LayoutWindow(Window);

    TEST_EXPECT(ReplayMatchesFreshWalk(MenuBar));
    TEST_EXPECT(FileAnchor->IsOpen());
    TEST_EXPECT(!WindowAnchor->IsOpen());

    TEST_SECTION("A menu the stack dismisses behind the anchor's back still clears the button's chrome");
    SettleDrawCache(MenuBar);
    TEST_EXPECT(RecordingReplays(MenuBar));

    FMenuStack::Get().DismissAll();
    FApplication::LayoutWindow(Window);

    TEST_EXPECT(ReplayMatchesFreshWalk(MenuBar));
    TEST_EXPECT(!FileAnchor->IsOpen());

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheToolTip_Test()
{
    TEST_BEGIN();

    FScopedStubApplication Application;
    FScopedMenuServices    MenuServices;

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace> Font   = CreateFont();
    TSharedPtr<FWindow>         Window = Application.CreateWindow(IntVector2(800, 600), IntVector2(0, 0));

    TSharedPtr<FBorder> Panel = MakeBigPanel(Font);
    Window->SetContent(Panel);
    FApplication::LayoutWindow(Window);

    SettleWindowDrawCache(Window);
    TEST_EXPECT(RecordingReplays(Window));

    TEST_SECTION("A tip that is still up once the window has gone quiet again is painted every frame");
    FToolTipService& ToolTips = FToolTipService::Get();
    ToolTips.NotifyCursorMoved(IntVector2(300, 200));
    ToolTips.RequestTextToolTip(Panel, "Opens the file", Font, EToolTipPlacement::FollowCursor, 0.5f);
    ToolTips.Tick(1.0f);

    TEST_EXPECT(ToolTips.IsShowing());
    FApplication::LayoutWindow(Window);

    SettleWindowDrawCache(Window);
    TEST_EXPECT(RecordingReplays(Window));

    FDrawCommandList WithTip;
    RecordWindow(Window, WithTip);

    TEST_EXPECT(WithTip.FindTextCommand("Opens the file") != FDrawCommandList::InvalidIndex);

    TEST_SECTION("Dismissing it takes it away rather than leaving it replaying out of the menu host");
    ToolTips.DismissToolTip();
    FApplication::LayoutWindow(Window);

    FDrawCommandList AfterDismiss;
    RecordWindow(Window, AfterDismiss);

    TEST_EXPECT_EQ(AfterDismiss.FindTextCommand("Opens the file"), FDrawCommandList::InvalidIndex);

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheLayerOrder_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    IConsoleVariable* GeometryVariable = FConsoleManager::Get().FindConsoleVariable("UI.DrawCache.Geometry");
    TEST_EXPECT(GeometryVariable != nullptr);

    if (!GeometryVariable)
    {
        TEST_END();
    }

    const bool bWasEnabled = GeometryVariable->GetBool();
    GeometryVariable->SetAsBool(true, EConsoleVariableFlags::SetByCode);

    TSharedPtr<FLayerStepDownElement> Element = MakeSharedPtr<FLayerStepDownElement>();
    Element->SetDrawCachePolicy(EDrawCachePolicy::Always);
    LayoutElement(Element, FRectangle(IntVector2(10, 20), 60, 40));

    FDrawCommandList FreshCommands;
    FreshCommands.SetDrawCacheSuppressed(true);
    Record(Element, FreshCommands);

    FUIDrawData FreshData;
    FreshData.BuildFromCommandList(FreshCommands);

    TEST_SECTION("A block whose layers step back down still sorts as one run, so its geometry is kept and spliced");
    for (int32 Frame = 0; Frame < 2; ++Frame)
    {
        FDrawCommandList Warmup;
        Record(Element, Warmup);

        FUIDrawData WarmupData;
        WarmupData.BuildFromCommandList(Warmup);
    }

    FDrawCommandList ReplayCommands;
    Record(Element, ReplayCommands);
    TEST_EXPECT(ReplayCommands.WasFullyReplayed());

    FUIDrawData ReplayData;
    ReplayData.BuildFromCommandList(ReplayCommands);
    TEST_EXPECT(ReplayData.IsFullyReplayed());

    TEST_SECTION("And the spliced streams are the ones a fresh build sorts into");
    TEST_EXPECT_EQ(ReplayData.GetVertices().Size(), FreshData.GetVertices().Size());
    TEST_EXPECT_EQ(ReplayData.GetShapeInstances().Size(), FreshData.GetShapeInstances().Size());
    TEST_EXPECT_EQ(ReplayData.GetBatches().Size(), FreshData.GetBatches().Size());

    bool bShapesMatch = ReplayData.GetShapeInstances().Size() == FreshData.GetShapeInstances().Size();
    for (int32 Index = 0; bShapesMatch && Index < FreshData.GetShapeInstances().Size(); ++Index)
    {
        bShapesMatch = Memory::Memcmp(&ReplayData.GetShapeInstances()[Index], &FreshData.GetShapeInstances()[Index], sizeof(FUIShapeInstance)) == 0;
    }

    TEST_EXPECT(bShapesMatch);

    bool bVerticesMatch = ReplayData.GetVertices().Size() == FreshData.GetVertices().Size();
    for (int32 Index = 0; bVerticesMatch && Index < FreshData.GetVertices().Size(); ++Index)
    {
        const FUIVertex& Left  = ReplayData.GetVertices()[Index];
        const FUIVertex& Right = FreshData.GetVertices()[Index];
        bVerticesMatch = Left.Position == Right.Position && Left.Color == Right.Color;
    }

    TEST_EXPECT(bVerticesMatch);

    GeometryVariable->SetAsBool(bWasEnabled, EConsoleVariableFlags::SetByCode);

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCacheTranslation_Test()
{
    TEST_BEGIN();

    DrawCacheRegistry::ReleaseAll();

    const TSharedPtr<IFontFace>  Font  = CreateFont();
    TSharedPtr<FBorder>          Panel = MakePanel(Font);
    TSharedPtr<FCompoundElement> Frame = MakeSharedPtr<FCompoundElement>();

    Panel->SetDrawCachePolicy(EDrawCachePolicy::Always);
    Frame->SetContent(Panel);

    LayoutElement(Frame, FRectangle(IntVector2(0, 0), 200, 160));
    SettleDrawCache(Frame);

    TEST_SECTION("Moving the frame without resizing carries the panel along, so it stays clean");
    LayoutElement(Frame, FRectangle(IntVector2(30, 40), 200, 160));
    TEST_EXPECT(!Panel->IsPaintDirty());
    TEST_EXPECT(Frame->IsPaintDirty());

    TEST_SECTION("The panel replays its recording moved, and that agrees with walking it where it stands now");
    FDrawCommandList Moved;
    Record(Frame, Moved);
    TEST_EXPECT(Moved.GetReplayedCommandCount() > 0);

    FDrawCommandList Fresh;
    Fresh.SetDrawCacheSuppressed(true);
    Record(Frame, Fresh);
    TEST_EXPECT(RecordingsMatch(Moved, Fresh));

    TEST_SECTION("A resize is not a move, so the panel records again");
    LayoutElement(Frame, FRectangle(IntVector2(30, 40), 220, 160));
    TEST_EXPECT(Panel->IsPaintDirty());

    TEST_SECTION("A child placed where its moving parent was is dirtied, while one that moves along is not");
    TSharedPtr<FChildPlacementHost> Host  = MakeSharedPtr<FChildPlacementHost>();
    TSharedPtr<FVisualElement>      Child = MakeSharedPtr<FCompoundElement>();
    Host->SetChild(Child);

    LayoutElement(Host, FRectangle(IntVector2(0, 0), 100, 100));
    FDrawCommandList Settle;
    Record(Host, Settle);

    LayoutElement(Host, FRectangle(IntVector2(10, 0), 100, 100));
    TEST_EXPECT(!Child->IsPaintDirty());

    Record(Host, Settle);
    Host->bPinChild = true;
    Host->InvalidateArrange();
    LayoutElement(Host, FRectangle(IntVector2(20, 0), 100, 100));
    TEST_EXPECT(Child->IsPaintDirty());

    DrawCacheRegistry::ReleaseAll();
    TEST_END();
}

bool DrawCommandPayload_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateFont();
    const FRectangle      Bounds(IntVector2(0, 0), 40, 20);
    const FCornerRadii    Radii(1.0f, 2.0f, 3.0f, 4.0f);
    const FFloatColor     Tint(1.0f, 1.0f, 1.0f, 1.0f);

    TEST_SECTION("Each command keeps the payload its type owns");
    FDrawCommandList Commands;
    Commands.AddRoundedBottomBar(0, Bounds, Radii, 2.0f, Tint, 6.0f);
    Commands.AddRoundedAccentRing(0, Bounds, Radii, 2.0f, Tint, 0.25f, 0.5f);
    Commands.AddText(0, Bounds, "Text", Font.Get(), Tint);

    const TArray<FDrawCommand>& Recorded = Commands.GetCommands();
    TEST_EXPECT_EQ(Recorded.Size(), 3);
    TEST_EXPECT_EQ(Recorded[0].GetFadeWidth(), 6.0f);
    TEST_EXPECT(Recorded[0].CornerRadius == Radii);
    TEST_EXPECT_EQ(Recorded[1].GetFadeFraction(), 0.25f);
    TEST_EXPECT_EQ(Recorded[1].GetTrailAlpha(), 0.5f);
    TEST_EXPECT(Recorded[2].Font == Font.Get());

    TEST_SECTION("A different fade is a difference, and so is a different face");
    FDrawCommandList OtherFade;
    OtherFade.AddRoundedBottomBar(0, Bounds, Radii, 2.0f, Tint, 7.0f);
    OtherFade.AddRoundedAccentRing(0, Bounds, Radii, 2.0f, Tint, 0.25f, 0.5f);
    OtherFade.AddText(0, Bounds, "Text", Font.Get(), Tint);
    TEST_EXPECT(!RecordingsAgree(Commands, OtherFade));

    TSharedPtr<IFontFace> OtherFont = CreateFont();
    FDrawCommandList      OtherFace;
    OtherFace.AddRoundedBottomBar(0, Bounds, Radii, 2.0f, Tint, 6.0f);
    OtherFace.AddRoundedAccentRing(0, Bounds, Radii, 2.0f, Tint, 0.25f, 0.5f);
    OtherFace.AddText(0, Bounds, "Text", OtherFont.Get(), Tint);
    TEST_EXPECT(!RecordingsAgree(Commands, OtherFace));

    TEST_SECTION("The same recording twice agrees");
    FDrawCommandList Same;
    Same.AddRoundedBottomBar(0, Bounds, Radii, 2.0f, Tint, 6.0f);
    Same.AddRoundedAccentRing(0, Bounds, Radii, 2.0f, Tint, 0.25f, 0.5f);
    Same.AddText(0, Bounds, "Text", Font.Get(), Tint);
    TEST_EXPECT(RecordingsAgree(Commands, Same));

    TEST_END();
}
