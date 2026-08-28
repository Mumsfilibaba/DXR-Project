#include <Core/Containers/SharedPtr.h>
#include <Application/Elements/Border.h>
#include <Application/Elements/Box.h>
#include <Application/Elements/Overlay.h>
#include <Application/Elements/Slider.h>
#include <Application/Elements/Spacer.h>
#include <Application/Elements/TextBlock.h>
#include <Application/Elements/ToolBar.h>
#include <Application/Graph/GraphCanvas.h>
#include <Application/Menus/Menu.h>
#include <Application/Menus/MenuItem.h>
#include <Application/Style/UIStyle.h>

#include "PlaygroundScene.h"
#include "ScenePanel.h"

// Tall enough to hold a graph a few columns wide without the view having to be panned first
constexpr int32 NODE_GRAPH_HEIGHT = 460;

// The read-only view is only there to be read, so it needs less room
constexpr int32 RENDER_GRAPH_HEIGHT = 300;

static TSharedPtr<FGraphModel>  GShaderModel;
static TSharedPtr<FGraphCanvas> GShaderCanvas;
static TSharedPtr<FTextBlock>   GShaderReadout;

static FFloatColor GetTypeTint(const String& TypeTag)
{
    if (TypeTag == "Float")
    {
        return FFloatColor(0.55f, 0.78f, 0.45f, 1.0f);
    }

    if (TypeTag == "Vector")
    {
        return FFloatColor(0.42f, 0.65f, 0.92f, 1.0f);
    }

    if (TypeTag == "Texture")
    {
        return FFloatColor(0.78f, 0.52f, 0.86f, 1.0f);
    }

    return FFloatColor(0.85f, 0.62f, 0.35f, 1.0f);
}

static FGraphPin MakePin(EGraphPinDirection Direction, const CHAR* Name, const CHAR* TypeTag)
{
    return FGraphPin(Direction, Name, TypeTag, GetTypeTint(TypeTag));
}

static FGraphNode MakeNode(const CHAR* Title, const Vector2& Position, const FFloatColor& TitleTint)
{
    FGraphNode Node;
    Node.Title     = Title;
    Node.Position  = Position;
    Node.TitleTint = TitleTint;

    return Node;
}

static TSharedPtr<FVisualElement> MakeValueBody(const FPlaygroundFonts& Fonts, float StartingValue)
{
    FTextBlock::FDesc ValueDesc;
    ValueDesc.Text            = String::Printf("%.2f", StartingValue);
    ValueDesc.Font            = Fonts.Monospace;
    ValueDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    TSharedPtr<FTextBlock> ValueText = FTextBlock::Create(ValueDesc);

    FSlider::FDesc SliderDesc;
    SliderDesc.SetRange(0.0f, 1.0f).SetValue(StartingValue);
    SliderDesc.OnValueChanged = FOnSliderValueChanged::CreateLambda([ValueText](float NewValue)
    {
        ValueText->SetText(String::Printf("%.2f", NewValue));
    });

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FSlider::Create(SliderDesc)).SetHorizontalAlignment(EHorizontalAlignment::Fill);
    Column->AddSlot(ValueText).SetPadding(FMargin(0, 4, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Center);
    return Column;
}

