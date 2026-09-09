#include "Engine/EngineUI/EditorUI/Panels/EditorPropertiesPanel.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EditorEngine.h"
#include "Engine/World/Actors/Actor.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/Components/DirectionalLightComponent.h"
#include "Engine/World/Components/LightComponent.h"
#include "Engine/World/Components/LightProbeComponent.h"
#include "Engine/World/Components/PointLightComponent.h"
#include "Engine/World/Components/StaticMeshComponent.h"
#include "Engine/Resources/Material.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/ColorBlock.h"
#include "Application/Elements/ColorPicker.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/SeparatorText.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Elements/TexturePreview.h"
#include "Application/Input/Keys.h"
#include "Application/Menus/ComboBox.h"
#include "Application/Menus/MenuAnchor.h"
#include "Application/Style/UIStyle.h"

static const CHAR* GMaterialTextureSlotNames[EMaterialTextureSlot::Count] =
{
    "Base Color",
    "Normal",
    "Height",
    "Mask A",
    "Mask B",
    "Mask C",
    "Mask D",
};

static const CHAR* GNormalMapAxisLabels[2] = { "-Y (DirectX)", "+Y (OpenGL)" };
static const CHAR* GDegreeSuffix           = "\xC2\xB0";

constexpr float REVERT_EPSILON = 0.00005f;

constexpr float PROPERTIES_LABEL_FRACTION     = 0.4f;
constexpr int32 PROPERTIES_LABEL_COLUMN_WIDTH = 128;

constexpr int32 MATERIAL_TEXTURE_PREVIEW_SIZE = 48;
constexpr int32 MATERIAL_TEXTURE_ZOOM_SIZE    = 256;
constexpr int32 MATERIAL_TEXTURE_ROW_HEIGHT   = MATERIAL_TEXTURE_PREVIEW_SIZE + 8;

constexpr int32 UNIFORM_LOCK_SIZE = 16;

constexpr int32 COLOR_ROW_SPACING = 4;
constexpr float COLOR_ROW_STEP    = 0.01f;

static String FormatVector(const Vector3& Value)
{
    return String::Printf("%.3f, %.3f, %.3f", Value.X, Value.Y, Value.Z);
}

class FUniformScaleToggle final : public FVisualElement
{
public:
    static TSharedPtr<FUniformScaleToggle> Create()
    {
        return MakeSharedPtr<FUniformScaleToggle>();
    }

    FUniformScaleToggle()
        : FVisualElement()
        , bIsLocked(false)
    {
    }

    ~FUniformScaleToggle() = default;

    virtual IntVector2 ComputeDesiredSize() const override final
    {
        return IntVector2(UNIFORM_LOCK_SIZE, UNIFORM_LOCK_SIZE);
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final
    {
        const FUIBrush& Icon = bIsLocked ? FEditorIcons::Locked : FEditorIcons::Unlocked;
        if (Icon.IsValid())
        {
            OutCommandList.AddImage(LayerId, AllottedGeometry.Bounds, Icon, FUIStyle::GetDefault().Colors.Text);
        }

        return LayerId;
    }

    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override final
    {
        if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
        {
            return FEventResponse::Unhandled();
        }

        bIsLocked = !bIsLocked;
        OnToggled.ExecuteIfBound(bIsLocked);

        return FEventResponse::Handled();
    }

    /** @brief Fired every time the lock is clicked, with the state it landed in. */
    TDelegate<void(bool /*bIsLocked*/)> OnToggled;

private:
    bool bIsLocked;
};

FEditorPropertiesPanel::FEditorPropertiesPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "Properties", "Properties")
    , ScrollBox(nullptr)
    , Column(nullptr)
    , LightDirectionFields()
    , VectorRows()
    , ColorRows()
    , BuiltForActor(nullptr)
    , SelectedMaterialIndex(0)
    , bRebuildRequested(false)
{
}

FEditorPropertiesPanel::~FEditorPropertiesPanel()
{
}

bool FEditorPropertiesPanel::Initialize()
{
    Column = FVerticalBox::Create();

    ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    Content = ScrollBox;

    RebuildContent();
    return true;
}

void FEditorPropertiesPanel::Release()
{
    ScrollBox.Reset();
    Column.Reset();
    VectorRows.Clear();
    ColorRows.Clear();

    for (TSharedPtr<TNumericEntry<float>>& Field : LightDirectionFields)
    {
        Field.Reset();
    }

    BuiltForActor         = nullptr;
    SelectedMaterialIndex = 0;
    bRebuildRequested     = false;

    FEditorPanel::Release();
}

void FEditorPropertiesPanel::Tick(float /*DeltaTime*/)
{
    FActor* SelectedActor = EditorEngine->GetSelectedActor();
    if (SelectedActor != BuiltForActor || bRebuildRequested)
    {
        RebuildContent();
        return;
    }

    RefreshValues();
}

void FEditorPropertiesPanel::OnActorRemoved(FActor* Actor)
{
    if (Actor == BuiltForActor)
    {
        BuiltForActor = nullptr;
        RebuildContent();
    }
}

