#include "Engine/EngineUI/EditorUI/Panels/EditorPropertiesPanel.h"
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
#include "Application/Elements/Box.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/CheckBox.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Menus/ComboBox.h"

static const FFloatColor GAxisColors[3] =
{
    FFloatColor(0.62f, 0.20f, 0.22f, 1.0f),
    FFloatColor(0.22f, 0.52f, 0.24f, 1.0f),
    FFloatColor(0.20f, 0.35f, 0.62f, 1.0f),
};

static const CHAR* GAxisLabels[3] = { "X", "Y", "Z" };

static const CHAR* GMaterialTextureSlotNames[EMaterialTextureSlot::Count] =
{
    "Base color",
    "Normal",
    "Height",
    "Mask A",
    "Mask B",
    "Mask C",
    "Mask D",
};

static const CHAR* GNormalMapAxisLabels[2] = { "-Y (DirectX)", "+Y (OpenGL)" };
static const CHAR* GRevertButtonLabel = "R";

static String FormatVector(const Vector3& Value)
{
    return String::Printf("%.3f, %.3f, %.3f", Value.X, Value.Y, Value.Z);
}

FEditorPropertiesPanel::FEditorPropertiesPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "Properties", "Properties")
    , ScrollBox(nullptr)
    , Column(nullptr)
    , LightDirectionText(nullptr)
    , VectorRows()
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
    LightDirectionText.Reset();
    VectorRows.Clear();
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
    LightDirectionText.Reset();

    bRebuildRequested = false;

    FActor* Actor = EditorEngine->GetSelectedActor();
    if (Actor != BuiltForActor)
    {
        SelectedMaterialIndex = 0;
    }

    BuiltForActor = Actor;

    if (!Actor)
    {
        Column->AddSlot(MakeTextRow("Nothing selected")).SetPadding(FMargin(8, 8, 8, 8));
        return;
    }

    BuildTransformSection(Column, Actor);

    if (FStaticMeshComponent* Component = Actor->GetComponentOfType<FStaticMeshComponent>())
    {
        BuildStaticMeshSection(Column, Component);
    }

    if (FLightComponent* Component = Actor->GetComponentOfType<FLightComponent>())
    {
        BuildLightSection(Column, Component);
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

void FEditorPropertiesPanel::BuildTransformSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* Actor)
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);

    Table->AddRow("Name", MakeTextRow(Actor->GetName()));
    Table->AddRow("Type", MakeTextRow(Actor->GetTypeLabel()));

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
        }));

    FExpander::FDesc ExpanderDesc;
    ExpanderDesc.Label       = "Transform";
    ExpanderDesc.Font        = FEditorStyle::GetFonts().BodyBold;
    ExpanderDesc.Content     = Table;
    ExpanderDesc.bIsExpanded = true;

    InColumn->AddSlot(FExpander::Create(ExpanderDesc));
}

void FEditorPropertiesPanel::BuildStaticMeshSection(const TSharedPtr<FVerticalBox>& InColumn, FStaticMeshComponent* Component)
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);

    const int32 NumMaterials = Component->GetNumMaterials();
    SelectedMaterialIndex = Math::Clamp<int32>(SelectedMaterialIndex, 0, Math::Max<int32>(NumMaterials - 1, 0));

    Table->AddRow("Material slots", MakeTextRow(String::Printf("%d", NumMaterials)));

    if (NumMaterials > 1)
    {
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

        Table->AddRow("Material", MakeComboEditor(Options, SelectedMaterialIndex,
            TDelegate<void(int32)>::CreateLambda([this](int32 NewIndex)
            {
                SelectedMaterialIndex = NewIndex;
                RequestRebuild();
            })));
    }

    TSharedPtr<FMaterial> Material = Component->GetMaterial(SelectedMaterialIndex);
    if (Material)
    {
        FMaterial* MaterialPtr = Material.Get();

        AddMaterialRows(Table, MaterialPtr);
        AddMaterialTextureRows(Table, MaterialPtr);
        AddMaterialFlagRows(Table, MaterialPtr);

        if (MaterialPtr->HasHeightMap())
        {
            AddParallaxRows(Table, MaterialPtr);
        }
    }

    FExpander::FDesc ExpanderDesc;
    ExpanderDesc.Label       = "Static Mesh";
    ExpanderDesc.Font        = FEditorStyle::GetFonts().BodyBold;
    ExpanderDesc.Content     = Table;
    ExpanderDesc.bIsExpanded = true;

    InColumn->AddSlot(FExpander::Create(ExpanderDesc));
}

