#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "ScenePanel.h"

TSharedPtr<FScenePanel> FScenePanel::Create(const FDesc& Desc)
{
    TSharedPtr<FScenePanel> NewPanel = MakeSharedPtr<FScenePanel>();
    NewPanel->Initialize(Desc);
    return NewPanel;
}

FScenePanel::FScenePanel()
    : FCompoundElement()
{
}

FScenePanel::~FScenePanel()
{
}

void FScenePanel::Initialize(const FDesc& Desc)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    FTextBlock::FDesc TitleDesc;
    TitleDesc.Text            = Desc.Title;
    TitleDesc.Font            = Desc.Fonts.Heading;
    TitleDesc.ColorAndOpacity = Style.Colors.Text;

    Column->AddSlot(FTextBlock::Create(TitleDesc))
        .SetHorizontalAlignment(EHorizontalAlignment::Left);

    if (!Desc.Description.IsEmpty())
    {
        FTextBlock::FDesc DescriptionDesc;
        DescriptionDesc.Text            = Desc.Description;
        DescriptionDesc.Font            = Desc.Fonts.Body;
        DescriptionDesc.ColorAndOpacity = Style.Colors.TextDisabled;

        Column->AddSlot(FTextBlock::Create(DescriptionDesc))
            .SetPadding(FMargin(0, 2, 0, 0))
            .SetHorizontalAlignment(EHorizontalAlignment::Left);
    }

    if (Desc.Content)
    {
        Column->AddSlot(Desc.Content).SetPadding(FMargin(0, 10, 0, 0));
    }

    SetPadding(FMargin(14, 12, 14, 14));
    SetContent(Column);
}

int32 FScenePanel::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style  = FUIStyle::GetDefault();
    const FCornerRadii Radii  = Style.Metrics.CornerRadius * 2.0f;
    const FRectangle&  Bounds = AllottedGeometry.Bounds;

    OutCommandList.AddBox(LayerId, Bounds, Style.Colors.PanelBackground, Radii);
    OutCommandList.AddBoxOutline(LayerId, Bounds, Style.Colors.Border, Style.Metrics.BorderThickness, Radii);

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
}

TSharedPtr<FVisualElement> MakeSceneColumn(
    const String&                             Title,
    const FPlaygroundFonts&                   Fonts,
    const TArray<TSharedPtr<FVisualElement>>& Panels)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    FTextBlock::FDesc TitleDesc;
    TitleDesc.Text            = Title;
    TitleDesc.Font            = Fonts.Heading;
    TitleDesc.ColorAndOpacity = Style.Colors.Accent;

    Column->AddSlot(FTextBlock::Create(TitleDesc))
        .SetPadding(FMargin(2, 0, 2, 12))
        .SetHorizontalAlignment(EHorizontalAlignment::Left);

    for (const TSharedPtr<FVisualElement>& Panel : Panels)
    {
        Column->AddSlot(Panel).SetPadding(FMargin(0, 0, 0, 12));
    }

    return Column;
}
