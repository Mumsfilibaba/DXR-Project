#include "LayoutTests.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Application.h>
#include <Application/Layout/LayoutTypes.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Elements/BoxElements.h>
#include <Application/Elements/ScrollBoxElement.h>
#include <Application/Elements/TextBlockElement.h>
#include <Application/Elements/WindowElement.h>

static TSharedPtr<IFontFace> CreateTestFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static TSharedPtr<FTextBlockElement> CreateTextBlock(const CHAR* Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlockElement::FInitializer Initializer;
    Initializer.Text = Text;
    Initializer.Font = Font;
    return FTextBlockElement::Create(Initializer);
}

bool Margin_Test()
{
    TEST_BEGIN();

    TEST_SECTION("A uniform margin applies to every side");
    const FMargin Uniform(4);
    TEST_EXPECT_EQ(Uniform.Left, 4);
    TEST_EXPECT_EQ(Uniform.Top, 4);
    TEST_EXPECT_EQ(Uniform.Right, 4);
    TEST_EXPECT_EQ(Uniform.Bottom, 4);
    TEST_EXPECT_EQ(Uniform.GetTotalHorizontal(), 8);
    TEST_EXPECT_EQ(Uniform.GetTotalVertical(), 8);

    TEST_SECTION("The two-argument form is horizontal then vertical");
    const FMargin Axes(10, 6);
    TEST_EXPECT_EQ(Axes.Left, 10);
    TEST_EXPECT_EQ(Axes.Right, 10);
    TEST_EXPECT_EQ(Axes.Top, 6);
    TEST_EXPECT_EQ(Axes.Bottom, 6);
    TEST_EXPECT_EQ(Axes.GetTotalHorizontal(), 20);
    TEST_EXPECT_EQ(Axes.GetTotalVertical(), 12);

    TEST_SECTION("The four-argument form goes left, top, right, bottom");
    const FMargin Sides(1, 2, 3, 4);
    TEST_EXPECT_EQ(Sides.GetTotalHorizontal(), 4);
    TEST_EXPECT_EQ(Sides.GetTotalVertical(), 6);

    TEST_SECTION("Equality compares every side");
    TEST_EXPECT(FMargin(1, 2, 3, 4) == Sides);
    TEST_EXPECT(FMargin(1, 2, 3, 5) != Sides);

    TEST_END();
}

bool Rectangle_Test()
{
    TEST_BEGIN();

    const FRectangle Bounds(IntVector2(10, 20), 100, 50);

    TEST_SECTION("The edges are one past the far side");
    TEST_EXPECT_EQ(Bounds.GetRight(), 110);
    TEST_EXPECT_EQ(Bounds.GetBottom(), 70);
    TEST_EXPECT(!Bounds.IsEmpty());

    TEST_SECTION("Deflate moves the origin in and shrinks the extent");
    const FRectangle Deflated = Bounds.Deflate(FMargin(5, 4, 3, 2));
    TEST_EXPECT_EQ(Deflated.Position.X, 15);
    TEST_EXPECT_EQ(Deflated.Position.Y, 24);
    TEST_EXPECT_EQ(Deflated.Width, 92);
    TEST_EXPECT_EQ(Deflated.Height, 44);

    TEST_SECTION("Deflate clamps at zero rather than going negative");
    const FRectangle Collapsed = Bounds.Deflate(FMargin(200));
    TEST_EXPECT_EQ(Collapsed.Width, 0);
    TEST_EXPECT_EQ(Collapsed.Height, 0);
    TEST_EXPECT(Collapsed.IsEmpty());

    TEST_SECTION("Intersect returns the overlap");
    const FRectangle Overlap = Bounds.Intersect(FRectangle(IntVector2(60, 0), 100, 40));
    TEST_EXPECT_EQ(Overlap.Position.X, 60);
    TEST_EXPECT_EQ(Overlap.Position.Y, 20);
    TEST_EXPECT_EQ(Overlap.Width, 50);
    TEST_EXPECT_EQ(Overlap.Height, 20);

    TEST_SECTION("Disjoint rectangles intersect to nothing");
    const FRectangle Disjoint = Bounds.Intersect(FRectangle(IntVector2(500, 500), 10, 10));
    TEST_EXPECT(Disjoint.IsEmpty());
    TEST_EXPECT_EQ(Disjoint.Width, 0);
    TEST_EXPECT_EQ(Disjoint.Height, 0);

    TEST_END();
}