void FEditorPropertiesPanel::AddMaterialRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

    Table->AddHeaderRow("Material");

    const Vector3 AlbedoDefault = Vector3(1.0f, 1.0f, 1.0f);
    AddVectorRow(Table, "Albedo", Vector3(MaterialInfo.Albedo.R, MaterialInfo.Albedo.G, MaterialInfo.Albedo.B), 0.01f,
        FOnVectorChanged::CreateLambda([Material](const Vector3& Value)
        {
            Material->SetAlbedo(FFloatColor(Value.X, Value.Y, Value.Z, 1.0f));
        }), false, &AlbedoDefault).IndentLevel = 1;

    AddFloatRow(Table, "Roughness", MaterialInfo.Roughness, 0.01f, 1.0f, 0.01f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetRoughness(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Metallic", MaterialInfo.Metallic, 0.01f, 1.0f, 0.01f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetMetallic(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Ambient occlusion", MaterialInfo.AmbientOcclusion, 0.01f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetAmbientOcclusion(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Opacity", MaterialInfo.Opacity, 0.0f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetOpacity(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Index of refraction", MaterialInfo.IndexOfRefraction, 1.0f, 3.0f, 0.01f, 1.5f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetIndexOfRefraction(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Refraction strength", MaterialInfo.RefractionStrength, 0.0f, 1.0f, 0.01f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetRefractionStrength(Value);
        })).IndentLevel = 1;
}

void FEditorPropertiesPanel::AddMaterialTextureRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    Table->AddHeaderRow("Textures");

    for (uint32 Slot = 0; Slot < EMaterialTextureSlot::Count; ++Slot)
    {
        const FRHITextureRef& Texture = Material->GetTexture(EMaterialTextureSlot::Type(Slot));
        if (!Texture.IsValid())
        {
            continue;
        }

        const IntVector3& Extent = Texture->GetDesc().Extent;
        Table->AddRow(GMaterialTextureSlotNames[Slot], MakeTextRow(String::Printf("%d x %d", Extent.X, Extent.Y))).IndentLevel = 1;
    }
}