static void BuildShaderGraph(const FPlaygroundFonts& Fonts)
{
    GShaderModel->Clear();

    const FUIStyle& Style = FUIStyle::GetDefault();

    FGraphNode Texture = MakeNode("Base Color", Vector2(40.0f, 60.0f), Style.Colors.ControlNormal);
    Texture.Pins.Add(MakePin(EGraphPinDirection::Input, "UV", "Vector"));
    Texture.Pins.Add(MakePin(EGraphPinDirection::Output, "RGB", "Vector"));
    Texture.Pins.Add(MakePin(EGraphPinDirection::Output, "Alpha", "Float"));

    FGraphNode Coordinates = MakeNode("Texture Coordinate", Vector2(40.0f, 240.0f), Style.Colors.ControlNormal);
    Coordinates.Pins.Add(MakePin(EGraphPinDirection::Output, "UV", "Vector"));

    FGraphNode Roughness = MakeNode("Roughness", Vector2(40.0f, 350.0f), Style.Colors.ControlNormal);
    Roughness.Pins.Add(MakePin(EGraphPinDirection::Output, "Value", "Float"));
    Roughness.Content = MakeValueBody(Fonts, 0.35f);

    FGraphNode Multiply = MakeNode("Multiply", Vector2(320.0f, 120.0f), Style.Colors.ControlNormal);
    Multiply.Pins.Add(MakePin(EGraphPinDirection::Input, "A", "Vector"));
    Multiply.Pins.Add(MakePin(EGraphPinDirection::Input, "B", "Vector"));
    Multiply.Pins.Add(MakePin(EGraphPinDirection::Output, "Result", "Vector"));

    FGraphNode Output = MakeNode("Surface", Vector2(600.0f, 160.0f), Style.Colors.Accent);
    Output.Pins.Add(MakePin(EGraphPinDirection::Input, "Base Color", "Vector"));
    Output.Pins.Add(MakePin(EGraphPinDirection::Input, "Roughness", "Float"));
    Output.Pins.Add(MakePin(EGraphPinDirection::Input, "Normal", "Vector"));

    const int32 TextureId     = GShaderModel->AddNode(Texture);
    const int32 CoordinatesId = GShaderModel->AddNode(Coordinates);
    const int32 RoughnessId   = GShaderModel->AddNode(Roughness);
    const int32 MultiplyId    = GShaderModel->AddNode(Multiply);
    const int32 OutputId      = GShaderModel->AddNode(Output);

    GShaderModel->AddLink(GShaderModel->FindNode(CoordinatesId)->Pins[0].PinId, GShaderModel->FindNode(TextureId)->Pins[0].PinId);
    GShaderModel->AddLink(GShaderModel->FindNode(TextureId)->Pins[1].PinId, GShaderModel->FindNode(MultiplyId)->Pins[0].PinId);
    GShaderModel->AddLink(GShaderModel->FindNode(MultiplyId)->Pins[2].PinId, GShaderModel->FindNode(OutputId)->Pins[0].PinId);
    GShaderModel->AddLink(GShaderModel->FindNode(RoughnessId)->Pins[0].PinId, GShaderModel->FindNode(OutputId)->Pins[1].PinId);
}

static void RefreshShaderReadout()
{
    if (!GShaderCanvas || !GShaderReadout)
    {
        return;
    }

    const int32 NumSelected = GShaderCanvas->GetSelectedNodes().Size();

    String Description = String::Printf(
        "%d nodes, %d links, zoom %.2fx, ",
        GShaderModel->GetNodes().Size(),
        GShaderModel->GetLinks().Size(),
        GShaderCanvas->GetZoom());

    if (GShaderCanvas->GetSelectedLink() >= 0)
    {
        Description.Append("a link selected");
    }
    else if (NumSelected == 1)
    {
        const FGraphNode* Node = GShaderModel->FindNode(GShaderCanvas->GetSelectedNodes()[0]);
        Description.Append(Node ? String::Printf("%s selected", Node->Title.Data()) : String("nothing selected"));
    }
    else if (NumSelected > 1)
    {
        Description.Append(String::Printf("%d nodes selected", NumSelected));
    }
    else
    {
        Description.Append("nothing selected");
    }

    GShaderReadout->SetText(Description);
}

static void AddNodeAt(const CHAR* Title, const CHAR* TypeTag, const Vector2& GraphPosition)
{
    FGraphNode Node = MakeNode(Title, GraphPosition, FUIStyle::GetDefault().Colors.ControlNormal);
    Node.Pins.Add(MakePin(EGraphPinDirection::Input, "A", TypeTag));
    Node.Pins.Add(MakePin(EGraphPinDirection::Input, "B", TypeTag));
    Node.Pins.Add(MakePin(EGraphPinDirection::Output, "Result", TypeTag));

    GShaderModel->AddNode(Node);
    RefreshShaderReadout();
}