bool FixedWidthFontFace_Test()
{
    TEST_BEGIN();

    const FFixedWidthFontFace Font(8, 16);

    TEST_SECTION("The metrics come straight from the two numbers");
    TEST_EXPECT_EQ(Font.GetLineHeight(), 16);
    TEST_EXPECT_EQ(Font.GetCharacterAdvance('A'), 8);
    TEST_EXPECT_EQ(Font.GetAscent(), 12);
    TEST_EXPECT_EQ(Font.GetDescent(), 4);

    TEST_SECTION("Width is the character count times the advance");
    TEST_EXPECT_EQ(Font.MeasureWidth(StringView("")), 0);
    TEST_EXPECT_EQ(Font.MeasureWidth(StringView("Test")), 32);

    TEST_SECTION("Hit testing an offset rounds to the nearest position between characters");
    const StringView Text("Test");
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, 0), 0);
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, 3), 0);
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, 5), 1);
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, 12), 2);

    TEST_SECTION("Hit testing clamps into the text");
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, -100), 0);
    TEST_EXPECT_EQ(Font.FindCharacterIndexAtOffset(Text, 1000), 4);

    TEST_SECTION("A degenerate face still measures");
    const FFixedWidthFontFace Degenerate(0, 0);
    TEST_EXPECT_EQ(Degenerate.GetLineHeight(), 1);
    TEST_EXPECT_EQ(Degenerate.GetCharacterAdvance('A'), 1);

    TEST_END();
}

bool TextBlockDesiredSize_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TEST_SECTION("The desired size is the measured text plus the margin");
    FTextBlockElement::FInitializer Initializer;
    Initializer.Text   = "Hello";
    Initializer.Font   = Font;
    Initializer.Margin = FMargin(2, 3);

    TSharedPtr<FTextBlockElement> TextBlock = FTextBlockElement::Create(Initializer);

    const IntVector2 DesiredSize = TextBlock->PrepareDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, (5 * 8) + 4);
    TEST_EXPECT_EQ(DesiredSize.Y, 16 + 6);
    TEST_EXPECT(TextBlock->GetCachedDesiredSize() == DesiredSize);

    TEST_SECTION("A block with no face asks only for its margin");
    FTextBlockElement::FInitializer FontlessInitializer;
    FontlessInitializer.Text   = "Hello";
    FontlessInitializer.Margin = FMargin(2);

    TSharedPtr<FTextBlockElement> Fontless = FTextBlockElement::Create(FontlessInitializer);

    const IntVector2 FontlessSize = Fontless->PrepareDesiredSize();
    TEST_EXPECT_EQ(FontlessSize.X, 4);
    TEST_EXPECT_EQ(FontlessSize.Y, 4);

    TEST_END();
}

bool VerticalBoxLayout_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FTextBlockElement> First  = CreateTextBlock("AAAA", Font);
    TSharedPtr<FTextBlockElement> Second = CreateTextBlock("BB", Font);

    TSharedPtr<FVerticalBoxElement> Box = FVerticalBoxElement::Create();
    Box->AddSlot(First);
    Box->AddSlot(Second);

    TEST_SECTION("Heights stack and widths take the widest child");
    const IntVector2 DesiredSize = Box->PrepareDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, 4 * 8);
    TEST_EXPECT_EQ(DesiredSize.Y, 2 * 16);

    TEST_SECTION("Auto slots get exactly their desired height, stacked downward");
    Box->Tick(FRectangle(IntVector2(0, 0), 200, 100));
    TEST_EXPECT_EQ(First->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(First->GetContentRectangle().Height, 16);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Position.Y, 16);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Height, 16);

    TEST_SECTION("Two fill slots split the leftover height");
    TSharedPtr<FTextBlockElement> FillFirst  = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> FillSecond = CreateTextBlock("B", Font);

    TSharedPtr<FVerticalBoxElement> FillBox = FVerticalBoxElement::Create();
    FillBox->AddSlot(FillFirst).SetFillCoefficient(1.0f);
    FillBox->AddSlot(FillSecond).SetFillCoefficient(1.0f);

    FillBox->PrepareDesiredSize();
    FillBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(FillFirst->GetContentRectangle().Height, 50);
    TEST_EXPECT_EQ(FillSecond->GetContentRectangle().Height, 50);
    TEST_EXPECT_EQ(FillSecond->GetContentRectangle().Position.Y, 50);

    TEST_SECTION("The last fill slot absorbs the rounding remainder");
    TSharedPtr<FTextBlockElement> OddFirst  = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> OddSecond = CreateTextBlock("B", Font);

    TSharedPtr<FVerticalBoxElement> OddBox = FVerticalBoxElement::Create();
    OddBox->AddSlot(OddFirst).SetFillCoefficient(1.0f);
    OddBox->AddSlot(OddSecond).SetFillCoefficient(1.0f);

    OddBox->PrepareDesiredSize();
    OddBox->Tick(FRectangle(IntVector2(0, 0), 200, 101));

    const int32 TotalHeight = OddFirst->GetContentRectangle().Height + OddSecond->GetContentRectangle().Height;
    TEST_EXPECT_EQ(TotalHeight, 101);
    TEST_EXPECT_EQ(OddFirst->GetContentRectangle().Height, 50);
    TEST_EXPECT_EQ(OddSecond->GetContentRectangle().Height, 51);

    TEST_SECTION("A fill slot only gets what an auto slot leaves behind");
    TSharedPtr<FTextBlockElement> AutoChild = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> FillChild = CreateTextBlock("B", Font);

    TSharedPtr<FVerticalBoxElement> MixedBox = FVerticalBoxElement::Create();
    MixedBox->AddSlot(FillChild).SetFillCoefficient(1.0f);
    MixedBox->AddSlot(AutoChild);

    MixedBox->PrepareDesiredSize();
    MixedBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(FillChild->GetContentRectangle().Height, 84);
    TEST_EXPECT_EQ(AutoChild->GetContentRectangle().Position.Y, 84);
    TEST_EXPECT_EQ(AutoChild->GetContentRectangle().Height, 16);

    TEST_END();
}