void FEditorPropertiesPanel::AddMaterialFlagRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    Table->AddHeaderRow("Material Flags");

    const EMaterialFlags Flags             = Material->GetMaterialInfo().MaterialFlags;
    const bool           bHasNormalTexture = Material->GetTexture(EMaterialTextureSlot::Normal).IsValid();
    const bool           bHasHeightTexture = Material->GetTexture(EMaterialTextureSlot::Height).IsValid();

    AddBoolRow(Table, "Normal mapping", IsEnumFlagSet(Flags, EMaterialFlags::EnableNormalMapping),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableNormalMapping(bValue);
            RequestRebuild();
        }), nullptr, bHasNormalTexture).IndentLevel = 1;

    if (IsEnumFlagSet(Flags, EMaterialFlags::EnableNormalMapping) && bHasNormalTexture)
    {
        TArray<String> AxisOptions;
        AxisOptions.Reserve(2);
        AxisOptions.Emplace(GNormalMapAxisLabels[0]);
        AxisOptions.Emplace(GNormalMapAxisLabels[1]);

        Table->AddRow("Normal map axis", MakeComboEditor(AxisOptions, Material->IsNormalMapPositiveY() ? 1 : 0,
            TDelegate<void(int32)>::CreateLambda([Material](int32 NewIndex)
            {
                Material->SetNormalMapPositiveY(NewIndex == 1);
            }))).IndentLevel = 2;
    }

    AddBoolRow(Table, "Alpha mask", IsEnumFlagSet(Flags, EMaterialFlags::EnableAlpha),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableAlphaMask(bValue);
        }), nullptr, Material->IsRouteFed(EMaterialScalar::Opacity) && !Material->IsTranslucent()).IndentLevel = 1;

    AddBoolRow(Table, "Double sided", IsEnumFlagSet(Flags, EMaterialFlags::DoubleSided),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableDoubleSided(bValue);
        })).IndentLevel = 1;

    AddBoolRow(Table, "Height map", IsEnumFlagSet(Flags, EMaterialFlags::EnableHeight),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableHeightMap(bValue);
            RequestRebuild();
        }), nullptr, bHasHeightTexture).IndentLevel = 1;

    AddBoolRow(Table, "Parallax clipping", IsEnumFlagSet(Flags, EMaterialFlags::EnableParallaxClipping),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableParallaxClipping(bValue);
        }), nullptr, Material->HasHeightMap()).IndentLevel = 1;

    AddBoolRow(Table, "Force forward pass", IsEnumFlagSet(Flags, EMaterialFlags::ForceForwardPass),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->ForceForwardPass(bValue);
        })).IndentLevel = 1;

    AddBoolRow(Table, "Translucent", IsEnumFlagSet(Flags, EMaterialFlags::Translucent),
        TDelegate<void(bool)>::CreateLambda([this, Material](bool bValue)
        {
            Material->EnableTranslucent(bValue);
            RequestRebuild();
        })).IndentLevel = 1;

    AddBoolRow(Table, "Refraction", IsEnumFlagSet(Flags, EMaterialFlags::EnableRefraction),
        TDelegate<void(bool)>::CreateLambda([Material](bool bValue)
        {
            Material->EnableRefraction(bValue);
        }), nullptr, Material->IsTranslucent()).IndentLevel = 1;
}

void FEditorPropertiesPanel::AddParallaxRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material)
{
    const FMaterialInfo& MaterialInfo = Material->GetMaterialInfo();

    Table->AddHeaderRow("Parallax");

    AddFloatRow(Table, "Height scale", MaterialInfo.ParallaxHeightScale, 0.0f, 0.2f, 0.001f, 0.03f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxHeightScale(Value);
        })).IndentLevel = 1;

    // Either end of the range is written through the one setter, so the other end is read back as it stands
    AddFloatRow(Table, "Min layers", MaterialInfo.ParallaxMinLayers, 1.0f, 128.0f, 1.0f, 32.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxLayers(Value, Material->GetParallaxMaxLayers());
        })).IndentLevel = 1;

    AddFloatRow(Table, "Max layers", MaterialInfo.ParallaxMaxLayers, 1.0f, 256.0f, 1.0f, 64.0f,
        TDelegate<void(float)>::CreateLambda([Material](float Value)
        {
            Material->SetParallaxLayers(Material->GetParallaxMinLayers(), Value);
        })).IndentLevel = 1;
}

void FEditorPropertiesPanel::BuildLightSection(const TSharedPtr<FVerticalBox>& InColumn, FLightComponent* Component)
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);

    FPointLightComponent*       PointLight       = Cast<FPointLightComponent>(Component);
    FDirectionalLightComponent* DirectionalLight = Cast<FDirectionalLightComponent>(Component);

    const Vector3 ColorDefault = Vector3(1.0f, 1.0f, 1.0f);
    AddVectorRow(Table, "Color", Component->GetColor(), 0.01f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetColor(Value);
        }), false, &ColorDefault);

    const CHAR* IntensityLabel = "Intensity";
    if (PointLight)
    {
        IntensityLabel = "Intensity (Lumen)";
    }
    else if (DirectionalLight)
    {
        IntensityLabel = "Intensity (Lux)";
    }

    AddFloatRow(Table, IntensityLabel, Component->GetIntensity(), 0.0f, 200000.0f, 0.1f, 1.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetIntensity(Value);
        }));

    if (DirectionalLight)
    {
        AddLightDirectionRows(Table, DirectionalLight);
    }

    Table->AddHeaderRow("Shadows");

    const bool bCastShadowsDefault = true;
    AddBoolRow(Table, "Cast shadows", Component->CastsShadows(),
        TDelegate<void(bool)>::CreateLambda([Component](bool bValue)
        {
            Component->SetCastShadows(bValue);
        }), &bCastShadowsDefault).IndentLevel = 1;

    AddFloatRow(Table, "Shadow bias", Component->GetShadowBias(), 0.0001f, 0.1f, 0.0001f, 0.005f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetShadowBias(Value);
        })).IndentLevel = 1;

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
        })).IndentLevel = 1;

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
        })).IndentLevel = 1;

    if (DirectionalLight)
    {
        AddCascadeRows(Table, DirectionalLight);
    }

    FExpander::FDesc ExpanderDesc;
    ExpanderDesc.Label       = "Light";
    ExpanderDesc.Font        = FEditorStyle::GetFonts().BodyBold;
    ExpanderDesc.Content     = Table;
    ExpanderDesc.bIsExpanded = true;

    InColumn->AddSlot(FExpander::Create(ExpanderDesc));
}

