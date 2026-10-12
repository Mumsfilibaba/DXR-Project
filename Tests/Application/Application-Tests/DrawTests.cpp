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

static TSharedPtr<FTextBlock> CreateTextBlock(const CHAR_T* Text, const TSharedPtr<IFontFace>& Font)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = Font;
    return FTextBlock::Create(Desc);
}

static int32 CountCoveredPixels(const FSnapshotImage& Image, const FIntVector2& Start, const FIntVector2& End)
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

bool DrawCommandList_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    FDrawCommandList CommandList;

    TEST_SECTION(TEXT("A fresh list is empty and balanced"));
    TEST_EXPECT(CommandList.IsEmpty());
    TEST_EXPECT_EQ(CommandList.Size(), 0);
    TEST_EXPECT(CommandList.IsClipStackBalanced());

    TEST_SECTION(TEXT("Commands come back in emission order with their layers"));
    CommandList.AddBox(0, FRectangle(FIntVector2(0, 0), 100, 50), FFloatColor::White);
    CommandList.AddText(1, FRectangle(FIntVector2(4, 4), 90, 16), FString(TEXT("Hello")), Font.Get(), FFloatColor::White);
    CommandList.AddLine(2, FRectangle(FIntVector2(4, 4), 1, 16), FFloatColor::White);

    TEST_EXPECT_EQ(CommandList.Size(), 3);
    TEST_EXPECT(!CommandList.IsEmpty());
    TEST_EXPECT(CommandList[0].Type == EDrawCommandType::Box);
    TEST_EXPECT(CommandList[1].Type == EDrawCommandType::Text);
    TEST_EXPECT(CommandList[2].Type == EDrawCommandType::Line);
    TEST_EXPECT_EQ(CommandList[0].LayerId, 0);
    TEST_EXPECT_EQ(CommandList[1].LayerId, 1);
    TEST_EXPECT_EQ(CommandList[2].LayerId, 2);

    TEST_SECTION(TEXT("A text command keeps its string and face"));
    TEST_EXPECT(CommandList.GetCommandText(CommandList[1]).Equals(TEXT("Hello")));
    TEST_EXPECT(CommandList[1].Font == Font.Get());

    TEST_SECTION(TEXT("Counting by type only counts that type"));
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);
    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Text), 1);

    TEST_SECTION(TEXT("Text lookup finds a match and reports a miss"));
    TEST_EXPECT_EQ(CommandList.FindTextCommand(TEXT("Hello")), 1);
    TEST_EXPECT_EQ(CommandList.FindTextCommand(TEXT("Missing")), FDrawCommandList::InvalidIndex);

    TEST_SECTION(TEXT("Reset empties the list and the clip state"));
    CommandList.PushClip(0, FRectangle(FIntVector2(0, 0), 10, 10));
    CommandList.Reset();

    TEST_EXPECT(CommandList.IsEmpty());
    TEST_EXPECT(CommandList.IsClipStackBalanced());

    TEST_END();
}

bool HairlineCoverage_Test()
{
    TEST_BEGIN();

    constexpr float Hairline = 1.0f;

    TEST_SECTION(TEXT("A horizontal hairline on an integer coordinate covers pixels"));
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, FVector2(4.0f, 8.0f), FVector2(28.0f, 8.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 16);
        TEST_EXPECT(Image.IsValid());
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(4, 6), FIntVector2(27, 9)) > 0);
    }

    TEST_SECTION(TEXT("A vertical hairline on an integer coordinate covers pixels"));
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, FVector2(8.0f, 4.0f), FVector2(8.0f, 28.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 16, 32);
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(6, 4), FIntVector2(9, 27)) > 0);
    }

    TEST_SECTION(TEXT("A closed axis-aligned hairline outline covers pixels on every side"));
    {
        const FVector2 Corners[] =
        {
            FVector2(6.0f, 6.0f),
            FVector2(26.0f, 6.0f),
            FVector2(26.0f, 26.0f),
            FVector2(6.0f, 26.0f),
        };

        FDrawCommandList CommandList;
        CommandList.AddPolyline(0, MakeArrayView(Corners, 4), FFloatColor::White, Hairline, true);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 32);

        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(7, 5), FIntVector2(25, 7)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(7, 25), FIntVector2(25, 27)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(5, 7), FIntVector2(7, 25)) > 0);
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(25, 7), FIntVector2(27, 25)) > 0);
    }

    TEST_SECTION(TEXT("A diagonal hairline still covers pixels"));
    {
        FDrawCommandList CommandList;
        CommandList.AddLine(0, FVector2(4.0f, 4.0f), FVector2(28.0f, 28.0f), FFloatColor::White, Hairline);

        const FSnapshotImage Image = RasterizeCommands(CommandList, 32, 32);
        TEST_EXPECT(CountCoveredPixels(Image, FIntVector2(4, 4), FIntVector2(27, 27)) > 0);
    }

    TEST_END();
}

