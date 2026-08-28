#include <Application/Draw/DrawCommandList.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/ScrollBox.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Style/UIStyle.h>

#include "DrawCanvas.h"
#include "NavButton.h"
#include "PlaygroundShell.h"

TSharedPtr<FPlaygroundShell> FPlaygroundShell::Create(const FPlaygroundFonts& Fonts, const TArray<FPlaygroundScene>& InScenes)
{
    TSharedPtr<FPlaygroundShell> NewShell = MakeSharedPtr<FPlaygroundShell>();
    NewShell->Initialize(Fonts, InScenes);
    return NewShell;
}

FPlaygroundShell::FPlaygroundShell()
    : FCompoundElement()
    , Scenes()
    , NavButtons()
    , ContentHost(nullptr)
    , SelectedScene(-1)
{
}

FPlaygroundShell::~FPlaygroundShell()
{
}

void FPlaygroundShell::Initialize(const FPlaygroundFonts& Fonts, const TArray<FPlaygroundScene>& InScenes)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    Scenes = InScenes;

    TSharedPtr<FVerticalBox> SidebarBox = FVerticalBox::Create();

    FDrawCanvas::FDesc WidthAnchorDesc;
    WidthAnchorDesc.DesiredSize = IntVector2(SidebarWidth, 0);
    SidebarBox->AddSlot(FDrawCanvas::Create(WidthAnchorDesc));

    FTextBlock::FDesc TitleDesc;
    TitleDesc.Text            = "UI Playground";
    TitleDesc.Font            = Fonts.Heading;
    TitleDesc.ColorAndOpacity = Style.Colors.Text;

    SidebarBox->AddSlot(FTextBlock::Create(TitleDesc))
        .SetPadding(FMargin(12, 6, 12, 14))
        .SetHorizontalAlignment(EHorizontalAlignment::Left);

    NavButtons.Reserve(Scenes.Size());
    for (int32 SceneIndex = 0; SceneIndex < Scenes.Size(); ++SceneIndex)
    {
        FNavButton::FDesc ButtonDesc;
        ButtonDesc.Label = Scenes[SceneIndex].Name;
        ButtonDesc.Font  = Fonts.Body;
        ButtonDesc.Index = SceneIndex;

        TSharedPtr<FNavButton> Button = FNavButton::Create(ButtonDesc);
        Button->SetOnClicked(FOnNavButtonClicked::CreateRaw(this, &FPlaygroundShell::SelectScene));

        SidebarBox->AddSlot(Button)
            .SetPadding(FMargin(8, 1, 8, 1));

        NavButtons.Add(Button);
    }

    FBorder::FDesc SidebarDesc;
    SidebarDesc.BackgroundColor = Style.Colors.PanelBackground;
    SidebarDesc.Padding         = FMargin(0, 8, 0, 8);
    SidebarDesc.Content         = SidebarBox;

    ContentHost = FScrollBox::Create();

    FBorder::FDesc ContentDesc;
    ContentDesc.BackgroundColor = Style.Colors.WindowBackground;
    ContentDesc.Padding         = FMargin(16);
    ContentDesc.Content         = ContentHost;

    TSharedPtr<FHorizontalBox> RootBox = FHorizontalBox::Create();
    RootBox->AddSlot(FBorder::Create(SidebarDesc));
    RootBox->AddSlot(FBorder::Create(ContentDesc)).SetFillCoefficient(1.0f);

    SetContent(RootBox);
    SelectScene(0);
}

int32 FPlaygroundShell::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();
    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.WindowBackground);

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId + 1);
}

void FPlaygroundShell::SelectScene(int32 SceneIndex)
{
    if (SceneIndex < 0 || SceneIndex >= Scenes.Size() || SceneIndex == SelectedScene)
    {
        return;
    }

    SelectedScene = SceneIndex;

    for (int32 ButtonIndex = 0; ButtonIndex < NavButtons.Size(); ++ButtonIndex)
    {
        NavButtons[ButtonIndex]->SetSelected(ButtonIndex == SceneIndex);
    }

    if (ContentHost)
    {
        ContentHost->SetContent(Scenes[SceneIndex].Content);
    }
}
