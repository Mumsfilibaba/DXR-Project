#include "DrawTests.h"
#include "UISnapshot.h"

#include "TestCommon/TestHarness.h"
#include "TestCommon/TestMacros.h"

#include <Application/Console/ConsoleLogBuffer.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Draw/UIDrawData.h>
#include <Application/Text/FixedWidthFontFace.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/TextBlock.h>

static TSharedPtr<IFontFace> CreateTestFont()
{
    return MakeSharedPtr<FFixedWidthFontFace>(8, 16);
}

static TSharedPtr<FTextBlock> CreateTextBlock(const CHAR* Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;
    return FTextBlock::Create(Desc);
}

bool DrawCommandList_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    FDrawCommandList CommandList;

    TEST_SECTION("A fresh list is empty and balanced");
    TEST_EXPECT(CommandList.IsEmpty());
    TEST_EXPECT_EQ(CommandList.Size(), 0);
    TEST_EXPECT(CommandList.IsClipStackBalanced());

    TEST_SECTION("Commands come back in emission order with their layers");
    CommandList.AddBox(0, FRectangle(IntVector2(0, 0), 100, 50), FFloatColor::White);
    CommandList.AddText(1, FRectangle(IntVector2(4, 4), 90, 16), String("Hello"), Font.Get(), FFloatColor::White);
    CommandList.AddLine(2, FRectangle(IntVector2(4, 4), 1, 16), FFloatColor::White);

    TEST_EXPECT_EQ(CommandList.Size(), 3);
    TEST_EXPECT(!CommandList.IsEmpty());
    TEST_EXPECT(CommandList[0].Type == EDrawCommandType::Box);
    TEST_EXPECT(CommandList[1].Type == EDrawCommandType::Text);
    TEST_EXPECT(CommandList[2].Type == EDrawCommandType::Line);
    TEST_EXPECT_EQ(CommandList[0].LayerId, 0);
    TEST_EXPECT_EQ(CommandList[1].LayerId, 1);
    TEST_EXPECT_EQ(CommandList[2].LayerId, 2);

    TEST_SECTION("A text command keeps its string and face");
    TEST_EXPECT(CommandList[1].Text.Equals("Hello"));
    TEST_EXPECT(CommandList[1].Font == Font.Get());

    TEST_SECTION("Counting by type only counts that type");
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Text), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::ClipPush), 0);

    TEST_SECTION("Text lookup finds a match and reports a miss");
    TEST_EXPECT_EQ(CommandList.FindTextCommand("Hello"), 1);
    TEST_EXPECT_EQ(CommandList.FindTextCommand("Missing"), FDrawCommandList::InvalidIndex);

    TEST_SECTION("Reset empties the list and the clip state");
    CommandList.PushClip(0, FRectangle(IntVector2(0, 0), 10, 10));
    CommandList.Reset();

    TEST_EXPECT(CommandList.IsEmpty());
    TEST_EXPECT(CommandList.IsClipStackBalanced());

    TEST_END();
}

static int32 CountCoveredPixels(const FSnapshotImage& Image, const IntVector2& Start, const IntVector2& End)
{
    int32 Covered = 0;
    for (int32 Y = Start.Y; Y <= End.Y; ++Y)
    {
        for (int32 X = Start.X; X <= End.X; ++X)
        {
            if (Image.GetPixel(X, Y) != 0)
            {
                ++Covered;
            }
        }
    }

    return Covered;
}

static FSnapshotImage RasterizeCommands(const FDrawCommandList& CommandList, int32 Width, int32 Height)
{
    FUIDrawData DrawData;
    DrawData.BuildFromCommandList(CommandList);

    FSnapshotImage Image;
    Image.Initialize(Width, Height, 0);

    RasterizeDrawData(DrawData, Image);
    return Image;
}

