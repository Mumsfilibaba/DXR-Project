#include "Engine/EngineUI/EditorUI/EditorPrimitiveShapeDialog.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/EditorEngine.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/PrimitiveShapeComponent.h"
#include "Application/Application.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/NumericEntry.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/Spinner.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/PopupWindow.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Misc/IniFile.h"
#include "Core/Misc/Paths.h"

static constexpr const CHAR_T* SettingsSection = TEXT("PrimitiveShapes");

static constexpr int32 DialogPadding      = 16;
static constexpr int32 DialogWidth        = 340;
static constexpr int32 LabelWidth         = 120;
static constexpr int32 RowGap             = 6;
static constexpr int32 SectionGap         = 12;
static constexpr int32 ButtonWidth        = 100;
static constexpr int32 ButtonSpacing      = 12;

struct FShapeDialogState
{
    FEditorEngine*            EditorEngine = nullptr;
    FActor*                   Actor        = nullptr;
    FPrimitiveShapeComponent* Component    = nullptr;
    FVector3                  Location;
    FDelegateHandle           MeshChangedHandle;
    TSharedPtr<FWindow>       PopupWindow;
    TSharedPtr<FTextBlock>    Counts;
    TSharedPtr<FSpinner>      Spinner;
};

static FString GetSettingsFilename()
{
    return File::CombinePath(Paths::GetProjectUserDir(), TEXT("EditorSettings.ini"));
}

static FString GetSettingsKey(const FPrimitiveShapeDesc& Desc, int32 ParameterIndex)
{
    FString Key = FString(Desc.Name) + TEXT(".");
    for (const CHAR_T* Character = Desc.Parameters[ParameterIndex].Name; *Character; ++Character)
    {
        if (*Character != TEXT(' '))
        {
            Key += *Character;
        }
    }

    return Key;
}

static bool IsInWorld(const TSharedPtr<FShapeDialogState>& State)
{
    FWorld* World = State->EditorEngine ? State->EditorEngine->GetWorld() : nullptr;
    return World && World->GetActors().Contains(State->Actor);
}

static void UpdateCounts(const TSharedPtr<FShapeDialogState>& State)
{
    if (!IsInWorld(State))
    {
        return;
    }

    State->Spinner->SetSpinning(State->Component->IsBuilding());

    const TSharedPtr<FMesh> Mesh = State->Component->GetMesh();
    if (State->Component->IsBuilding())
    {
        State->Counts->SetText(TEXT("Building..."));
    }
    else if (Mesh)
    {
        State->Counts->SetText(FString::Printf(TEXT("%d vertices, %d triangles"), Mesh->GetVertexCount(), Mesh->GetIndexCount() / 3));
    }
}

static void CloseDialog(const TSharedPtr<FShapeDialogState>& State, bool bKeep)
{
    if (!State->PopupWindow)
    {
        return;
    }

    const TSharedPtr<FWindow> PopupWindow = State->PopupWindow;
    State->PopupWindow.Reset();
    Popups::Close(PopupWindow);

    if (!IsInWorld(State))
    {
        return;
    }

    State->Component->GetOnMeshChanged().Unbind(State->MeshChangedHandle);

    if (bKeep)
    {
        FEditorPrimitiveShapeDialog::SaveRemembered(State->Component->GetShape());
    }
    else
    {
        State->EditorEngine->ClearSelection();
        State->EditorEngine->RequestDeleteActors({ State->Actor });
    }
}

static TSharedPtr<FTextBlock> CreateText(const FString& Text, const TSharedPtr<IFontFace>& Font, const FFloatColor& Color)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = Text;
    Desc.Font            = Font;
    Desc.ColorAndOpacity = Color;
    return FTextBlock::Create(Desc);
}

static TSharedPtr<FButton> CreateButton(const FString& Label, const FOnClicked& OnClicked)
{
    TSharedPtr<FOverlay> Content = FOverlay::Create();
    Content->AddSlot(FSpacer::CreateHorizontal(ButtonWidth));
    Content->AddSlot(CreateText(Label, FEditorStyle::GetFonts().Body, FUIStyle::GetDefault().Colors.Text))
        .SetHorizontalAlignment(EHorizontalAlignment::Center)
        .SetVerticalAlignment(EVerticalAlignment::Center);

    FButton::FDesc Desc;
    Desc.Font      = FEditorStyle::GetFonts().Body;
    Desc.MinHeight = FEditorStyle::ButtonHeight;
    Desc.Content   = Content;
    Desc.OnClicked = OnClicked;
    return FButton::Create(Desc);
}