bool HorizontalBoxLayout_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FTextBlockElement> First  = CreateTextBlock("AAAA", Font);
    TSharedPtr<FTextBlockElement> Second = CreateTextBlock("BB", Font);

    TSharedPtr<FHorizontalBoxElement> Box = FHorizontalBoxElement::Create();
    Box->AddSlot(First);
    Box->AddSlot(Second);

    TEST_SECTION("Widths stack and heights take the tallest child");
    const IntVector2 DesiredSize = Box->PrepareDesiredSize();
    TEST_EXPECT_EQ(DesiredSize.X, (4 * 8) + (2 * 8));
    TEST_EXPECT_EQ(DesiredSize.Y, 16);

    TEST_SECTION("Auto slots get their desired width, stacked rightward");
    Box->Tick(FRectangle(IntVector2(0, 0), 200, 100));
    TEST_EXPECT_EQ(First->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(First->GetContentRectangle().Width, 32);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Position.X, 32);
    TEST_EXPECT_EQ(Second->GetContentRectangle().Width, 16);

    TEST_SECTION("Two fill slots split the leftover width");
    TSharedPtr<FTextBlockElement> FillFirst  = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> FillSecond = CreateTextBlock("B", Font);

    TSharedPtr<FHorizontalBoxElement> FillBox = FHorizontalBoxElement::Create();
    FillBox->AddSlot(FillFirst).SetFillCoefficient(1.0f);
    FillBox->AddSlot(FillSecond).SetFillCoefficient(3.0f);

    FillBox->PrepareDesiredSize();
    FillBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(FillFirst->GetContentRectangle().Width, 50);
    TEST_EXPECT_EQ(FillSecond->GetContentRectangle().Width, 150);

    TEST_END();
}

