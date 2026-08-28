#include <Core/Containers/SharedPtr.h>
#include <Core/Math/Math.h>
#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/EditableText.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>
#include <Application/Text/TextLayout.h>

#include "DrawCanvas.h"
#include "PlaygroundScene.h"
#include "ScenePanel.h"

static const CHAR* GWrapSample =
    "FTextLayout breaks a run wherever the wrap width says it must, preferring the space between two "
    "words and falling back to a mid-word cut only when one word is wider than the line it is on. "
    "Resize the window and this paragraph reflows: the runs are re-split every frame against the width "
    "the canvas was arranged into, so nothing here is measured once and cached against a stale width.";

static TSharedPtr<FVisualElement> MakeTextBlockRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    struct FSample
    {
        const CHAR*        Text;
        const FFloatColor& Color;
    };

    const FSample Samples[] =
    {
        { "Normal text",   Style.Colors.Text         },
        { "Disabled text", Style.Colors.TextDisabled },
        { "Accent text",   Style.Colors.Accent       },
    };

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Samples)); ++Index)
    {
        FTextBlock::FDesc Desc;
        Desc.Text            = Samples[Index].Text;
        Desc.Font            = (Index == 2) ? Fonts.Heading : Fonts.Body;
        Desc.ColorAndOpacity = Samples[Index].Color;

        Column->AddSlot(FTextBlock::Create(Desc))
            .SetPadding(FMargin(0, 1, 0, 1))
            .SetHorizontalAlignment(EHorizontalAlignment::Left);
    }

    FTextBlock::FDesc MonoDesc;
    MonoDesc.Text            = "Monospace 0123456789 iiiii mmmmm |||||";
    MonoDesc.Font            = Fonts.Monospace;
    MonoDesc.ColorAndOpacity = Style.Colors.Text;

    Column->AddSlot(FTextBlock::Create(MonoDesc))
        .SetPadding(FMargin(0, 6, 0, 0))
        .SetHorizontalAlignment(EHorizontalAlignment::Left);

    return Column;
}

static TSharedPtr<FVisualElement> MakeEditableTextRow(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    const CHAR* Initial[] =
    {
        "Click to place the caret, drag to select",
        "",
    };

    for (int32 Index = 0; Index < static_cast<int32>(ARRAY_COUNT(Initial)); ++Index)
    {
        FEditableText::FDesc EditDesc;
        EditDesc.Text     = Initial[Index];
        EditDesc.HintText = "type here";
        EditDesc.Font     = Fonts.Monospace;

        FBorder::FDesc FieldDesc;
        FieldDesc.BackgroundColor = Style.Colors.WindowBackground;
        FieldDesc.Padding         = FMargin(2);
        FieldDesc.CornerRadius    = Style.Metrics.CornerRadius;
        FieldDesc.MinHeight       = Style.Metrics.RowHeight;
        FieldDesc.Content         = FEditableText::Create(EditDesc);

        Column->AddSlot(FBorder::Create(FieldDesc)).SetPadding(FMargin(0, 2, 0, 2));
    }

    return Column;
}

static TSharedPtr<FVisualElement> MakeWrappingCanvas(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FTextLayout> Layout = MakeSharedPtr<FTextLayout>();
    Layout->AppendRun(FTextRun(GWrapSample, Fonts.Body.Get(), Style.Colors.Text));

    FDrawCanvas::FDesc CanvasDesc;
    CanvasDesc.DesiredSize  = IntVector2(0, 96);
    CanvasDesc.OnCanvasDraw = FOnCanvasDraw::CreateLambda(
        [Layout](const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId) -> int32
        {
            Layout->WrapToWidth(Geometry.Bounds.Width);
            return Layout->Draw(Geometry, CommandList, LayerId);
        });

    return FDrawCanvas::Create(CanvasDesc);
}

static TSharedPtr<FVisualElement> MakeRichRunCanvas(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FTextLayout> Layout = MakeSharedPtr<FTextLayout>();

    Layout->AppendRun(FTextRun("Runs carry a face and a color, ", Fonts.Body.Get(), Style.Colors.Text));
    Layout->AppendRun(FTextRun("so a heading face", Fonts.Heading.Get(), Style.Colors.Accent));
    Layout->AppendRun(FTextRun(" and a ", Fonts.Body.Get(), Style.Colors.Text));
    Layout->AppendRun(FTextRun("monospace one", Fonts.Monospace.Get(), Style.Colors.Text));
    Layout->AppendRun(FTextRun(" share a line and the taller of them sets its height.\n", Fonts.Body.Get(), Style.Colors.Text));

    FTextRun Selected("This run carries a selection tint", Fonts.Body.Get(), Style.Colors.Text);
    Selected.BackgroundTint = Style.Colors.TextSelectionBackground;

    Layout->AppendRun(Selected);
    Layout->AppendRun(FTextRun(" and ", Fonts.Body.Get(), Style.Colors.Text));

    FTextRun Highlighted("this one a search highlight", Fonts.Body.Get(), FFloatColor(0.06f, 0.06f, 0.06f, 1.0f));
    Highlighted.BackgroundTint = FFloatColor(0.95f, 0.78f, 0.24f, 1.0f);

    Layout->AppendRun(Highlighted);
    Layout->AppendRun(FTextRun(".\n\nA blank line keeps its height, and a trailing newline opens an empty line after it.\n", Fonts.Body.Get(), Style.Colors.TextDisabled));

    FDrawCanvas::FDesc CanvasDesc;
    CanvasDesc.DesiredSize  = IntVector2(0, 130);
    CanvasDesc.OnCanvasDraw = FOnCanvasDraw::CreateLambda(
        [Layout](const FDrawGeometry& Geometry, FDrawCommandList& CommandList, int32 LayerId) -> int32
        {
            Layout->WrapToWidth(Geometry.Bounds.Width);
            return Layout->Draw(Geometry, CommandList, LayerId);
        });

    return FDrawCanvas::Create(CanvasDesc);
}

FPlaygroundScene CreateTextScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Text blocks";
        Desc.Description = "One line per face and color, measured through the atlas the renderer uploads.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeTextBlockRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Editable text";
        Desc.Description = "Caret, selection, word motion and the hint shown by the empty field below.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeEditableTextRow(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Wrapping";
        Desc.Description = "Wrapped to the width it was arranged into, so resizing the window reflows it.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeWrappingCanvas(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Styled runs";
        Desc.Description = "Mixed faces on one line, newlines, blank lines, and the tints selection and search draw with.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeRichRunCanvas(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Text", MakeSceneColumn("Text", Fonts, Panels));
}