void FEditorPropertiesPanel::RebuildContent()
{
    Column->ClearSlots();
    VectorRows.Clear();
    ColorRows.Clear();

    for (TSharedPtr<TNumericEntry<float>>& Field : LightDirectionFields)
    {
        Field.Reset();
    }

    bRebuildRequested = false;

    FActor* Actor = EditorEngine->GetSelectedActor();
    if (Actor != BuiltForActor)
    {
        SelectedMaterialIndex = 0;
    }

    BuiltForActor = Actor;

    if (!Actor)
    {
        Column->AddSlot(CreateDisabledTextRow("No selection")).SetPadding(FMargin(8, 8, 8, 8));
        return;
    }

    Column->AddSlot(CreateSectionLabel("Actor"));

    BuildTransformSection(Column, Actor);

    if (FActor* ParentActor = Actor->GetParentActor())
    {
        BuildAttachmentSection(Column, ParentActor);
    }

    if (FStaticMeshComponent* Component = Actor->GetComponentOfType<FStaticMeshComponent>())
    {
        BuildStaticMeshSection(Column, Component);
    }

    if (FDirectionalLightComponent* Component = Actor->GetComponentOfType<FDirectionalLightComponent>())
    {
        BuildDirectionalLightSection(Column, Component);
    }
    else if (FLightComponent* Component = Actor->GetComponentOfType<FLightComponent>())
    {
        BuildPointLightSection(Column, Component);
    }

    if (FCameraComponent* Component = Actor->GetComponentOfType<FCameraComponent>())
    {
        BuildCameraSection(Column, Component);
    }

    if (FLightProbeComponent* Component = Actor->GetComponentOfType<FLightProbeComponent>())
    {
        BuildLightProbeSection(Column, Component);
    }
}

void FEditorPropertiesPanel::RequestRebuild()
{
    bRebuildRequested = true;
}

void FEditorPropertiesPanel::BuildAttachmentSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* ParentActor)
{
    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();

    const String& ParentName = ParentActor->GetName();
    Row->AddSlot(CreateTextRow(ParentName.IsEmpty() ? String("Actor") : ParentName)).SetFillCoefficient(1.0f);

    FButton::FDesc DetachDesc;
    DetachDesc.SetText("Detach").SetFont(FEditorStyle::GetFonts().Body);
    DetachDesc.OnClicked = FOnClicked::CreateLambda([this]()
    {
        if (FActor* Target = EditorEngine->GetSelectedActor())
        {
            Target->DetachFromParent(EAttachmentRule::KeepWorld);
            RequestRebuild();
        }
    });

    Row->AddSlot(FButton::Create(DetachDesc)).SetPadding(FMargin(8, 0, 0, 0));

    TSharedPtr<FPropertyTable> Table = CreateTable();
    Table->AddRow("Parent", Row);

    AddSection(InColumn, "Attachment", Table);
}

void FEditorPropertiesPanel::BuildTransformSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* Actor)
{
    TSharedPtr<FPropertyTable> Table = CreateTable();
    Table->AddRow("Name", CreateTextRow(Actor->GetName()));
    Table->AddRow("Type", CreateTextRow(Actor->GetTypeLabel()));

    const FActorTransform& Transform = Actor->GetTransform();

    const Vector3 TranslationDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Location", Transform.GetTranslation(), 0.1f,
        FOnVectorChanged::CreateLambda([this](const Vector3& Value)
        {
            if (FActor* Target = EditorEngine->GetSelectedActor())
            {
                Target->GetTransform().SetTranslation(Value);
            }
        }), false, &TranslationDefault,
        FOnVectorRead::CreateLambda([this]() -> Vector3
        {
            FActor* Target = EditorEngine->GetSelectedActor();
            return Target ? Target->GetTransform().GetTranslation() : Vector3(0.0f, 0.0f, 0.0f);
        }));

    FCameraComponent* Camera = Actor->GetComponentOfType<FCameraComponent>();

    const Vector3 RotationDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Rotation", Transform.GetRotation(), 0.5f,
        FOnVectorChanged::CreateLambda([this, Camera](const Vector3& Value)
        {
            if (Camera)
            {
                Camera->SetRotation(Value);
            }
            else if (FActor* Target = EditorEngine->GetSelectedActor())
            {
                Target->GetTransform().SetRotation(Value);
            }
        }), true, &RotationDefault,
        FOnVectorRead::CreateLambda([this]() -> Vector3
        {
            FActor* Target = EditorEngine->GetSelectedActor();
            return Target ? Target->GetTransform().GetRotation() : Vector3(0.0f, 0.0f, 0.0f);
        }));

    const Vector3 ScaleDefault = Vector3(1.0f, 1.0f, 1.0f);
    AddVectorRow(Table, "Scale", Transform.GetScale(), 0.01f,
        FOnVectorChanged::CreateLambda([this](const Vector3& Value)
        {
            if (FActor* Target = EditorEngine->GetSelectedActor())
            {
                Target->GetTransform().SetScale(Value);
            }
        }), false, &ScaleDefault,
        FOnVectorRead::CreateLambda([this]() -> Vector3
        {
            FActor* Target = EditorEngine->GetSelectedActor();
            return Target ? Target->GetTransform().GetScale() : Vector3(1.0f, 1.0f, 1.0f);
        }), true);

    AddSection(InColumn, Actor->GetParentActor() ? "Transform (Relative)" : "Transform", Table);
}

void FEditorPropertiesPanel::BuildStaticMeshSection(const TSharedPtr<FVerticalBox>& InColumn, FStaticMeshComponent* Component)
{
    InColumn->AddSlot(CreateSectionLabel("StaticMesh"));

    TSharedPtr<FPropertyTable> MaterialTable = CreateTable();
    AddMaterialSlotRows(MaterialTable, Component);

    TSharedPtr<FMaterial> Material = Component->GetMaterial(SelectedMaterialIndex);
    if (!Material)
    {
        AddSection(InColumn, "Material", MaterialTable);
        return;
    }

    FMaterial* MaterialPtr = Material.Get();

    AddMaterialRows(MaterialTable, MaterialPtr);
    AddSection(InColumn, "Material", MaterialTable);

    TSharedPtr<FPropertyTable> TextureTable = CreateTable();
    AddMaterialTextureRows(TextureTable, MaterialPtr);
    AddSection(InColumn, "Textures", TextureTable);

    TSharedPtr<FPropertyTable> FlagTable = CreateTable();
    AddMaterialFlagRows(FlagTable, MaterialPtr);
    AddSection(InColumn, "Material Flags", FlagTable);

    if (MaterialPtr->HasHeightMap())
    {
        TSharedPtr<FPropertyTable> ParallaxTable = CreateTable();
        AddParallaxRows(ParallaxTable, MaterialPtr);
        AddSection(InColumn, "Parallax", ParallaxTable);
    }
}