bool DrawClipNesting_Test()
{
    TEST_BEGIN();

    FDrawCommandList CommandList;

    TEST_SECTION(TEXT("A nested region is intersected with the enclosing one"));
    CommandList.PushClip(0, FRectangle(FIntVector2(0, 0), 100, 100));
    TEST_EXPECT(CommandList.GetCurrentClipRectangle() == FRectangle(FIntVector2(0, 0), 100, 100));

    CommandList.PushClip(1, FRectangle(FIntVector2(50, 50), 100, 100));

    const FRectangle Nested = CommandList.GetCurrentClipRectangle();
    TEST_EXPECT_EQ(Nested.Position.X, 50);
    TEST_EXPECT_EQ(Nested.Position.Y, 50);
    TEST_EXPECT_EQ(Nested.Width, 50);
    TEST_EXPECT_EQ(Nested.Height, 50);

    TEST_SECTION(TEXT("A draw recorded inside the nested region stores that clip"));
    CommandList.AddBox(2, FRectangle(FIntVector2(60, 60), 10, 10), FFloatColor::White);
    TEST_EXPECT_EQ(CommandList.Size(), 1);
    TEST_EXPECT(CommandList[0].IsClipped());
    TEST_EXPECT(CommandList.GetCommandClipRectangle(CommandList[0]) == Nested);
    TEST_EXPECT_EQ(CommandList.Size(), 1);

    TEST_SECTION(TEXT("Popping restores the enclosing region"));
    CommandList.PopClip(1);
    TEST_EXPECT(CommandList.GetCurrentClipRectangle() == FRectangle(FIntVector2(0, 0), 100, 100));

    TEST_SECTION(TEXT("An unclosed push leaves the list unbalanced"));
    TEST_EXPECT(!CommandList.IsClipStackBalanced());

    CommandList.PopClip(0);
    TEST_EXPECT(CommandList.IsClipStackBalanced());
    TEST_EXPECT_EQ(CommandList.Size(), 1);

    TEST_SECTION(TEXT("An extra pop leaves the list unbalanced too"));
    CommandList.PopClip(0);
    TEST_EXPECT(!CommandList.IsClipStackBalanced());

    TEST_SECTION(TEXT("A scroll box clips its children without recording clip commands"));
    TSharedPtr<IFontFace> Font = CreateTestFont();

    TSharedPtr<FVerticalBox> Content = FVerticalBox::Create();
    for (int32 Index = 0; Index < 10; ++Index)
    {
        Content->AddSlot(CreateTextBlock(TEXT("Line"), Font));
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(Content);
    ScrollBox->PrepareDesiredSize();
    ScrollBox->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList ScrollCommandList;
    ScrollBox->OnDraw(FDrawGeometry(ScrollBox->GetContentRectangle(), 1.0f), ScrollCommandList, 0);

    TEST_EXPECT(ScrollCommandList.IsClipStackBalanced());
    TEST_EXPECT(ScrollCommandList.CountCommandsOfType(EDrawCommandType::Text) > 0);
    TEST_EXPECT(ScrollCommandList.CountCommandsOfType(EDrawCommandType::Text) < 10);

    bool bFoundClippedText = false;
    for (const FDrawCommand& Command : ScrollCommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Text && Command.IsClipped())
        {
            bFoundClippedText = true;
            break;
        }
    }
    TEST_EXPECT(bFoundClippedText);

    TEST_END();
}