static TSharedPtr<FVisualElement> MakeContextMenu(const FPlaygroundFonts& Fonts, const Vector2& GraphPosition)
{
    struct FEntry
    {
        const CHAR* Title;
        const CHAR* TypeTag;
    };

    const FEntry Entries[] =
    {
        { "Add", "Vector" },
        { "Multiply", "Vector" },
        { "Lerp", "Vector" },
        { "Saturate", "Float" },
    };

    TSharedPtr<FMenu> Menu = FMenu::Create();
    for (const FEntry& Entry : Entries)
    {
        FMenuItem::FDesc Desc;
        Desc.SetLabel(Entry.Title).SetFont(Fonts.Body);
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([Entry, GraphPosition]()
        {
            AddNodeAt(Entry.Title, Entry.TypeTag, GraphPosition);
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    return Menu;
}

static TSharedPtr<FVisualElement> MakeShaderGraph(const FPlaygroundFonts& Fonts)
{
    GShaderModel = MakeSharedPtr<FGraphModel>();
    BuildShaderGraph(Fonts);

    FGraphCanvas::FDesc Desc;
    Desc.Font  = Fonts.Body;
    Desc.Model = GShaderModel;

    Desc.OnGetContextMenu = FOnGetGraphContextMenu::CreateLambda([Fonts](const Vector2& GraphPosition)
    {
        return MakeContextMenu(Fonts, GraphPosition);
    });

    Desc.OnSelectionChanged = FOnGraphSelectionChanged::CreateLambda([]()
    {
        RefreshShaderReadout();
    });

    GShaderCanvas = FGraphCanvas::Create(Desc);

    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    BorderDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    BorderDesc.Content         = GShaderCanvas;

    TSharedPtr<FOverlay> Sized = FOverlay::Create();
    Sized->AddSlot(FSpacer::CreateVertical(NODE_GRAPH_HEIGHT));
    Sized->AddSlot(FBorder::Create(BorderDesc));

    return Sized;
}

static TSharedPtr<FVisualElement> MakeGraphControls(const FPlaygroundFonts& Fonts)
{
    FTextBlock::FDesc ReadoutDesc;
    ReadoutDesc.Text            = "";
    ReadoutDesc.Font            = Fonts.Monospace;
    ReadoutDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.TextDisabled;

    GShaderReadout = FTextBlock::Create(ReadoutDesc);

    FToolBar::FDesc ToolBarDesc;
    ToolBarDesc.Font           = Fonts.Body;
    ToolBarDesc.bHasBackground = false;

    TSharedPtr<FToolBar> ToolBar = FToolBar::Create(ToolBarDesc);

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Auto layout").SetToolTipText("Rank the nodes and space the columns out"),
        FOnClicked::CreateLambda([]()
        {
            GShaderCanvas->AutoLayout();
            GShaderCanvas->FitToNodes();
            RefreshShaderReadout();
        }));

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Fit").SetToolTipText("Pan and zoom until every node is in view"),
        FOnClicked::CreateLambda([]()
        {
            GShaderCanvas->FitToNodes();
            RefreshShaderReadout();
        }));

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Reset view").SetToolTipText("Back to one-to-one, at the origin"),
        FOnClicked::CreateLambda([]()
        {
            GShaderCanvas->SetZoom(1.0f);
            GShaderCanvas->SetPan(Vector2(0.0f, 0.0f));
            RefreshShaderReadout();
        }));

    ToolBar->AddSeparator();

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Delete selection").SetToolTipText("The same thing the Delete key does"),
        FOnClicked::CreateLambda([]()
        {
            GShaderCanvas->DeleteSelection();
            RefreshShaderReadout();
        }));

    ToolBar->AddButton(
        FToolBarItemDesc().SetLabel("Rebuild the graph").SetToolTipText("Throw the edits away and start over"),
        FOnClicked::CreateLambda([Fonts]()
        {
            GShaderCanvas->ClearSelection();
            BuildShaderGraph(Fonts);
            RefreshShaderReadout();
        }));

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = FUIStyle::GetDefault().Colors.PanelBackground;
    FrameDesc.BorderColor     = FUIStyle::GetDefault().Colors.Border;
    FrameDesc.BorderThickness = FUIStyle::GetDefault().Metrics.BorderThickness;
    FrameDesc.CornerRadius    = FUIStyle::GetDefault().Metrics.CornerRadius;
    FrameDesc.Content         = ToolBar;

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(FBorder::Create(FrameDesc)).SetHorizontalAlignment(EHorizontalAlignment::Left);
    Column->AddSlot(GShaderReadout).SetPadding(FMargin(0, 8, 0, 0)).SetHorizontalAlignment(EHorizontalAlignment::Left);

    RefreshShaderReadout();
    return Column;
}