void FEditorPropertiesPanel::AddMaterialSlotRows(const TSharedPtr<FPropertyTable>& Table, FStaticMeshComponent* Component)
{
    const int32 NumMaterials = Component->GetNumMaterials();
    SelectedMaterialIndex = Math::Clamp<int32>(SelectedMaterialIndex, 0, Math::Max<int32>(NumMaterials - 1, 0));

    Table->AddRow("Material slots", CreateTextRow(String::Printf("%d", NumMaterials)));

    if (NumMaterials <= 1)
    {
        return;
    }

    TArray<String> Options;
    Options.Reserve(NumMaterials);

    for (int32 Index = 0; Index < NumMaterials; ++Index)
    {
        TSharedPtr<FMaterial> Entry = Component->GetMaterial(Index);
        if (Entry && !Entry->GetName().IsEmpty())
        {
            Options.Emplace(String::Printf("%d: %s", Index, *Entry->GetName()));
        }
        else
        {
            Options.Emplace(String::Printf("%d: <unnamed>", Index));
        }
    }

    Table->AddRow("Material", CreateComboEditor(Options, SelectedMaterialIndex,
        TDelegate<void(int32)>::CreateLambda([this](int32 NewIndex)
        {
            SelectedMaterialIndex = NewIndex;
            RequestRebuild();
        })));
}

void FEditorPropertiesPanel::AddMaterialRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

    const FFloatColor AlbedoDefault = FFloatColor::White;
    AddColorRow(Table, "Albedo", MaterialInfo.Albedo, &AlbedoDefault,
        FOnColorChanged::CreateLambda([Material](const FFloatColor& Value)
        {
            Material->SetAlbedo(Value);
        }));

    AddFloatRow(Table, "Roughness", MaterialInfo.Roughness, 0.01f, 1.0f, 0.01f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetRoughness(Value);
        }));

    AddFloatRow(Table, "Metallic", MaterialInfo.Metallic, 0.01f, 1.0f, 0.01f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetMetallic(Value);
        }));

    AddFloatRow(Table, "Ambient occlusion", MaterialInfo.AmbientOcclusion, 0.01f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetAmbientOcclusion(Value);
        }));

    AddFloatRow(Table, "Opacity", MaterialInfo.Opacity, 0.0f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetOpacity(Value);
        }));

    AddFloatRow(Table, "Index of refraction", MaterialInfo.IndexOfRefraction, 1.0f, 3.0f, 0.01f, 1.5f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetIndexOfRefraction(Value);
        }));

    AddFloatRow(Table, "Refraction strength", MaterialInfo.RefractionStrength, 0.0f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetRefractionStrength(Value);
        }));
}

void FEditorPropertiesPanel::AddMaterialTextureRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    for (uint32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
    {
        const FRHITextureRef& Texture = Material->GetTexture(EMaterialTextureSlot::Type(Slot));
        if (!Texture)
        {
            continue;
        }

        FTexturePreview::FDesc PreviewDesc;
        PreviewDesc.Brush       = FUIBrush(Texture.Get());
        PreviewDesc.Font        = FEditorStyle::GetFonts().Body;
        PreviewDesc.EmptyText   = "None";
        PreviewDesc.PreviewSize = MATERIAL_TEXTURE_PREVIEW_SIZE;
        PreviewDesc.ZoomSize    = MATERIAL_TEXTURE_ZOOM_SIZE;

        const IntVector3& Extent = Texture->GetDesc().Extent;

        FPropertyRow& Row  = Table->AddRow(GMaterialTextureSlotNames[Slot], FTexturePreview::Create(PreviewDesc));
        Row.HeightOverride = MATERIAL_TEXTURE_ROW_HEIGHT;
        Row.ToolTipText    = String::Printf("%d x %d", Extent.X, Extent.Y);
    }
}

void FEditorPropertiesPanel::AddMaterialFlagRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    const EMaterialFlags Flags             = Material->GetMaterialInfo().MaterialFlags;
    const bool           bHasNormalTexture = Material->GetTexture(EMaterialTextureSlot::Normal).IsValid();
    const bool           bHasHeightTexture = Material->GetTexture(EMaterialTextureSlot::Height).IsValid();

    AddBoolRow(Table, "Normal mapping", IsEnumFlagSet(Flags, EMaterialFlags::EnableNormalMapping),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableNormalMapping(bValue);
            RequestRebuild();
        }), nullptr, bHasNormalTexture);

    if (IsEnumFlagSet(Flags, EMaterialFlags::EnableNormalMapping) && bHasNormalTexture)
    {
        TArray<String> AxisOptions;
        AxisOptions.Reserve(2);
        AxisOptions.Emplace(GNormalMapAxisLabels[0]);
        AxisOptions.Emplace(GNormalMapAxisLabels[1]);

        Table->AddRow("Normal map axis", CreateComboEditor(AxisOptions, Material->IsNormalMapPositiveY() ? 1 : 0,
            TDelegate<void(int32)>::CreateLambda([Material](int32 NewIndex)
            {
                Material->SetNormalMapPositiveY(NewIndex == 1);
            }))).IndentLevel = 1;
    }

    AddBoolRow(Table, "Alpha mask", IsEnumFlagSet(Flags, EMaterialFlags::EnableAlpha),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableAlphaMask(bValue);
        }), nullptr, Material->IsRouteFed(EMaterialScalar::Opacity) && !Material->IsTranslucent());

    AddBoolRow(Table, "Double sided", IsEnumFlagSet(Flags, EMaterialFlags::DoubleSided),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableDoubleSided(bValue);
        }));

    AddBoolRow(Table, "Height map", IsEnumFlagSet(Flags, EMaterialFlags::EnableHeight),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableHeightMap(bValue);
            RequestRebuild();
        }), nullptr, bHasHeightTexture);

    AddBoolRow(Table, "Parallax clipping", IsEnumFlagSet(Flags, EMaterialFlags::EnableParallaxClipping),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableParallaxClipping(bValue);
        }), nullptr, Material->HasHeightMap());

    AddBoolRow(Table, "Force forward pass", IsEnumFlagSet(Flags, EMaterialFlags::ForceForwardPass),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->ForceForwardPass(bValue);
        }));

    AddBoolRow(Table, "Translucent", IsEnumFlagSet(Flags, EMaterialFlags::Translucent),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableTranslucent(bValue);
            RequestRebuild();
        }));

    AddBoolRow(Table, "Refraction", IsEnumFlagSet(Flags, EMaterialFlags::EnableRefraction),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableRefraction(bValue);
        }), nullptr, Material->IsTranslucent());
}

