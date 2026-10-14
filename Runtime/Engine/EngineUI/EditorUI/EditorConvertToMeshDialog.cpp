#include "Engine/EngineUI/EditorUI/EditorConvertToMeshDialog.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EditorEngine.h"
#include "Engine/Engine.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/EngineContent.h"
#include "Engine/Assets/MeshAsset.h"
#include "Engine/World/World.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/PrimitiveShapeComponent.h"
#include "AssetCore/AssetFile.h"
#include "AssetCore/AssetMetadata.h"
#include "Application/Application.h"
#include "Application/Elements/Border.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/Overlay.h"
#include "Application/Elements/Spacer.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/PopupWindow.h"
#include "Core/Filesystem/File.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformFile.h"

static constexpr const CHAR_T* DefaultFolder = TEXT("/Project/Meshes");

static constexpr int32 DialogPadding = 16;
static constexpr int32 DialogWidth   = 420;
static constexpr int32 LabelWidth    = 64;
static constexpr int32 FieldPadding  = 6;
static constexpr int32 RowGap        = 8;
static constexpr int32 SectionGap    = 12;
static constexpr int32 ButtonWidth   = 100;
static constexpr int32 ButtonSpacing = 12;

struct FConvertDialogState
{
    FEditorEngine*            EditorEngine = nullptr;
    FActor*                   Actor        = nullptr;
    FPrimitiveShapeComponent* Component    = nullptr;
    TSharedPtr<FWindow>       PopupWindow;
    TSharedPtr<FEditableText> FolderField;
    TSharedPtr<FEditableText> NameField;
    TSharedPtr<FTextBlock>    ErrorText;
};

static bool IsValidName(const FString& Name)
{
    if (Name.IsEmpty() || Name.EndsWith(TEXT(".")) || Name.EndsWith(TEXT(" ")))
    {
        return false;
    }

    for (const CHAR_T Character : Name)
    {
        if (Character < 32 || FString(TEXT("\\/:*?\"<>|")).Contains(Character))
        {
            return false;
        }
    }

    return true;
}

static FString MakeDefaultName(const FActor* Actor)
{
    FString Name;
    for (const CHAR_T Character : Actor->GetName())
    {
        Name += (Character == TEXT(' ') || Character == TEXT('(') || Character == TEXT(')')) ? TEXT('_') : Character;
    }

    return Name;
}

static bool IsInWorld(const TSharedPtr<FConvertDialogState>& State)
{
    FWorld* World = State->EditorEngine ? State->EditorEngine->GetWorld() : nullptr;
    return World && World->GetActors().Contains(State->Actor) && State->Actor->GetComponentOfType<FPrimitiveShapeComponent>() == State->Component;
}