static TSharedPtr<FVisualElement> MakeRenderGraphView(const FPlaygroundFonts& Fonts)
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FGraphModel> Model = MakeSharedPtr<FGraphModel>();

    struct FPass
    {
        const CHAR* Title;
        const CHAR* Input;
        const CHAR* Output;
        bool        bIsCulled;
    };

    const FPass Passes[] =
    {
        { "Depth Prepass", nullptr, "Depth", false },
        { "Shadow Maps", nullptr, "Shadows", false },
        { "Base Pass", "Depth", "Scene Color", false },
        { "Screen Space AO", "Depth", "AO", true },
        { "Tone Map", "Scene Color", "Back Buffer", false },
    };

    TArray<int32> PassIds;
    for (const FPass& Pass : Passes)
    {
        FGraphNode Node = MakeNode(Pass.Title, Vector2(0.0f, 0.0f), Style.Colors.ControlNormal);
        Node.bIsMuted   = Pass.bIsCulled;

        if (Pass.Input)
        {
            Node.Pins.Add(MakePin(EGraphPinDirection::Input, Pass.Input, "Texture"));
        }

        Node.Pins.Add(MakePin(EGraphPinDirection::Output, Pass.Output, "Texture"));
        PassIds.Add(Model->AddNode(Node));
    }

    for (int32 Index = 0; Index < PassIds.Size(); ++Index)
    {
        if (!Passes[Index].Input)
        {
            continue;
        }

        for (int32 Other = 0; Other < Index; ++Other)
        {
            if (String(Passes[Other].Output) == Passes[Index].Input)
            {
                const FGraphNode* From = Model->FindNode(PassIds[Other]);
                const FGraphNode* To   = Model->FindNode(PassIds[Index]);

                Model->AddLink(From->Pins[From->Pins.Size() - 1].PinId, To->Pins[0].PinId);
                break;
            }
        }
    }

    FGraphCanvas::FDesc Desc;
    Desc.Font  = Fonts.Body;
    Desc.Model = Model;

    TSharedPtr<FGraphCanvas> Canvas = FGraphCanvas::Create(Desc);
    Canvas->AutoLayout();

    Model->SetReadOnly(true);

    FBorder::FDesc BorderDesc;
    BorderDesc.BorderColor     = Style.Colors.Border;
    BorderDesc.BorderThickness = Style.Metrics.BorderThickness;
    BorderDesc.Content         = Canvas;

    TSharedPtr<FOverlay> Sized = FOverlay::Create();
    Sized->AddSlot(FSpacer::CreateVertical(RENDER_GRAPH_HEIGHT));
    Sized->AddSlot(FBorder::Create(BorderDesc));

    return Sized;
}

FPlaygroundScene CreateNodeGraphScene(const FPlaygroundFonts& Fonts)
{
    TArray<TSharedPtr<FVisualElement>> Panels;

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "Node graph";
        Desc.Description = "Drag a node by its title, sweep a marquee over empty canvas, and pull a link out of one pin onto another. The wheel zooms about the cursor, the middle button pans, and the right button opens a menu that drops a node where it was pressed. A pin only takes a link from a pin of its own type.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeShaderGraph(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "The graph underneath";
        Desc.Description = "The same model, driven from outside the canvas. Laying out ranks the nodes by their longest path from a source, and the canvas notices the edit and rebuilds.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeGraphControls(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    {
        FScenePanel::FDesc Desc;
        Desc.Title       = "A read-only view";
        Desc.Description = "The shape a render graph debug view takes: passes laid out by rank, resources as pins, and a culled pass drawn dimmed. The model is read-only, so it pans, zooms and selects but nothing moves or connects.";
        Desc.Fonts       = Fonts;
        Desc.Content     = MakeRenderGraphView(Fonts);
        Panels.Add(FScenePanel::Create(Desc));
    }

    return FPlaygroundScene("Node graph", MakeSceneColumn("Node graph", Fonts, Panels));
}