void FEditorPropertiesPanel::AddLightDirectionRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component)
{
    FActor* Actor = Component->GetActorOwner();
    if (!Actor)
    {
        return;
    }

    Table->AddHeaderRow("Direction");

    const Vector3 Rotation = Actor->GetWorldTransform().GetRotation();

    AddFloatRow(Table, "Rotation theta (degrees)", Math::RadiansToDegrees(Rotation.X), -90.0f, 90.0f, 0.25f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Actor](float Degrees)
        {
            FActorTransform NewWorldTransform = Actor->GetWorldTransform();

            Vector3 NewRotation = NewWorldTransform.GetRotation();
            NewRotation.X = Math::DegreesToRadians(Degrees);

            NewWorldTransform.SetRotation(NewRotation);
            Actor->SetWorldTransform(NewWorldTransform);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Rotation phi (degrees)", Math::RadiansToDegrees(Rotation.Y), 0.0f, 360.0f, 0.25f, 0.0f,
        TDelegate<void(float)>::CreateLambda([Actor](float Degrees)
        {
            FActorTransform NewWorldTransform = Actor->GetWorldTransform();

            Vector3 NewRotation = NewWorldTransform.GetRotation();
            NewRotation.Y = Math::DegreesToRadians(Degrees);

            NewWorldTransform.SetRotation(NewRotation);
            Actor->SetWorldTransform(NewWorldTransform);
        })).IndentLevel = 1;

    LightDirectionText = MakeTextRow(FormatVector(Component->GetDirectionVector()));
    Table->AddRow("Direction", LightDirectionText).IndentLevel = 1;
}

void FEditorPropertiesPanel::AddCascadeRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component)
{
    Table->AddHeaderRow("Cascades");

    AddFloatRow(Table, "Split lambda", Component->GetCascadeSplitLambda(), 0.0f, 1.0f, 0.01f, 0.95f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetCascadeSplitLambda(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Position offset", Component->GetShadowPositionOffset(), 0.0f, 1000.0f, 1.0f, 200.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetShadowPositionOffset(Value);
        })).IndentLevel = 1;

    AddFloatRow(Table, "Light area", Component->GetLightArea(), 0.0f, 1.0f, 0.01f, 0.05f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetLightArea(Value);
        })).IndentLevel = 1;
}