static void CloseDialog(const TSharedPtr<FConvertDialogState>& State)
{
    if (const TSharedPtr<FWindow> PopupWindow = State->PopupWindow)
    {
        State->PopupWindow.Reset();
        Popups::Close(PopupWindow);
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

static TSharedPtr<FVisualElement> CreateFieldRow(const FString& Label, const FString& Text, TSharedPtr<FEditableText>& OutField)
{
    const FInputFrameStyle FrameStyle = FEditorStyle::GetInputFrameStyle();

    FEditableText::FDesc FieldDesc;
    FieldDesc.Text            = Text;
    FieldDesc.Font            = FEditorStyle::GetFonts().Body;
    FieldDesc.ForegroundColor = FrameStyle.Text;
    FieldDesc.HintColor       = FrameStyle.HintNormal;
    FieldDesc.TextCursorColor = FrameStyle.Text;
    FieldDesc.SelectionColor  = FrameStyle.Selection;
    FieldDesc.Padding         = FMargin(FieldPadding, FieldPadding / 2);

    OutField = FEditableText::Create(FieldDesc);

    FBorder::FDesc FrameDesc;
    FrameDesc.BackgroundColor = FrameStyle.Fill;
    FrameDesc.BorderColor     = FrameStyle.BorderNormal;
    FrameDesc.BorderThickness = FrameStyle.BorderThickness;
    FrameDesc.CornerRadius    = FCornerRadii(FrameStyle.CornerRadius);
    FrameDesc.Content         = OutField;

    TSharedPtr<FOverlay> LabelCell = FOverlay::Create();
    LabelCell->AddSlot(FSpacer::CreateHorizontal(LabelWidth));
    LabelCell->AddSlot(CreateText(Label, FEditorStyle::GetFonts().Body, FUIStyle::GetDefault().Colors.Text)).SetVerticalAlignment(EVerticalAlignment::Center);

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(LabelCell).SetVerticalAlignment(EVerticalAlignment::Center);
    Row->AddSlot(FBorder::Create(FrameDesc)).SetFillCoefficient(1.0f).SetVerticalAlignment(EVerticalAlignment::Center);
    return Row;
}

bool FEditorConvertToMeshDialog::Convert(FPrimitiveShapeComponent* Component, const FString& InFolder, const FString& Name, FString& OutError)
{
    FActor* Actor = Component ? Component->GetActorOwner() : nullptr;
    if (!Actor)
    {
        OutError = TEXT("The shape isn't on an actor");
        return false;
    }

    if (!IsValidName(Name))
    {
        OutError = FString::Printf(TEXT("'%s' isn't a valid asset name"), *Name);
        return false;
    }

    const FString Folder = InFolder.EndsWith(TEXT("/")) ? InFolder.SubString(0, InFolder.Length() - 1) : InFolder;

    FAssetManager& AssetManager = FAssetManager::Get();

    FString Directory;
    if (!AssetManager.GetAssetRegistry().GetMounts().ToDiskPath(Folder, Directory))
    {
        OutError = FString::Printf(TEXT("'%s' isn't a folder under /Project/ or another mount"), *Folder);
        return false;
    }

    const FString VirtualPath = Folder + TEXT("/") + Name + FAssetFile::FileExtension;
    const FString Filename    = File::CombinePath(Directory, Name + FAssetFile::FileExtension);
    if (FPlatformFile::IsFile(*Filename))
    {
        OutError = FString::Printf(TEXT("'%s' exists already"), *VirtualPath);
        return false;
    }

    FMeshAssetData MeshAsset;
    MeshAsset.Mesh      = PrimitiveShapes::BuildMeshData(Component->GetShape());
    MeshAsset.Mesh.Name = Name;

    if (MeshAsset.Mesh.SubMeshes.IsEmpty())
    {
        FSubMesh& SubMesh = MeshAsset.Mesh.SubMeshes.Emplace();
        SubMesh.VertexCount = static_cast<uint32>(MeshAsset.Mesh.Vertices.Size());
        SubMesh.IndexCount  = static_cast<uint32>(MeshAsset.Mesh.Indices.Size());
    }

    for (FSubMesh& SubMesh : MeshAsset.Mesh.SubMeshes)
    {
        SubMesh.MaterialIndex = 0;
    }

    FMeshMaterialSlot& Slot = MeshAsset.MaterialSlots.Emplace();
    Slot.Name = TEXT("Default");
    if (FEngine::Get() && Component->GetMaterial(0) == FEngine::Get()->BaseMaterial)
    {
        Slot.Material = TAssetRef<FMaterial>(EngineContent::BaseMaterial, TEXT("/Engine/Materials/BaseMaterial.dxrmaterial"));
    }

    TArray<uint8> Payload;
    if (!MeshAssets::Write(MeshAsset, Payload, OutError))
    {
        return false;
    }

    FAssetMetadata Metadata;
    Metadata.Name         = Name;
    Metadata.Factory      = TEXT("PrimitiveShape");
    Metadata.Dependencies = MeshAssets::GetDependencies(MeshAsset);

    const FGuid Guid = FGuid::Create();

    FAssetFileWriter Writer(Guid, EAssetType::Mesh);
    Writer.SetMetadata(AssetMetadata::ToJson(Metadata));
    Writer.SetChunk(AssetChunk::Data, ::Move(Payload));

    if (!File::CreateDirectoryTree(Directory) || !Writer.SaveToFile(Filename, OutError))
    {
        OutError = OutError.IsEmpty() ? FString::Printf(TEXT("Could not write '%s'"), *Filename) : OutError;
        return false;
    }

    AssetManager.GetAssetRegistry().Rescan();

    const TSharedPtr<FMesh> Mesh = AssetManager.LoadAsset<FMesh>(Guid);
    if (!Mesh)
    {
        OutError = FString::Printf(TEXT("'%s' was written but couldn't be loaded"), *VirtualPath);
        return false;
    }

    FStaticMeshComponent* NewComponent = NewObject<FStaticMeshComponent>();
    NewComponent->SetMesh(Mesh);

    for (int32 Index = 0; Index < NewComponent->GetNumMaterials(); ++Index)
    {
        TSharedPtr<FMaterial> Material = Component->GetMaterial(Index) ? Component->GetMaterial(Index) : Component->GetMaterial(0);
        if (Material)
        {
            NewComponent->SetMaterial(Material, Index);
        }
    }

    Actor->RemoveComponent(Component);
    Actor->AddComponent(NewComponent);
    return true;
}

bool FEditorConvertToMeshDialog::Open(const TSharedPtr<FVisualElement>& AnchorElement, FEditorEngine* EditorEngine, FPrimitiveShapeComponent* Component)
{
    if (!AnchorElement || !FApplication::IsInitialized() || !Component || !Component->GetActorOwner())
    {
        return false;
    }

    TSharedPtr<FWindow> ParentWindow = FApplication::Get().FindWindow(AnchorElement);
    if (!ParentWindow)
    {
        return false;
    }

    const FUIStyle& Style = FUIStyle::GetDefault();

    TSharedPtr<FConvertDialogState> State = MakeSharedPtr<FConvertDialogState>();
    State->EditorEngine = EditorEngine;
    State->Actor        = Component->GetActorOwner();
    State->Component    = Component;
    State->ErrorText    = CreateText(FString(), FEditorStyle::GetFonts().Body, FFloatColor(0.95f, 0.45f, 0.40f, 1.0f));

    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();
    Column->AddSlot(CreateText(TEXT("Convert to Mesh Asset"), FEditorStyle::GetFonts().BodyBold, Style.Colors.Text));
    Column->AddSlot(FSpacer::CreateVertical(SectionGap));
    Column->AddSlot(CreateFieldRow(TEXT("Folder"), DefaultFolder, State->FolderField));
    Column->AddSlot(FSpacer::CreateVertical(RowGap));
    Column->AddSlot(CreateFieldRow(TEXT("Name"), MakeDefaultName(State->Actor), State->NameField));
    Column->AddSlot(FSpacer::CreateVertical(RowGap));
    Column->AddSlot(State->ErrorText);

    TSharedPtr<FHorizontalBox> ButtonRow = FHorizontalBox::Create();
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(0)).SetFillCoefficient(1.0f);
    ButtonRow->AddSlot(CreateButton(TEXT("Convert"), FOnClicked::CreateLambda([State]()
    {
        if (!IsInWorld(State))
        {
            CloseDialog(State);
            return;
        }

        FString Error;
        if (Convert(State->Component, State->FolderField->GetText(), State->NameField->GetText(), Error))
        {
            CloseDialog(State);
            State->EditorEngine->SetSelectedActor(State->Actor);
        }
        else
        {
            State->ErrorText->SetText(Error);
        }
    })));
    ButtonRow->AddSlot(FSpacer::CreateHorizontal(ButtonSpacing));
    ButtonRow->AddSlot(CreateButton(TEXT("Cancel"), FOnClicked::CreateLambda([State]()
    {
        CloseDialog(State);
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

    State->PopupWindow = Popups::Open(ParentWindow, Popups::ClampToWorkArea(FRectangle(DialogPosition, DialogSize.X, DialogSize.Y)), Frame);
    if (State->PopupWindow)
    {
        FApplication::Get().SetFocusElement(State->NameField);
    }

    return State->PopupWindow != nullptr;
}