static TSharedPtr<FVisualElement> CreateParameterEditor(const TSharedPtr<FShapeDialogState>& State, int32 ParameterIndex)
{
    const FPrimitiveShape  Shape     = State->Component->GetShape();
    const FShapeParameter& Parameter = PrimitiveShapes::GetDesc(Shape.Type).Parameters[ParameterIndex];

    const auto ApplyValue = [State, ParameterIndex](float Value)
    {
        if (!IsInWorld(State))
        {
            return;
        }

        FPrimitiveShape NewShape = State->Component->GetShape();
        NewShape.Values[ParameterIndex] = Value;
        State->Component->SetShape(NewShape);
        UpdateCounts(State);
    };

    if (Parameter.Type == EShapeParameterType::Integer)
    {
        TNumericEntry<int32>::FDesc Desc;
        Desc.Value          = static_cast<int32>(Shape.Values[ParameterIndex]);
        Desc.MinValue       = static_cast<int32>(Parameter.Min);
        Desc.MaxValue       = static_cast<int32>(Parameter.Max);
        Desc.SliderMinValue = static_cast<int32>(Parameter.Min);
        Desc.SliderMaxValue = static_cast<int32>(Parameter.SliderMax);
        Desc.Font           = FEditorStyle::GetFonts().Body;
        Desc.bShowLabel     = false;
        Desc.bShowFillTrack = true;
        Desc.TextAlignment  = EHorizontalAlignment::Center;
        Desc.OnValueChanged = TDelegate<void(int32)>::CreateLambda([ApplyValue](int32 Value)
        {
            ApplyValue(static_cast<float>(Value));
        });

        return TNumericEntry<int32>::Create(Desc);
    }

    TNumericEntry<float>::FDesc Desc;
    Desc.Value          = Shape.Values[ParameterIndex];
    Desc.MinValue       = Parameter.Min;
    Desc.MaxValue       = Parameter.Max;
    Desc.SliderMinValue = Parameter.Min;
    Desc.SliderMaxValue = Parameter.SliderMax;
    Desc.Step           = 0.05f;
    Desc.Precision      = 2;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.bShowLabel     = false;
    Desc.bShowFillTrack = true;
    Desc.TextAlignment  = EHorizontalAlignment::Center;
    Desc.Suffix         = Parameter.Unit ? FString(TEXT(" ")) + Parameter.Unit : FString();
    Desc.OnValueChanged = TDelegate<void(float)>::CreateLambda(ApplyValue);

    return TNumericEntry<float>::Create(Desc);
}

static TSharedPtr<FWindow> FindMainWindow()
{
    if (!FApplication::IsInitialized())
    {
        return nullptr;
    }

    TSharedPtr<FWindow> Window = FApplication::Get().GetFocusWindow();
    if (!Window && !FApplication::Get().GetWindows().IsEmpty())
    {
        Window = FApplication::Get().GetWindows()[0];
    }

    while (Window && Window->GetParentWindow())
    {
        Window = Window->GetParentWindow();
    }

    return Window;
}

FPrimitiveShape FEditorPrimitiveShapeDialog::LoadRemembered(EPrimitiveShape Type)
{
    FPrimitiveShape Shape = PrimitiveShapes::MakeDefault(Type);

    FIniFile Settings;
    if (!Settings.LoadFromFile(GetSettingsFilename()))
    {
        return Shape;
    }

    const FPrimitiveShapeDesc& Desc = PrimitiveShapes::GetDesc(Type);
    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        float Value = 0.0f;
        if (Settings.GetFloat(SettingsSection, *GetSettingsKey(Desc, Index), Value))
        {
            Shape.Values[Index] = Value;
        }
    }

    return PrimitiveShapes::Clamp(Shape);
}

void FEditorPrimitiveShapeDialog::SaveRemembered(const FPrimitiveShape& Shape)
{
    const FString Filename = GetSettingsFilename();

    FIniFile Settings;
    if (!Settings.LoadFromFile(Filename))
    {
        Settings.Filename = Filename;
    }

    const FPrimitiveShapeDesc& Desc = PrimitiveShapes::GetDesc(Shape.Type);
    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        Settings.SetOrAddFloat(SettingsSection, *GetSettingsKey(Desc, Index), Shape.Values[Index]);
    }

    File::CreateDirectoryTree(File::GetDirectoryOf(Filename));
    Settings.WriteToFile();
}