bool HairlineCoverage_Test()
{
    TEST_BEGIN();

    constexpr float Hairline = 1.0f;

    TEST_SECTION("A horizontal hairline on an integer coordinate covers pixels");
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, Vector2(4.0f, 8.0f), Vector2(28.0f, 8.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 16);
        TEST_EXPECT(Image.IsValid());
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(4, 6), IntVector2(27, 9)) > 0);
    }

    TEST_SECTION("A vertical hairline on an integer coordinate covers pixels");
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, Vector2(8.0f, 4.0f), Vector2(8.0f, 28.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 16, 32);
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(6, 4), IntVector2(9, 27)) > 0);
    }

    TEST_SECTION("A closed axis-aligned hairline outline covers pixels on every side");
    {
        const Vector2 Corners[] =
        {
            Vector2(6.0f, 6.0f),
            Vector2(26.0f, 6.0f),
            Vector2(26.0f, 26.0f),
            Vector2(6.0f, 26.0f),
        };

        FDrawCommandList CommandList;
        CommandList.AddPolyline(0, MakeArrayView(Corners, 4), FFloatColor::White, Hairline, true);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 32);

        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(7, 5), IntVector2(25, 7)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(7, 25), IntVector2(25, 27)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(5, 7), IntVector2(7, 25)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(25, 7), IntVector2(27, 25)) > 0);
    }

    TEST_SECTION("A diagonal hairline still covers pixels");
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, Vector2(4.0f, 4.0f), Vector2(28.0f, 28.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 32);
        TEST_EXPECT(CountCoveredPixels(Image, IntVector2(4, 4), IntVector2(27, 27)) > 0);
    }

    TEST_END();
}

bool DrawClipNesting_Test()
{
    TEST_BEGIN();

    FDrawCommandList CommandList;

    TEST_SECTION("A nested region is intersected with the enclosing one");
    CommandList.PushClip(0, FRectangle(IntVector2(0, 0), 100, 100));
    TEST_EXPECT(CommandList.GetCurrentClipRectangle() == FRectangle(IntVector2(0, 0), 100, 100));

    CommandList.PushClip(1, FRectangle(IntVector2(50, 50), 100, 100));

    const FRectangle Nested = CommandList.GetCurrentClipRectangle();
    TEST_EXPECT_EQ(Nested.Position.X, 50);
    TEST_EXPECT_EQ(Nested.Position.Y, 50);
    TEST_EXPECT_EQ(Nested.Width, 50);
    TEST_EXPECT_EQ(Nested.Height, 50);

    TEST_SECTION("The recorded push carries the intersected region");
    TEST_EXPECT(CommandList[1].Bounds == Nested);

    TEST_SECTION("Popping restores the enclosing region");
    CommandList.PopClip(1);
    TEST_EXPECT(CommandList.GetCurrentClipRectangle() == FRectangle(IntVector2(0, 0), 100, 100));

    TEST_SECTION("An unclosed push leaves the list unbalanced");
    TEST_EXPECT(!CommandList.IsClipStackBalanced());

    CommandList.PopClip(0);
    TEST_EXPECT(CommandList.IsClipStackBalanced());
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::ClipPush), 2);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::ClipPop), 2);

    TEST_SECTION("An extra pop leaves the list unbalanced too");
    CommandList.PopClip(0);
    TEST_EXPECT(!CommandList.IsClipStackBalanced());

    TEST_SECTION("A scroll box wraps its child in exactly one push and one pop");
    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FVerticalBox> Content = FVerticalBox::Create();
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Content->AddSlot(CreateTextBlock("Line", Font));
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(Content);
    ScrollBox->PrepareDesiredSize();
    ScrollBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    FDrawCommandList ScrollCommandList;
    ScrollBox->OnDraw(FDrawGeometry(ScrollBox->GetContentRectangle(), 1.0f), ScrollCommandList, 0);

    TEST_EXPECT_EQ(ScrollCommandList.CountCommandsOfType(EDrawCommandType::ClipPush), 1);
    TEST_EXPECT_EQ(ScrollCommandList.CountCommandsOfType(EDrawCommandType::ClipPop), 1);
    TEST_EXPECT(ScrollCommandList.IsClipStackBalanced());
    TEST_EXPECT_EQ(ScrollCommandList.CountCommandsOfType(EDrawCommandType::Text), 10);

    TEST_END();
}