void FEditorPropertiesPanel::AddParallaxRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

    AddFloatRow(Table, "Height scale", MaterialInfo.ParallaxHeightScale, 0.0f, 0.2f, 0.001f, 0.03f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxHeightScale(Value);
        }));

    AddFloatRow(Table, "Min layers", MaterialInfo.ParallaxMinLayers, 1.0f, 128.0f, 1.0f, 32.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxLayers(Value, Material->GetParallaxMaxLayers());
        }));

    AddFloatRow(Table, "Max layers", MaterialInfo.ParallaxMaxLayers, 1.0f, 256.0f, 1.0f, 64.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxLayers(Material->GetParallaxMinLayers(), Value);
        }));
}

void FEditorPropertiesPanel::BuildPointLightSection(const TSharedPtr<FVerticalBox>& InColumn, FLightComponent* Component)
{
    InColumn->AddSlot(CreateSectionLabel("Light"));

    TSharedPtr<FPropertyTable> SettingsTable = CreateTable();
    AddLightSettingRows(SettingsTable, Component, Cast<FPointLightComponent>(Component) ? "Intensity (Lumen)" : "Intensity");
    AddSection(InColumn, "Settings", SettingsTable);

    TSharedPtr<FPropertyTable> ShadowTable = CreateTable();
    AddShadowRows(ShadowTable, Component);
    AddSection(InColumn, "Shadows", ShadowTable);
}

void FEditorPropertiesPanel::BuildDirectionalLightSection(const TSharedPtr<FVerticalBox>& InColumn, FDirectionalLightComponent* Component)
{
    InColumn->AddSlot(CreateSectionLabel("DirectionalLight"));

    TSharedPtr<FPropertyTable> SettingsTable = CreateTable();
    AddLightSettingRows(SettingsTable, Component, "Intensity (Lux)");
    AddSection(InColumn, "Settings", SettingsTable);

    TSharedPtr<FPropertyTable> DirectionTable = CreateTable();
    AddLightDirectionRows(DirectionTable, Component);
    AddSection(InColumn, "Direction", DirectionTable);

    TSharedPtr<FPropertyTable> ShadowTable = CreateTable();
    AddShadowRows(ShadowTable, Component);
    AddSection(InColumn, "Shadows", ShadowTable);

    TSharedPtr<FPropertyTable> CascadeTable = CreateTable();
    AddCascadeRows(CascadeTable, Component);
    AddSection(InColumn, "Cascades", CascadeTable);
}

void FEditorPropertiesPanel::AddLightSettingRows(const TSharedPtr<FPropertyTable>& Table, FLightComponent* Component, const CHAR* IntensityLabel)
{
    const Vector3     Color        = Component->GetColor();
    const FFloatColor ColorDefault = FFloatColor::White;

    AddColorRow(Table, "Color", FFloatColor(Color.X, Color.Y, Color.Z, 1.0f), &ColorDefault,
        FOnColorChanged::CreateLambda([Component](const FFloatColor& Value)
        {
            Component->SetColor(Vector3(Value.R, Value.G, Value.B));
        }));

    AddFloatRow(Table, IntensityLabel, Component->GetIntensity(), 0.0f, 200000.0f, 0.1f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetIntensity(Value);
        }));
}

void FEditorPropertiesPanel::AddShadowRows(const TSharedPtr<FPropertyTable>& Table, FLightComponent* Component)
{
    FPointLightComponent* PointLight = Cast<FPointLightComponent>(Component);

    const bool bCastShadowsDefault = true;
    AddBoolRow(Table, "Cast shadows", Component->CastsShadows(),
        TDelegate<void(bool)>::CreateLambda([Component](bool bValue)
        {
            Component->SetCastShadows(bValue);
        }), &bCastShadowsDefault);

    AddFloatRow(Table, "Shadow bias", Component->GetShadowBias(), 0.0001f, 0.1f, 0.0001f, 0.005f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetShadowBias(Value);
        }));

    const float NearPlaneMax     = PointLight ? 1.0f : 1000.0f;
    const float NearPlaneStep    = PointLight ? 0.01f : 1.0f;
    const float NearPlaneDefault = PointLight ? 1.0f : 120.0f;
    const float FarPlaneMax      = PointLight ? 100.0f : 1000.0f;
    const float FarPlaneDefault  = PointLight ? 30.0f : 250.0f;

    AddFloatRow(Table, "Shadow near plane", Component->GetShadowNearPlane(), 0.01f, NearPlaneMax, NearPlaneStep, NearPlaneDefault,
        TDelegate<void(float)>::CreateLambda([Component, PointLight](float Value)
        {
            if (PointLight)
            {
                PointLight->SetShadowNearPlane(Value);
            }
            else
            {
                Component->SetShadowNearPlane(Value);
            }
        }));

    AddFloatRow(Table, "Shadow far plane", Component->GetShadowFarPlane(), 0.01f, FarPlaneMax, 1.0f, FarPlaneDefault,
        TDelegate<void(float)>::CreateLambda([Component, PointLight](float Value)
        {
            if (PointLight)
            {
                PointLight->SetShadowFarPlane(Value);
            }
            else
            {
                Component->SetShadowFarPlane(Value);
            }
        }));
}