void FEditorPropertiesPanel::BuildCameraSection(const TSharedPtr<FVerticalBox>& InColumn, FCameraComponent* Component)
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);

    Table->AddRow("Viewport size", MakeTextRow(String::Printf("%.0f x %.0f", Component->GetWidth(), Component->GetHeight())));

    AddFloatRow(Table, "Field of view", Component->GetFieldOfView(), 40.0f, 120.0f, 0.1f, 60.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetFieldOfView(Value);
        }));

    AddFloatRow(Table, "Near plane", Component->GetNearPlane(), 0.001f, 10.0f, 0.001f, 0.01f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetNearPlane(Value);
        }));

    AddFloatRow(Table, "Far plane", Component->GetFarPlane(), 10.0f, 10000.0f, 1.0f, 200.0f,
        TDelegate<void(float)>::CreateLambda([Component](float Value)
        {
            Component->SetFarPlane(Value);
        }));

    Table->AddHeaderRow("Transform");

    const Vector3 PositionDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Position", Component->GetPosition(), 0.1f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetPosition(Value);
        }), false, &PositionDefault,
        FOnVectorRead::CreateLambda([Component]() -> Vector3
        {
            return Component->GetPosition();
        })).IndentLevel = 1;

    const Vector3 RotationDefault = Vector3(0.0f, 0.0f, 0.0f);
    AddVectorRow(Table, "Rotation", Component->GetRotation(), 0.5f,
        FOnVectorChanged::CreateLambda([Component](const Vector3& Value)
        {
            Component->SetRotation(Value);
        }), true, &RotationDefault,
        FOnVectorRead::CreateLambda([Component]() -> Vector3
        {
            return Component->GetRotation();
        })).IndentLevel = 1;

    FExpander::FDesc ExpanderDesc;
    ExpanderDesc.Label       = "Camera";
    ExpanderDesc.Font        = FEditorStyle::GetFonts().BodyBold;
    ExpanderDesc.Content     = Table;
    ExpanderDesc.bIsExpanded = true;

    InColumn->AddSlot(FExpander::Create(ExpanderDesc));
}

void FEditorPropertiesPanel::BuildLightProbeSection(const TSharedPtr<FVerticalBox>& InColumn, FLightProbeComponent* Component)
{
    FPropertyTable::FDesc TableDesc;
    TableDesc.Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(TableDesc);

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

    FExpander::FDesc ExpanderDesc;
    ExpanderDesc.Label       = "Light Probe";
    ExpanderDesc.Font        = FEditorStyle::GetFonts().BodyBold;
    ExpanderDesc.Content     = Table;
    ExpanderDesc.bIsExpanded = true;

    InColumn->AddSlot(FExpander::Create(ExpanderDesc));
}

FPropertyRow& FEditorPropertiesPanel::AddVectorRow(
    const TSharedPtr<FPropertyTable>& Table,
    const String&                     Label,
    const Vector3&                    Value,
    float                             Step,
    const FOnVectorChanged&           OnChanged,
    bool                              bIsAngular,
    const Vector3*                    DefaultValue,
    const FOnVectorRead&              OnRead)
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
        FieldDesc.Label      = GAxisLabels[Axis];
        FieldDesc.Value      = bIsAngular ? Math::RadiansToDegrees(Value[Axis]) : Value[Axis];
        FieldDesc.Step       = Step;
        FieldDesc.Font       = FEditorStyle::GetFonts().Body;
        FieldDesc.LabelColor = GAxisColors[Axis];

        FieldDesc.OnValueChanged = TNumericEntry<float>::FOnValueChanged::CreateLambda(
            [this, RowIndex](float /*NewValue*/)
            {
                WriteVectorRow(RowIndex);
            });

        TSharedPtr<TNumericEntry<float>> Field = TNumericEntry<float>::Create(FieldDesc);
        Entry.Fields[Axis] = Field;

        Row->AddSlot(Field).SetFillCoefficient(1.0f).SetPadding(FMargin(Axis == 0 ? 0 : 2, 0, 0, 0));
    }

    VectorRows.Emplace(Entry);

    if (!DefaultValue)
    {
        return Table->AddRow(Label, Row);
    }

    const Vector3 Default = *DefaultValue;

    TSharedPtr<FVisualElement> RevertableRow = MakeRevertableRow(Row, FOnClicked::CreateLambda([this, RowIndex, Default]()
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
    }));

    FPropertyRow& NewRow = Table->AddRow(Label, RevertableRow);
    NewRow.ToolTipText   = String::Printf("Default %s", *FormatVector(Default));
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
    TSharedPtr<TNumericEntry<float>> Field = MakeFloatEditor(Value, MinValue, MaxValue, Step, OnChanged);

    TSharedPtr<FVisualElement> Row = MakeRevertableRow(Field, FOnClicked::CreateLambda([Field, DefaultValue, OnChanged]()
    {
        Field->SetValue(DefaultValue);
        OnChanged.ExecuteIfBound(Field->GetValue());
    }));

    FPropertyRow& NewRow = Table->AddRow(Label, Row);
    NewRow.ToolTipText   = String::Printf("Default %.4f", DefaultValue);
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
    TSharedPtr<FCheckBox> Field = MakeBoolEditor(bValue, OnChanged);
    Field->SetEnabled(bIsEnabled);

    if (!DefaultValue || !bIsEnabled)
    {
        return Table->AddRow(Label, Field);
    }

    const bool bDefaultValue = *DefaultValue;
    TSharedPtr<FVisualElement> Row = MakeRevertableRow(Field, FOnClicked::CreateLambda([Field, bDefaultValue, OnChanged]()
    {
        Field->SetCheckState(bDefaultValue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
        OnChanged.ExecuteIfBound(bDefaultValue);
    }));

    FPropertyRow& NewRow = Table->AddRow(Label, Row);
    NewRow.ToolTipText   = bDefaultValue ? String("Default on") : String("Default off");
    return NewRow;
}