bool BorderDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TEST_SECTION("An opaque background emits one box beneath the child");
    FBorder::FDesc Desc;
    Desc.BackgroundColor = FFloatColor(0.1f, 0.2f, 0.3f, 1.0f);
    Desc.Padding         = FMargin(10, 5);
    Desc.Content         = CreateTextBlock("Border", Font);

    TSharedPtr<FBorder> Border = FBorder::Create(Desc);
    Border->PrepareDesiredSize();
    Border->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    FDrawCommandList CommandList;
    const int32      MaxLayerId = Border->OnDraw(FDrawGeometry(Border->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);
    TEST_EXPECT(CommandList[0].Type == EDrawCommandType::Box);
    TEST_EXPECT_EQ(CommandList[0].LayerId, 0);
    TEST_EXPECT_EQ(CommandList.FindTextCommand("Border"), 1);
    TEST_EXPECT(MaxLayerId > 0);

    TEST_SECTION("The desired size is the child plus the padding");
    TEST_EXPECT_EQ(Border->GetCachedDesiredSize().X, (6 * 8) + 20);
    TEST_EXPECT_EQ(Border->GetCachedDesiredSize().Y, 16 + 10);

    TEST_SECTION("A zero-alpha background emits no box at all");
    FBorder::FDesc TransparentDesc;
    TransparentDesc.BackgroundColor = FFloatColor(0.1f, 0.2f, 0.3f, 0.0f);
    TransparentDesc.Content         = CreateTextBlock("Border", Font);

    TSharedPtr<FBorder> Transparent = FBorder::Create(TransparentDesc);
    Transparent->PrepareDesiredSize();
    Transparent->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    FDrawCommandList TransparentCommandList;
    Transparent->OnDraw(FDrawGeometry(Transparent->GetContentRectangle(), 1.0f), TransparentCommandList, 0);

    TEST_EXPECT_EQ(TransparentCommandList.CountCommandsOfType(EDrawCommandType::Box), 0);
    TEST_EXPECT_EQ(TransparentCommandList.CountCommandsOfType(EDrawCommandType::Text), 1);

    TEST_END();
}

bool LogSeverityColors_Test()
{
    TEST_BEGIN();

    const FFloatColor InfoColor    = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Info);
    const FFloatColor WarningColor = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning);
    const FFloatColor ErrorColor   = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Error);

    TEST_SECTION("Info is white and every severity is opaque");
    TEST_EXPECT(InfoColor.R == 1.0f && InfoColor.G == 1.0f && InfoColor.B == 1.0f);
    TEST_EXPECT(InfoColor.A == 1.0f && WarningColor.A == 1.0f && ErrorColor.A == 1.0f);

    TEST_SECTION("Warning and error are the saturated primaries the ImGui console used");
    TEST_EXPECT(WarningColor.R == 1.0f && WarningColor.G == 1.0f && WarningColor.B == 0.0f);
    TEST_EXPECT(ErrorColor.R == 1.0f && ErrorColor.G == 0.0f && ErrorColor.B == 0.0f);

    TEST_END();
}

bool BoxLayerSequencing_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FVerticalBox> ScrollContent = FVerticalBox::Create();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        ScrollContent->AddSlot(CreateTextBlock("Log", Font));
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(ScrollContent);

    FBorder::FDesc InputDesc;
    InputDesc.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    InputDesc.Content         = CreateTextBlock("Input", Font);

    TSharedPtr<FVerticalBox> RootBox = FVerticalBox::Create();
    RootBox->AddSlot(ScrollBox).SetFillCoefficient(1.0f);
    RootBox->AddSlot(FBorder::Create(InputDesc)).SetVerticalAlignment(EVerticalAlignment::Bottom);

    RootBox->PrepareDesiredSize();
    RootBox->Tick(FRectangle(IntVector2(0, 0), 200, 100));

    FDrawCommandList CommandList;
    RootBox->OnDraw(FDrawGeometry(RootBox->GetContentRectangle(), 1.0f), CommandList, 0);

    int32 ClipPushLayerId = -1;
    int32 ClipPopLayerId  = -1;
    int32 BoxLayerId      = -1;

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        switch (Command.Type)
        {
            case EDrawCommandType::ClipPush:
            {
                ClipPushLayerId = Command.LayerId;
                break;
            }
            case EDrawCommandType::ClipPop:
            {
                ClipPopLayerId = Command.LayerId;
                break;
            }
            case EDrawCommandType::Box:
            {
                BoxLayerId = Command.LayerId;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    const int32 LogIndex   = CommandList.FindTextCommand("Log");
    const int32 InputIndex = CommandList.FindTextCommand("Input");

    TEST_SECTION("Both slots drew, inside one balanced region");
    TEST_EXPECT(CommandList.IsClipStackBalanced());
    TEST_EXPECT(LogIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(InputIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(ClipPushLayerId >= 0 && ClipPopLayerId >= 0 && BoxLayerId >= 0);

    if (LogIndex == FDrawCommandList::InvalidIndex || InputIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    TEST_SECTION("The scrolled lines fall inside the region, which is what clips them");
    TEST_EXPECT(CommandList[LogIndex].LayerId >= ClipPushLayerId);
    TEST_EXPECT(CommandList[LogIndex].LayerId <= ClipPopLayerId);

    TEST_SECTION("The row after it starts above the region, so the layer sort leaves it unclipped");
    TEST_EXPECT(BoxLayerId > ClipPopLayerId);
    TEST_EXPECT(CommandList[InputIndex].LayerId > ClipPopLayerId);

    TEST_END();
}