void FEditorPropertiesPanel::AddLightDirectionRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component)
{
    FActor* Actor = Component->GetActorOwner();
    if (!Actor)
    {
        return;
    }

    const Vector3 Rotation = Actor->GetWorldTransform().GetRotation();

    AddFloatRow(Table, "Rotation theta (degrees)", Math::RadiansToDegrees(Rotation.X), -90.0f, 90.0f, 0.25f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Actor](float Degrees)
        {
            FActorTransform NewWorldTransform = Actor->GetWorldTransform();

            Vector3 NewRotation = NewWorldTransform.GetRotation();
            NewRotation.X = Math::DegreesToRadians(Degrees);

            NewWorldTransform.SetRotation(NewRotation);
            Actor->SetWorldTransform(NewWorldTransform);
        }));

    AddFloatRow(Table, "Rotation phi (degrees)", Math::RadiansToDegrees(Rotation.Y), 0.0f, 360.0f, 0.25f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Actor](float Degrees)
        {
            FActorTransform NewWorldTransform = Actor->GetWorldTransform();

            Vector3 NewRotation = NewWorldTransform.GetRotation();
            NewRotation.Y = Math::DegreesToRadians(Degrees);

            NewWorldTransform.SetRotation(NewRotation);
            Actor->SetWorldTransform(NewWorldTransform);
        }));

    const Vector3 Direction = Component->GetDirectionVector();

    TSharedPtr<FHorizontalBox> DirectionRow = FHorizontalBox::Create();
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        TNumericEntry<float>::FDesc FieldDesc;
        FieldDesc.Value             = Direction[Axis];
        FieldDesc.Font              = FEditorStyle::GetFonts().Body;
        FieldDesc.bShowLabel        = false;
        FieldDesc.AccentEdge        = FUIStyle::GetDefault().AxisColors[Axis];
        FieldDesc.bDynamicPrecision = true;
        FieldDesc.bIsReadOnly       = true;

        LightDirectionFields[Axis] = TNumericEntry<float>::Create(FieldDesc);

        DirectionRow->AddSlot(LightDirectionFields[Axis]).SetFillCoefficient(1.0f).SetPadding(FMargin(Axis == 0 ? 0 : 2, 0, 0, 0));
    }

    Table->AddRow("Direction", DirectionRow);
}

void FEditorPropertiesPanel::AddCascadeRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component)
{
    AddFloatRow(Table, "Split lambda", Component->GetCascadeSplitLambda(), 0.0f, 1.0f, 0.01f, 0.95f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetCascadeSplitLambda(Value);
        }));

    AddFloatRow(Table, "Position offset", Component->GetShadowPositionOffset(), 0.0f, 1000.0f, 1.0f, 200.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetShadowPositionOffset(Value);
        }));

    AddFloatRow(Table, "Light area", Component->GetLightArea(), 0.0f, 1.0f, 0.01f, 0.05f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetLightArea(Value);
        }));
}

void FEditorPropertiesPanel::BuildCameraSection(const TSharedPtr<FVerticalBox>& InColumn, FCameraComponent* Component)
{
    InColumn->AddSlot(CreateSectionLabel("Camera"));

    TSharedPtr<FPropertyTable> SettingsTable = CreateTable();
    SettingsTable->AddRow("Viewport size", CreateTextRow(String::Printf("%.0f x %.0f", Component->GetWidth(), Component->GetHeight())));

    AddFloatRow(SettingsTable, "Field of view", Component->GetFieldOfView(), 40.0f, 120.0f, 0.1f, 60.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetFieldOfView(Value);
        }));

    AddFloatRow(SettingsTable, "Near plane", Component->GetNearPlane(), 0.001f, 10.0f, 0.001f, 0.01f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetNearPlane(Value);
        }));

    AddFloatRow(SettingsTable, "Far plane", Component->GetFarPlane(), 10.0f, 10000.0f, 1.0f, 200.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetFarPlane(Value);
        }));

    AddSection(InColumn, "Settings", SettingsTable);

    TSharedPtr<FPropertyTable> TransformTable = CreateTable();

    const Vector3 PositionDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(TransformTable, "Position", Component->GetPosition(), 0.1f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetPosition(Value);
        }), false, &PositionDefault,
        FOnVectorRead::CreateLambda([Component]() -> Vector3
        {
            return Component->GetPosition();
        }));

    const Vector3 RotationDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(TransformTable, "Rotation", Component->GetRotation(), 0.5f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetRotation(Value);
        }), true, &RotationDefault,
        FOnVectorRead::CreateLambda([Component]() -> Vector3
        {
            return Component->GetRotation();
        }));

    AddSection(InColumn, "Transform", TransformTable);
}