bool BoxSlotAlignment_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    // Every child measures 32x16, so a 200x100 slot leaves plenty of room to align inside
    TEST_SECTION("Left, center and right place a narrow child horizontally");
    TSharedPtr<FTextBlockElement> Left   = CreateTextBlock("AAAA", Font);
    TSharedPtr<FTextBlockElement> Center = CreateTextBlock("AAAA", Font);
    TSharedPtr<FTextBlockElement> Right  = CreateTextBlock("AAAA", Font);

    TSharedPtr<FVerticalBoxElement> Box = FVerticalBoxElement::Create();
    Box->AddSlot(Left).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Box->AddSlot(Center).SetHorizontalAlignment(EHorizontalAlignment::Center);
    Box->AddSlot(Right).SetHorizontalAlignment(EHorizontalAlignment::Right);

    Box->PrepareDesiredSize();
    Box->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(Left->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Left->GetContentRectangle().Width, 32);
    TEST_EXPECT_EQ(Center->GetContentRectangle().Position.X, (200 - 32) / 2);
    TEST_EXPECT_EQ(Right->GetContentRectangle().Position.X, 200 - 32);

    TEST_SECTION("Fill gives the child the whole slot");
    TSharedPtr<FTextBlockElement> Filled = CreateTextBlock("AAAA", Font);

    TSharedPtr<FVerticalBoxElement> FillBox = FVerticalBoxElement::Create();
    FillBox->AddSlot(Filled).SetFillCoefficient(1.0f);

    FillBox->PrepareDesiredSize();
    FillBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(Filled->GetContentRectangle().Width, 200);
    TEST_EXPECT_EQ(Filled->GetContentRectangle().Height, 100);

    TEST_SECTION("Top, center and bottom place a short child vertically");
    TSharedPtr<FTextBlockElement> Top    = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> Middle = CreateTextBlock("A", Font);
    TSharedPtr<FTextBlockElement> Bottom = CreateTextBlock("A", Font);

    TSharedPtr<FHorizontalBoxElement> VerticalBox = FHorizontalBoxElement::Create();
    VerticalBox->AddSlot(Top).SetVerticalAlignment(EVerticalAlignment::Top);
    VerticalBox->AddSlot(Middle).SetVerticalAlignment(EVerticalAlignment::Center);
    VerticalBox->AddSlot(Bottom).SetVerticalAlignment(EVerticalAlignment::Bottom);

    VerticalBox->PrepareDesiredSize();
    VerticalBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(Top->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(Middle->GetContentRectangle().Position.Y, (100 - 16) / 2);
    TEST_EXPECT_EQ(Bottom->GetContentRectangle().Position.Y, 100 - 16);

    TEST_SECTION("Padding shrinks the space the child is aligned in");
    TSharedPtr<FTextBlockElement> Padded = CreateTextBlock("AAAA", Font);

    TSharedPtr<FVerticalBoxElement> PaddedBox = FVerticalBoxElement::Create();
    PaddedBox->AddSlot(Padded).SetPadding(FMargin(10, 5)).SetHorizontalAlignment(EHorizontalAlignment::Left);

    PaddedBox->PrepareDesiredSize();
    PaddedBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(Padded->GetContentRectangle().Position.X, 10);
    TEST_EXPECT_EQ(Padded->GetContentRectangle().Position.Y, 5);

    TEST_END();
}

bool ScrollBoxClamping_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    // Ten lines of sixteen pixels each, shown through a hundred-pixel view
    TSharedPtr<FVerticalBoxElement> Content = FVerticalBoxElement::Create();
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Content->AddSlot(CreateTextBlock("Line", Font));
    }

    TSharedPtr<FScrollBoxElement> ScrollBox = FScrollBoxElement::Create();
    ScrollBox->SetContent(Content);

    ScrollBox->PrepareDesiredSize();
    ScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_SECTION("The scrollable range is the content overflow");
    TEST_EXPECT_EQ(ScrollBox->GetMaxScrollOffset(), 60);
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 0);
    TEST_EXPECT(!ScrollBox->IsScrolledToEnd());

    TEST_SECTION("An offset inside the range is kept");
    ScrollBox->SetScrollOffset(20);
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 20);

    TEST_SECTION("Offsets outside the range clamp to it");
    ScrollBox->SetScrollOffset(-50);
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 0);

    ScrollBox->SetScrollOffset(5000);
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 60);
    TEST_EXPECT(ScrollBox->IsScrolledToEnd());

    TEST_SECTION("The content is offset upward by the scroll amount");
    ScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));
    TEST_EXPECT_EQ(Content->GetContentRectangle().Position.Y, -60);

    TEST_SECTION("ScrollToEnd is applied on the next arrange");
    ScrollBox->SetScrollOffset(0);
    ScrollBox->ScrollToEnd();
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 0);

    ScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 60);

    TEST_SECTION("Content that fits leaves nothing to scroll");
    TSharedPtr<FVerticalBoxElement> ShortContent = FVerticalBoxElement::Create();
    ShortContent->AddSlot(CreateTextBlock("Line", Font));

    TSharedPtr<FScrollBoxElement> ShortScrollBox = FScrollBoxElement::Create();
    ShortScrollBox->SetContent(ShortContent);

    ShortScrollBox->PrepareDesiredSize();
    ShortScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_EXPECT_EQ(ShortScrollBox->GetMaxScrollOffset(), 0);
    TEST_EXPECT(ShortScrollBox->IsScrolledToEnd());

    TEST_END();
}