void FEditorPropertiesPanel::WriteVectorRow(int32 RowIndex)
{
    if (!VectorRows.IsValidIndex(RowIndex))
    {
        return;
    }

    const FVectorRow& Row = VectorRows[RowIndex];

    Vector3 Result;
    for (int32 Axis = 0; Axis < 3; ++Axis)
    {
        const float Field = Row.Fields[Axis]->GetValue();
        Result[Axis] = Row.bIsAngular ? Math::DegreesToRadians(Field) : Field;
    }

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

    if (LightDirectionText)
    {
        FActor* Actor = EditorEngine->GetSelectedActor();
        if (FDirectionalLightComponent* Light = Actor ? Actor->GetComponentOfType<FDirectionalLightComponent>() : nullptr)
        {
            LightDirectionText->SetText(FormatVector(Light->GetDirectionVector()));
        }
    }
}

TSharedPtr<FVisualElement> FEditorPropertiesPanel::MakeRevertableRow(const TSharedPtr<FVisualElement>& Editor, const TDelegate<void()>& OnRevert)
{
    FButton::FDesc ButtonDesc;
    ButtonDesc.SetText(GRevertButtonLabel).SetFont(FEditorStyle::GetFonts().Body);

    ButtonDesc.Padding   = FMargin(4, 0, 4, 0);
    ButtonDesc.OnClicked = OnRevert;

    TSharedPtr<FHorizontalBox> Row = FHorizontalBox::Create();
    Row->AddSlot(Editor).SetFillCoefficient(1.0f);
    Row->AddSlot(FButton::Create(ButtonDesc)).SetPadding(FMargin(4, 0, 0, 0));

    return Row;
}

TSharedPtr<TNumericEntry<float>> FEditorPropertiesPanel::MakeFloatEditor(float Value, float Min, float Max, float Step, const TDelegate<void(float)>& OnChanged)
{
    TNumericEntry<float>::FDesc Desc;
    Desc.Value          = Value;
    Desc.MinValue       = Min;
    Desc.MaxValue       = Max;
    Desc.Step           = Step;
    Desc.Font           = FEditorStyle::GetFonts().Body;
    Desc.bShowLabel     = false;
    Desc.OnValueChanged = OnChanged;

    return TNumericEntry<float>::Create(Desc);
}

TSharedPtr<TNumericEntry<int32>> FEditorPropertiesPanel::MakeIntEditor(int32 Value, int32 Min, int32 Max, const TDelegate<void(int32)>& OnChanged)
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

TSharedPtr<FCheckBox> FEditorPropertiesPanel::MakeBoolEditor(bool bValue, const TDelegate<void(bool)>& OnChanged)
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

TSharedPtr<FVisualElement> FEditorPropertiesPanel::MakeComboEditor(const TArray<String>& Options, int32 SelectedIndex, const TDelegate<void(int32)>& OnChanged)
{
    FComboBox::FDesc Desc;
    Desc.SetOptions(Options).SetFont(FEditorStyle::GetFonts().Body);
    Desc.SelectedIndex      = SelectedIndex;
    Desc.OnSelectionChanged = OnChanged;

    return FComboBox::Create(Desc);
}

TSharedPtr<FTextBlock> FEditorPropertiesPanel::MakeTextRow(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Body;

    return FTextBlock::Create(Desc);
}