void FEditorPropertiesPanel::BuildLightProbeSection(const TSharedPtr<FVerticalBox>& InColumn, FLightProbeComponent* Component)
{
    InColumn->AddSlot(CreateSectionLabel("LightProbe"));

    TSharedPtr<FPropertyTable> Table = CreateTable();

    const Vector3 BoxExtentDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Box extents", Component->GetBoxExtents(), 0.1f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetBoxExtent(Value);
        }), false, &BoxExtentDefault);

    const Vector3 BoxOffsetDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Box offset", Component->GetBoxOffset(), 0.1f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetBoxOffset(Value);
        }), false, &BoxOffsetDefault);

    const bool bBoxProjectionDefault = false;
    AddBoolRow(Table, "Box projection", Component->GetBoxProjection(),
        TDelegate<void(bool)>::CreateLambda([Component](bool bValue)
        {
            Component->SetBoxProjection(bValue);
        }), &bBoxProjectionDefault);

    AddSection(InColumn, "Settings", Table);
}

FPropertyRow& FEditorPropertiesPanel::AddVectorRow(
    const TSharedPtr<FPropertyTable>& Table,
    const String&                     Label,
    const Vector3&                    Value,
    float                             Step,
    const FOnVectorChanged&           OnChanged,
    bool                              bIsAngular,
    const Vector3*                    DefaultValue,
    const FOnVectorRead&              OnRead,
    bool                              bAllowUniform)
{
    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();

    const int32 RowIndex = VectorRows.Size();

    FVectorRow Entry;
    Entry.OnChanged  = OnChanged;
    Entry.OnRead     = OnRead;
    Entry.bIsAngular = bIsAngular;

    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        TNumericEntry<float>::FDesc FieldDesc;
        FieldDesc.Value             = bIsAngular ? Math::RadiansToDegrees(Value[Axis]) : Value[Axis];
        FieldDesc.Step              = Step;
        FieldDesc.Font              = FEditorStyle::GetFonts().Body;
        FieldDesc.bShowLabel        = false;
        FieldDesc.AccentEdge        = FUIStyle::GetDefault().AxisColors[Axis];
        FieldDesc.Suffix            = bIsAngular ? GDegreeSuffix : "";
        FieldDesc.bDynamicPrecision = true;

        FieldDesc.OnValueChanged = TNumericEntry<float>::FOnValueChanged::CreateLambda(
            [this, RowIndex, Axis](float /*NewValue*/)
            {
                WriteVectorRow(RowIndex, Axis);
            });

        TSharedPtr<TNumericEntry<float>> Field = TNumericEntry<float>::Create(FieldDesc);
        Entry.Fields[Axis] = Field;

        Row->AddSlot(Field).SetFillCoefficient(1.0f).SetPadding(FMargin(Axis == 0 ? 0 : 2, 0, 0, 0));
    }

    VectorRows.Emplace(Entry);

    FPropertyRow& NewRow = Table->AddRow(Label, Row);

    if (bAllowUniform)
    {
        TSharedPtr<FUniformScaleToggle> Lock = FUniformScaleToggle::Create();
        Lock->OnToggled = TDelegate<void(bool)>::CreateLambda([this, RowIndex](bool bIsLocked)
        {
            if (VectorRows.IsValidIndex(RowIndex))
            {
                VectorRows[RowIndex].bIsUniform = bIsLocked;
            }
        });

        Table->SetRowLabelAccessory(Table->GetNumRows() - 1, Lock);
    }

    if (!DefaultValue)
    {
        return NewRow;
    }

    const Vector3 Default = *DefaultValue;

    NewRow.ToolTipText = String::Printf("Default %s", *FormatVector(Default));

    NewRow.OnRevert = FOnClicked::CreateLambda([this, RowIndex, Default]()
    {
        if (!VectorRows.IsValidIndex(RowIndex))
        {
            return;
        }

        const FVectorRow& Target = VectorRows[RowIndex];
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            Target.Fields[Axis]->SetValue(Target.bIsAngular ? Math::RadiansToDegrees(Default[Axis]) : Default[Axis]);
        }

        Target.OnChanged.ExecuteIfBound(Default);
    });

    NewRow.IsModified = FOnPropertyModified::CreateLambda([this, RowIndex, Default]() -> bool
    {
        if (!VectorRows.IsValidIndex(RowIndex))
        {
            return false;
        }

        const FVectorRow& Target = VectorRows[RowIndex];
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            const float Shown    = Target.Fields[Axis]->GetValue();
            const float Expected = Target.bIsAngular ? Math::RadiansToDegrees(Default[Axis]) : Default[Axis];

            if (Math::Abs(Shown - Expected) > REVERT_EPSILON)
            {
                return true;
            }
        }

        return false;
    });

    return NewRow;
}