bool BorderDraw_Test()
{
    TEST_BEGIN();

    TSharedPtr<IFontFace> Font = CreateTestFont();

    TEST_SECTION(TEXT("An opaque background emits one box beneath the child"));
    FBorder::FDesc Desc;
    Desc.BackgroundColor = FFloatColor(0.1f, 0.2f, 0.3f, 1.0f);
    Desc.Padding         = FMargin(10, 5);
    Desc.Content         = CreateTextBlock(TEXT("Border"), Font);

    TSharedPtr<FBorder> Border = FBorder::Create(Desc);
    Border->PrepareDesiredSize();
    Border->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList CommandList;
    const int32      MaxLayerId = Border->OnDraw(FDrawGeometry(Border->GetContentRectangle(), 1.0f), CommandList, 0);

    TEST_EXPECT_EQ(CommandList.CountCommandsOfType(EDrawCommandType::Box), 1);
    TEST_EXPECT(CommandList[0].Type == EDrawCommandType::Box);
    TEST_EXPECT_EQ(CommandList[0].LayerId, 0);
    TEST_EXPECT_EQ(CommandList.FindTextCommand(TEXT("Border")), 1);
    TEST_EXPECT(MaxLayerId > 0);

    TEST_SECTION(TEXT("The desired size is the child plus the padding"));
    TEST_EXPECT_EQ(Border->GetCachedDesiredSize().X, (6 * 8) + 20);
    TEST_EXPECT_EQ(Border->GetCachedDesiredSize().Y, 16 + 10);

    TEST_SECTION(TEXT("A zero-alpha background emits no box at all"));
    FBorder::FDesc TransparentDesc;
    TransparentDesc.BackgroundColor = FFloatColor(0.1f, 0.2f, 0.3f, 0.0f);
    TransparentDesc.Content         = CreateTextBlock(TEXT("Border"), Font);

    TSharedPtr<FBorder> Transparent = FBorder::Create(TransparentDesc);
    Transparent->PrepareDesiredSize();
    Transparent->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList TransparentCommandList;
    Transparent->OnDraw(FDrawGeometry(Transparent->GetContentRectangle(), 1.0f), TransparentCommandList, 0);

    TEST_EXPECT_EQ(TransparentCommandList.CountCommandsOfType(EDrawCommandType::Box), 0);
    TEST_EXPECT_EQ(TransparentCommandList.CountCommandsOfType(EDrawCommandType::Text), 1);

    TEST_SECTION(TEXT("A stroke goes under the child by default, so a child that fills the border hides it"));
    FBorder::FDesc StrokeDesc;
    StrokeDesc.BorderColor     = FFloatColor(0.8f, 0.1f, 0.1f, 1.0f);
    StrokeDesc.BorderThickness = 1.0f;
    StrokeDesc.Content         = CreateTextBlock(TEXT("Border"), Font);

    TSharedPtr<FBorder> UnderBorder = FBorder::Create(StrokeDesc);
    UnderBorder->PrepareDesiredSize();
    UnderBorder->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList UnderCommandList;
    UnderBorder->OnDraw(FDrawGeometry(UnderBorder->GetContentRectangle(), 1.0f), UnderCommandList, 0);

    TEST_EXPECT_EQ(UnderCommandList.CountCommandsOfType(EDrawCommandType::BoxOutline), 1);
    TEST_EXPECT(UnderCommandList[0].Type == EDrawCommandType::BoxOutline);

    TEST_SECTION(TEXT("Asked to draw over its child, the same stroke comes after it and no lower than its layer"));
    StrokeDesc.bDrawBorderOverContent = true;
    StrokeDesc.Content                = CreateTextBlock(TEXT("Border"), Font);

    TSharedPtr<FBorder> OverBorder = FBorder::Create(StrokeDesc);
    OverBorder->PrepareDesiredSize();
    OverBorder->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList OverCommandList;
    const int32      OverMaxLayerId = OverBorder->OnDraw(FDrawGeometry(OverBorder->GetContentRectangle(), 1.0f), OverCommandList, 0);

    TEST_EXPECT_EQ(OverCommandList.CountCommandsOfType(EDrawCommandType::BoxOutline), 1);

    const int32 TextIndex = OverCommandList.FindTextCommand(TEXT("Border"));
    TEST_EXPECT(TextIndex >= 0);

    const int32          StrokeIndex = OverCommandList.GetCommands().LastIndex();
    const FDrawCommand&  OverStroke  = OverCommandList[StrokeIndex];

    TEST_EXPECT(OverStroke.Type == EDrawCommandType::BoxOutline);
    TEST_EXPECT(StrokeIndex > TextIndex);
    TEST_EXPECT(OverStroke.LayerId >= OverCommandList[TextIndex].LayerId);
    TEST_EXPECT(OverMaxLayerId > OverStroke.LayerId);

    TEST_END();
}