bool WindowLayoutOrigin_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    FWindowElement::FInitializer Initializer;
    Initializer.Title    = "Layout Origin";
    Initializer.Size     = IntVector2(1280, 720);
    Initializer.Position = IntVector2(340, 180);

    TSharedPtr<FWindowElement> Window = FWindowElement::Create(Initializer);

    TSharedPtr<FTextBlockElement>   ContentRow = CreateTextBlock("Content", Font);
    TSharedPtr<FVerticalBoxElement> Content    = FVerticalBoxElement::Create();
    Content->AddSlot(ContentRow);

    TSharedPtr<FTextBlockElement>   OverlayRow = CreateTextBlock("Overlay", Font);
    TSharedPtr<FVerticalBoxElement> Overlay    = FVerticalBoxElement::Create();
    Overlay->AddSlot(OverlayRow);

    Window->SetContent(Content);
    Window->SetOverlay(Overlay);

    FApplication::LayoutWindow(Window);

    TEST_SECTION("The window is laid out from the client origin, not from its place on the desktop");
    TEST_EXPECT_EQ(Window->GetPosition().X, 340);
    TEST_EXPECT_EQ(Window->GetPosition().Y, 180);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Width, 1280);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Height, 720);

    TEST_SECTION("Both subtrees start at the same origin, which is what the renderer projects from");
    TEST_EXPECT_EQ(Content->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Content->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(Overlay->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Overlay->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(OverlayRow->GetContentRectangle().Position.Y, 0);

    TEST_SECTION("Moving the window leaves the layout where it was");
    Window->OnWindowMoved(IntVector2(900, 40));
    FApplication::LayoutWindow(Window);

    TEST_EXPECT_EQ(Window->GetPosition().X, 900);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Position.X, 0);
    TEST_EXPECT_EQ(Window->GetContentRectangle().Position.Y, 0);
    TEST_EXPECT_EQ(OverlayRow->GetContentRectangle().Position.Y, 0);

    TEST_END();
}

bool WindowOverlayMeasure_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    FWindowElement::FInitializer Initializer;
    Initializer.Title = "Overlay Measure";
    Initializer.Size  = IntVector2(1280, 720);

    TSharedPtr<FWindowElement> Window = FWindowElement::Create(Initializer);
    TSharedPtr<FTextBlockElement> LogRow   = CreateTextBlock("Log", Font);
    TSharedPtr<FTextBlockElement> InputRow = CreateTextBlock("Input", Font);

    TSharedPtr<FVerticalBoxElement> Overlay = FVerticalBoxElement::Create();
    Overlay->AddSlot(LogRow).SetFillCoefficient(1.0f);
    Overlay->AddSlot(InputRow);

    Window->SetOverlay(Overlay);

    TEST_SECTION("The window reports both subtrees as children, so the measure pass reaches them");
    TArray<TSharedPtr<FVisualElement>> Children;
    Window->GetChildren(Children);
    TEST_EXPECT_EQ(Children.Size(), 1);

    Window->SetContent(FVerticalBoxElement::Create());
    Children.Clear();
    Window->GetChildren(Children);
    TEST_EXPECT_EQ(Children.Size(), 2);

    FApplication::LayoutWindow(Window);

    TEST_SECTION("An element in the overlay is measured rather than left at zero");
    TEST_EXPECT_EQ(InputRow->GetCachedDesiredSize().Y, 16);
    TEST_EXPECT_EQ(InputRow->GetCachedDesiredSize().X, 5 * 8);
    TEST_EXPECT_EQ(Overlay->GetCachedDesiredSize().Y, 32);

    TEST_SECTION("The auto slot is arranged at the height it asked for, above the bottom edge");
    TEST_EXPECT_EQ(InputRow->GetContentRectangle().Height, 16);
    TEST_EXPECT_EQ(InputRow->GetContentRectangle().Position.Y, 720 - 16);
    TEST_EXPECT_EQ(LogRow->GetContentRectangle().Height, 720 - 16);

    TEST_END();
}

bool ScrollBoxScrollIntoView_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FVerticalBoxElement> Content = FVerticalBoxElement::Create();
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Content->AddSlot(CreateTextBlock("Line", Font));
    }

    TSharedPtr<FScrollBoxElement> ScrollBox = FScrollBoxElement::Create();
    ScrollBox->SetContent(Content);

    ScrollBox->PrepareDesiredSize();
    ScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    TEST_SECTION("A target already in view does not move the offset");
    ScrollBox->SetScrollOffset(0);
    ScrollBox->ScrollIntoView(FRectangle(IntVector2(0, 32), 200, 16));
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 0);

    TEST_SECTION("A target below the view scrolls the minimum distance down");
    ScrollBox->ScrollIntoView(FRectangle(IntVector2(0, 112), 200, 16));
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 28);

    TEST_SECTION("A target above the view scrolls the minimum distance up");
    ScrollBox->ScrollIntoView(FRectangle(IntVector2(0, 16), 200, 16));
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 16);

    TEST_SECTION("Revealing the last row scrolls to the end");
    ScrollBox->ScrollIntoView(FRectangle(IntVector2(0, 144), 200, 16));
    TEST_EXPECT_EQ(ScrollBox->GetScrollOffset(), 60);
    TEST_EXPECT(ScrollBox->IsScrolledToEnd());

    TEST_END();
}