FPropertyRow& FEditorPropertiesPanel::AddColorRow(
    const TSharedPtr<FPropertyTable>& Table,
    const String&                     Label,
    const FFloatColor&                Value,
    const FFloatColor*                DefaultValue,
    const FOnColorChanged&            OnChanged)
{
    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();

    const int32 RowIndex = ColorRows.Size();

    FColorRow Entry;
    Entry.OnChanged = OnChanged;

    FColorBlock::FDesc SwatchDesc;
    SwatchDesc.Color  = Value;
    SwatchDesc.Extent = FEditorStyle::GetStyle().Metrics.RowHeight;

    Entry.Swatch = FColorBlock::Create(SwatchDesc);

    FMenuAnchor::FDesc AnchorDesc;
    AnchorDesc.SetContent(Entry.Swatch);
    AnchorDesc.Placement = EMenuPlacement::BelowLeftAligned;

    AnchorDesc.OnGetMenuContent = FOnGetMenuContent::CreateLambda([this, RowIndex]() -> TSharedPtr<FVisualElement>
    {
        if (!ColorRows.IsValidIndex(RowIndex))
        {
            return nullptr;
        }

        FColorPicker::FDesc PickerDesc;
        PickerDesc.Color = ColorRows[RowIndex].Swatch->GetColor();

        PickerDesc.OnColorPicked = FOnColorPicked::CreateLambda([this, RowIndex](const FFloatColor& NewColor)
        {
            if (!ColorRows.IsValidIndex(RowIndex))
            {
                return;
            }

            const FColorRow& Target = ColorRows[RowIndex];
            Target.Fields[0]->SetValue(NewColor.R);
            Target.Fields[1]->SetValue(NewColor.G);
            Target.Fields[2]->SetValue(NewColor.B);

            WriteColorRow(RowIndex);
        });

        return FColorPicker::Create(PickerDesc);
    });

    TSharedPtr<FMenuAnchor> Anchor = FMenuAnchor::Create(AnchorDesc);

    Entry.Swatch->GetOnClicked() = FOnClicked::CreateLambda([AnchorPtr = Anchor.Get()]()
    {
        AnchorPtr->Toggle();
    });

    Row->AddSlot(Anchor);

    for (int32 Channel = 0; Channel < 3; ++Channel)
    {
        TNumericEntry<float>::FDesc FieldDesc;
        FieldDesc.Value             = Value.RGBA[Channel];
        FieldDesc.MinValue          = 0.0f;
        FieldDesc.MaxValue          = 1.0f;
        FieldDesc.Step              = COLOR_ROW_STEP;
        FieldDesc.Font              = FEditorStyle::GetFonts().Body;
        FieldDesc.bShowLabel        = false;
        FieldDesc.bDynamicPrecision = true;

        FieldDesc.OnValueChanged = TNumericEntry<float>::FOnValueChanged::CreateLambda(
            [this, RowIndex](float /*NewValue*/)
            {
                WriteColorRow(RowIndex);
            });

        Entry.Fields[Channel] = TNumericEntry<float>::Create(FieldDesc);

        Row->AddSlot(Entry.Fields[Channel]).SetFillCoefficient(1.0f).SetPadding(FMargin(COLOR_ROW_SPACING, 0, 0, 0));
    }

    ColorRows.Emplace(Entry);

    FPropertyRow& NewRow = Table->AddRow(Label, Row);

    if (!DefaultValue)
    {
        return NewRow;
    }

    const FFloatColor Default = *DefaultValue;

    NewRow.ToolTipText = String::Printf("Default %.3f, %.3f, %.3f", Default.R, Default.G, Default.B);

    NewRow.OnRevert = FOnClicked::CreateLambda([this, RowIndex, Default]()
    {
        if (!ColorRows.IsValidIndex(RowIndex))
        {
            return;
        }

        const FColorRow& Target = ColorRows[RowIndex];
        for (int32 Channel = 0; Channel < 3; ++Channel)
        {
            Target.Fields[Channel]->SetValue(Default.RGBA[Channel]);
        }

        WriteColorRow(RowIndex);
    });

    NewRow.IsModified = FOnPropertyModified::CreateLambda([this, RowIndex, Default]() -> bool
    {
        if (!ColorRows.IsValidIndex(RowIndex))
        {
            return false;
        }

        const FColorRow& Target = ColorRows[RowIndex];
        for (int32 Channel = 0; Channel < 3; ++Channel)
        {
            if (Math::Abs(Target.Fields[Channel]->GetValue() - Default.RGBA[Channel]) > REVERT_EPSILON)
            {
                return true;
            }
        }

        return false;
    });

    return NewRow;
}

FPropertyRow& FEditorPropertiesPanel::AddFloatRow(
    const TSharedPtr<FPropertyTable>& Table,
    const String&                     Label,
    float                             Value,
    float                             MinValue,
    float                             MaxValue,
    float                             Step,
    float                             DefaultValue,
    const TDelegate<void(float)>&     OnChanged)
{
    TSharedPtr<TNumericEntry<float>> Field = CreateFloatEditor(Value, MinValue, MaxValue, Step, OnChanged);

    FPropertyRow& NewRow = Table->AddRow(Label, Field);
    NewRow.ToolTipText   = String::Printf("Default %.4f", DefaultValue);

    NewRow.OnRevert = FOnClicked::CreateLambda([Field, DefaultValue, OnChanged]()
    {
        Field->SetValue(DefaultValue);
        OnChanged.ExecuteIfBound(Field->GetValue());
    });

    NewRow.IsModified = FOnPropertyModified::CreateLambda([Field, DefaultValue]() -> bool
    {
        return Math::Abs(Field->GetValue() - DefaultValue) > REVERT_EPSILON;
    });

    return NewRow;
}

FPropertyRow& FEditorPropertiesPanel::AddBoolRow(
    const TSharedPtr<FPropertyTable>& Table,
    const String&                     Label,
    bool                              bValue,
    const TDelegate<void(bool)>&      OnChanged,
    const bool*                       DefaultValue,
    bool                              bIsEnabled)
{
    TSharedPtr<FCheckBox> Field = CreateBoolEditor(bValue, OnChanged);
    Field->SetEnabled(bIsEnabled);

    FPropertyRow& NewRow = Table->AddRow(Label, Field);

    if (!DefaultValue || !bIsEnabled)
    {
        return NewRow;
    }

    const bool bDefaultValue = *DefaultValue;

    NewRow.ToolTipText = bDefaultValue ? String("Default on") : String("Default off");

    NewRow.OnRevert = FOnClicked::CreateLambda([Field, bDefaultValue, OnChanged]()
    {
        Field->SetCheckState(bDefaultValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        OnChanged.ExecuteIfBound(bDefaultValue);
    });

    NewRow.IsModified = FOnPropertyModified::CreateLambda([Field, bDefaultValue]() -> bool
    {
        return Field->IsChecked() != bDefaultValue;
    });

    return NewRow;
}

void FEditorPropertiesPanel::WriteVectorRow(int32 RowIndex, int32 DrivingAxis)
{
    if (!VectorRows.IsValidIndex(RowIndex))
    {
        return;
    }

    const FVectorRow& Row = VectorRows[RowIndex];

    if (Row.bIsUniform && Row.Fields[DrivingAxis])
    {
        const float Driving = Row.Fields[DrivingAxis]->GetValue();
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            if (Axis != DrivingAxis)
            {
                Row.Fields[Axis]->SetValue(Driving);
            }
        }
    }

    Vector3 Result;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const float Field = Row.Fields[Axis]->GetValue();
        Result[Axis] = Row.bIsAngular ? Math::DegreesToRadians(Field) : Field;
    }

    Row.OnChanged.ExecuteIfBound(Result);
}