bool LogSeverityColors_Test()
{
    TEST_BEGIN();

    const FFloatColor InfoColor    = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Info);
    const FFloatColor WarningColor = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Warning);
    const FFloatColor ErrorColor   = FConsoleLogBuffer::GetSeverityColor(ELogSeverity::Error);

    TEST_SECTION(TEXT("Info is white and every severity is opaque"));
    TEST_EXPECT(InfoColor.R == 1.0f && InfoColor.G == 1.0f && InfoColor.B == 1.0f);
    TEST_EXPECT(InfoColor.A == 1.0f && WarningColor.A == 1.0f && ErrorColor.A == 1.0f);

    TEST_SECTION(TEXT("Warning and error are the saturated primaries the ImGui console used"));
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
        ScrollContent->AddSlot(CreateTextBlock(TEXT("Log"), Font));
    }

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    ScrollBox->SetContent(ScrollContent);

    FBorder::FDesc InputDesc;
    InputDesc.BackgroundColor = FFloatColor(0.1f, 0.1f, 0.1f, 1.0f);
    InputDesc.Content         = CreateTextBlock(TEXT("Input"), Font);

    TSharedPtr<FVerticalBox> RootBox = FVerticalBox::Create();
    RootBox->AddSlot(ScrollBox).SetFillCoefficient(1.0f);
    RootBox->AddSlot(FBorder::Create(InputDesc)).SetVerticalAlignment(EVerticalAlignment::Bottom);

    RootBox->PrepareDesiredSize();
    RootBox->Arrange(FRectangle(FIntVector2(0, 0), 200, 100));

    FDrawCommandList CommandList;
    RootBox->OnDraw(FDrawGeometry(RootBox->GetContentRectangle(), 1.0f), CommandList, 0);

    int32 BoxLayerId = -1;

    for (const FDrawCommand& Command : CommandList.GetCommands())
    {
        if (Command.Type == EDrawCommandType::Box)
        {
            BoxLayerId = Command.LayerId;
        }
    }

    const int32 LogIndex   = CommandList.FindTextCommand(TEXT("Log"));
    const int32 InputIndex = CommandList.FindTextCommand(TEXT("Input"));

    TEST_SECTION(TEXT("Both slots drew, inside one balanced region"));
    TEST_EXPECT(CommandList.IsClipStackBalanced());
    TEST_EXPECT(LogIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(InputIndex != FDrawCommandList::InvalidIndex);
    TEST_EXPECT(BoxLayerId >= 0);

    if (LogIndex == FDrawCommandList::InvalidIndex || InputIndex == FDrawCommandList::InvalidIndex)
    {
        TEST_END();
    }

    TEST_SECTION(TEXT("The scrolled lines carry the clip, which is what clips them"));
    TEST_EXPECT(CommandList[LogIndex].IsClipped());

    TEST_SECTION(TEXT("The row after it is recorded outside the region, so the layer sort leaves it unclipped"));
    TEST_EXPECT(!CommandList[InputIndex].IsClipped());
    TEST_EXPECT(BoxLayerId > CommandList[LogIndex].LayerId);
    TEST_EXPECT(CommandList[InputIndex].LayerId > CommandList[LogIndex].LayerId);

    TEST_END();
}