FActor* FEditorPrimitiveShapeDialog::Place(FEditorEngine* EditorEngine, EPrimitiveShape Type, const FVector3& Location)
{
    FWorld* World = EditorEngine ? EditorEngine->GetWorld() : nullptr;
    FActor* Actor = EditorActorFactory::SpawnPrimitive(World, LoadRemembered(Type), Location);
    if (!Actor)
    {
        return nullptr;
    }

    EditorEngine->SetSelectedActor(Actor);

    const bool          bShiftDown   = FApplication::IsInitialized() && FApplication::Get().GetModifierKeyState().IsShiftDown();
    TSharedPtr<FWindow> ParentWindow = FindMainWindow();
    if (bShiftDown || !ParentWindow)
    {
        return Actor;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FShapeDialogState> State = MakeSharedPtr<FShapeDialogState>();
    State->EditorEngine = EditorEngine;
    State->Actor        = Actor;
    State->Component    = Actor->GetComponentOfType<FPrimitiveShapeComponent>();
    State->Location     = Location;
    State->Counts       = CreateText(FString(), FEditorStyle::GetFonts().Body, Style.Colors.TextDisabled);

    FSpinner::FDesc SpinnerDesc;
    State->Spinner = FSpinner::Create(SpinnerDesc);

    if (!State->Component)
    {
        return Actor;
    }

    State->MeshChangedHandle = State->Component->GetOnMeshChanged().AddLambda([State]()
    {
        if (IsInWorld(State))
        {
            EditorActorFactory::RestOnGround(State->Actor, State->Location);
            UpdateCounts(State);
        }
    });

    const FPrimitiveShapeDesc& Desc = PrimitiveShapes::GetDesc(Type);

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(CreateText(FString::Printf(TEXT("Add %s"), Desc.Name), FEditorStyle::GetFonts().BodyBold, Style.Colors.Text));
    Column->AddSlot(FSpacer::CreateVertical(SectionGap));

    for (int32 Index = 0; Index < Desc.NumParameters; ++Index)
    {
        const FShapeParameter& Parameter = Desc.Parameters[Index];

        TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();


        TSharedPtr<FOverlay> LabelCell = FOverlay::Create();
        LabelCell->AddSlot(FSpacer::CreateHorizontal(LabelWidth));
        LabelCell->AddSlot(CreateText(Parameter.Name, FEditorStyle::GetFonts().Body, Style.Colors.Text)).SetVerticalAlignment(EVerticalAlignment::Center);

        Row->AddSlot(LabelCell).SetVerticalAlignment(EVerticalAlignment::Center);
        Row->AddSlot(CreateParameterEditor(State, Index)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);

        if (Index > 0)
        {
            Column->AddSlot(FSpacer::CreateVertical(RowGap));
        }

        Column->AddSlot(Row);
    }

    TSharedPtr<FHorizontalBox> CountsRow = FHorizontalBox::Create();
    CountsRow->AddSlot(State->Counts).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);
    CountsRow->AddSlot(State->Spinner).SetVerticalAlignment(EVerticalAlignment::Center);

    Column->AddSlot(FSpacer::CreateVertical(SectionGap));
    Column->AddSlot(CountsRow);

    TSharedPtr<FHorizontalBox> ButtonRow = FHorizontalBox::Create();
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
    ButtonRow->AddSlot(CreateButton(TEXT("Place"), FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State, true);
    })));
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(ButtonSpacing));
    ButtonRow->AddSlot(CreateButton(TEXT("Cancel"), FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State, false);
    })));

    Column->AddSlot(FSpacer::CreateVertical(SectionGap));
    Column->AddSlot(ButtonRow);

    FBorder::FDesc BorderDesc;
    BorderDesc.BackgroundColor = Style.Panel.Fill;
    BorderDesc.BorderColor     = Style.Panel.Border;
    BorderDesc.BorderThickness = Style.Panel.BorderThickness;
    BorderDesc.CornerRadius    = FCornerRadii(Style.Panel.CornerRadius);
    BorderDesc.Padding         = FMargin(DialogPadding, DialogPadding, DialogPadding, DialogPadding);
    BorderDesc.Content         = Column;

    TSharedPtr<FBorder> Frame = FBorder::Create(BorderDesc);
    Frame->PrepareDesiredSize();

    const FIntVector2 DesiredSize = Frame->GetCachedDesiredSize();
    const FIntVector2 DialogSize(Math::Max(DesiredSize.X, DialogWidth), Math::Max(DesiredSize.Y, 1));
    const FIntVector2 ParentPosition = ParentWindow->GetPosition();
    const FIntVector2 ParentSize     = ParentWindow->GetSize();
    const FIntVector2 DialogPosition(ParentPosition.X + (ParentSize.X - DialogSize.X) / 2, ParentPosition.Y + (ParentSize.Y - DialogSize.Y) / 2);

    UpdateCounts(State);

    State->PopupWindow = Popups::Open(ParentWindow, Popups::ClampToWorkArea(FRectangle(DialogPosition, DialogSize.X, DialogSize.Y)), Frame);
    return Actor;
}