void FEditorPropertiesPanel::WriteColorRow(int32 RowIndex)
{
    if (!ColorRows.IsValidIndex(RowIndex))
    {
        return;
    }

    const FColorRow& Row = ColorRows[RowIndex];

    const FFloatColor Result(Row.Fields[0]->GetValue(), Row.Fields[1]->GetValue(), Row.Fields[2]->GetValue(), 1.0f);

    Row.Swatch->SetColor(Result);
    Row.OnChanged.ExecuteIfBound(Result);
}

void FEditorPropertiesPanel::RefreshValues()
{
    for (const FVectorRow& Row : VectorRows)
    {
        if (!Row.OnRead.IsBound())
        {
            continue;
        }

        const Vector3 Value = Row.OnRead.Execute();
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            const TSharedPtr<FEditableText>& FieldEditor = Row.Fields[Axis]->GetEditor();
            if (FieldEditor && FieldEditor->HasKeyboardFocus())
            {
                continue;
            }

            Row.Fields[Axis]->SetValue(Row.bIsAngular ? Math::RadiansToDegrees(Value[Axis]) : Value[Axis]);
        }
    }

    if (!LightDirectionFields[0])
    {
        return;
    }

    FActor* Actor = EditorEngine->GetSelectedActor();
    if (FDirectionalLightComponent* Light = Actor ? Actor->GetComponentOfType<FDirectionalLightComponent>() : nullptr)
    {
        const Vector3 Direction = Light->GetDirectionVector();
        for (int32 Axis = 0; Axis < 3; ++Axis)
        {
            LightDirectionFields[Axis]->SetValue(Direction[Axis]);
        }
    }
}

TSharedPtr<FPropertyTable> FEditorPropertiesPanel::CreateTable()
{
    return FPropertyTable::Create(FEditorStyle::MakePropertyTableDesc(PROPERTIES_LABEL_FRACTION, PROPERTIES_LABEL_COLUMN_WIDTH));
}

TSharedPtr<FSeparatorText> FEditorPropertiesPanel::CreateSectionLabel(const String& Label)
{
    FSeparatorText::FDesc Desc;
    Desc.Text = Label;
    Desc.Font = FEditorStyle::GetFonts().Body;

    return FSeparatorText::Create(Desc);
}

void FEditorPropertiesPanel::AddSection(const TSharedPtr<FVerticalBox>& InColumn, const String& Label, const TSharedPtr<FPropertyTable>& Table)
{
    InColumn->AddSlot(FExpander::Create(FEditorStyle::MakeExpanderDesc(Label, Table, true)));
}

TSharedPtr<TNumericEntry<float>> FEditorPropertiesPanel::CreateFloatEditor(float Value, float Min, float Max, float Step, const TDelegate<void(float)>& OnChanged)
{
    TNumericEntry<float>::FDesc Desc;
    Desc.Value          = Value;
    Desc.MinValue       = Min;
    Desc.MaxValue       = Max;
    Desc.Step           = Step;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.bShowLabel     = false;
    Desc.bShowFillTrack = true;
    Desc.TextAlignment  = EHorizontalAlignment::Center;
    Desc.OnValueChanged = OnChanged;

    return TNumericEntry<float>::Create(Desc);
}

TSharedPtr<TNumericEntry<int32>> FEditorPropertiesPanel::CreateIntEditor(int32 Value, int32 Min, int32 Max, const TDelegate<void(int32)>& OnChanged)
{
    TNumericEntry<int32>::FDesc Desc;
    Desc.Value          = Value;
    Desc.MinValue       = Min;
    Desc.MaxValue       = Max;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.bShowLabel     = false;
    Desc.OnValueChanged = OnChanged;

    return TNumericEntry<int32>::Create(Desc);
}

TSharedPtr<FCheckBox> FEditorPropertiesPanel::CreateBoolEditor(bool bValue, const TDelegate<void(bool)>& OnChanged)
{
    FCheckBox::FDesc Desc;
    Desc.InitialState = bValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    Desc.Font         = FEditorStyle::GetFonts().Body;
    Desc.OnStateChanged = FOnCheckStateChanged::CreateLambda([OnChanged](ECheckBoxState State)
    {
        OnChanged.ExecuteIfBound(State == ECheckBoxState::Checked);
    });

    return FCheckBox::Create(Desc);
}

TSharedPtr<FVisualElement> FEditorPropertiesPanel::CreateComboEditor(const TArray<String>& Options, int32 SelectedIndex, const TDelegate<void(int32)>& OnChanged)
{
    FComboBox::FDesc Desc;
    Desc.SetOptions(Options).SetFont(FEditorStyle::GetFonts().Body);
    Desc.SelectedIndex      = SelectedIndex;
    Desc.OnSelectionChanged = OnChanged;

    return FComboBox::Create(Desc);
}

TSharedPtr<FTextBlock> FEditorPropertiesPanel::CreateTextRow(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Body;

    return FTextBlock::Create(Desc);
}

TSharedPtr<FTextBlock> FEditorPropertiesPanel::CreateDisabledTextRow(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text            = Text;
    Desc.Font            = FEditorStyle::GetFonts().Body;
    Desc.ColorAndOpacity = FEditorStyle::GetStyle().Colors.TextDisabled;

    return FTextBlock::Create(Desc);
}
