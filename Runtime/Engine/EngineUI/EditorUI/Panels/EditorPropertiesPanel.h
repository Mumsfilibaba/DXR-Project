#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Color.h"
#include "Core/Math/Vector3.h"
#include "Application/Elements/NumericEntry.h"
#include "Application/Elements/PropertyTable.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FActor;
class FCameraComponent;
class FCheckBox;
class FColorBlock;
class FDirectionalLightComponent;
class FLightComponent;
class FLightProbeComponent;
class FMaterial;
class FScrollBox;
class FSeparatorText;
class FStaticMeshComponent;
class FTextBlock;
class FVerticalBox;

/** @brief Called with the vector a three-field row moved to. */
DECLARE_DELEGATE(FOnVectorChanged, const Vector3& /*NewValue*/);

/** @brief Called once a tick for the value a three-field row shows, on a row that follows the model. */
DECLARE_RETURN_DELEGATE(FOnVectorRead, Vector3);

/** @brief Called with the color a swatch and its three fields moved to. */
DECLARE_DELEGATE(FOnColorChanged, const FFloatColor& /*NewValue*/);

class ENGINE_API FEditorPropertiesPanel final : public FEditorPanel
{
public:
    FEditorPropertiesPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorPropertiesPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;
    virtual void OnActorRemoved(FActor* Actor) override final;

private:
    struct FVectorRow
    {
        TSharedPtr<TNumericEntry<float>> Fields[3];
        FOnVectorChanged                 OnChanged;
        FOnVectorRead                    OnRead;
        bool                             bIsAngular = false;
        bool                             bIsUniform = false;
    };

    struct FColorRow
    {
        TSharedPtr<FColorBlock>          Swatch;
        TSharedPtr<TNumericEntry<float>> Fields[3];
        FOnColorChanged                  OnChanged;
    };

    void RebuildContent();
    void RequestRebuild();
    void RefreshValues();
    void WriteVectorRow(int32 RowIndex, int32 DrivingAxis);
    void WriteColorRow(int32 RowIndex);

    void BuildAttachmentSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* ParentActor);
    void BuildTransformSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* Actor);
    void BuildStaticMeshSection(const TSharedPtr<FVerticalBox>& InColumn, FStaticMeshComponent* Component);
    void BuildPointLightSection(const TSharedPtr<FVerticalBox>& InColumn, FLightComponent* Component);
    void BuildDirectionalLightSection(const TSharedPtr<FVerticalBox>& InColumn, FDirectionalLightComponent* Component);
    void BuildCameraSection(const TSharedPtr<FVerticalBox>& InColumn, FCameraComponent* Component);
    void BuildLightProbeSection(const TSharedPtr<FVerticalBox>& InColumn, FLightProbeComponent* Component);

    void AddMaterialSlotRows(const TSharedPtr<FPropertyTable>& Table, FStaticMeshComponent* Component);
    void AddMaterialRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddMaterialTextureRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddMaterialFlagRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddParallaxRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddLightSettingRows(const TSharedPtr<FPropertyTable>& Table, FLightComponent* Component, const CHAR* IntensityLabel);
    void AddShadowRows(const TSharedPtr<FPropertyTable>& Table, FLightComponent* Component);
    void AddLightDirectionRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component);
    void AddCascadeRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component);
    void AddSection(const TSharedPtr<FVerticalBox>& InColumn, const String& SectionLabel, const TSharedPtr<FPropertyTable>& Table);

    FPropertyRow& AddVectorRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     RowLabel,
        const Vector3&                    Value,
        float                             Step,
        const FOnVectorChanged&           OnChanged,
        bool                              bIsAngular   = false,
        const Vector3*                    DefaultValue = nullptr,
        const FOnVectorRead&              OnRead       = FOnVectorRead(),
        bool                              bAllowUniform = false);

    FPropertyRow& AddColorRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     RowLabel,
        const FFloatColor&                Value,
        const FFloatColor*                DefaultValue,
        const FOnColorChanged&            OnChanged);

    FPropertyRow& AddFloatRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     RowLabel,
        float                             Value,
        float                             MinValue,
        float                             MaxValue,
        float                             Step,
        float                             DefaultValue,
        int32                             Precision,
        const TDelegate<void(float)>&     OnChanged);

    FPropertyRow& AddBoolRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     RowLabel,
        bool                              bValue,
        const TDelegate<void(bool)>&      OnChanged,
        const bool*                       DefaultValue = nullptr,
        bool                              bIsEnabled   = true);

    FPropertyRow& AddTextRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     RowLabel,
        const String&                     Text);

    NODISCARD TSharedPtr<FPropertyTable> CreateTable();
    NODISCARD TSharedPtr<FSeparatorText> CreateSectionLabel(const String& SectionLabel);
    NODISCARD TSharedPtr<TNumericEntry<float>> CreateFloatEditor(float Value, float Min, float Max, float Step, int32 Precision, const TDelegate<void(float)>& OnChanged);
    NODISCARD TSharedPtr<TNumericEntry<int32>> CreateIntEditor(int32 Value, int32 Min, int32 Max, const TDelegate<void(int32)>& OnChanged);
    NODISCARD TSharedPtr<FCheckBox> CreateBoolEditor(bool bValue, const TDelegate<void(bool)>& OnChanged);
    NODISCARD TSharedPtr<FVisualElement> CreateComboEditor(const TArray<String>& Options, int32 SelectedIndex, const TDelegate<void(int32)>& OnChanged);
    NODISCARD TSharedPtr<FTextBlock> CreateTextRow(const String& Text);
    NODISCARD TSharedPtr<FTextBlock> CreateDisabledTextRow(const String& Text);

    TSharedPtr<FScrollBox>           ScrollBox;
    TSharedPtr<FVerticalBox>         Column;
    TSharedPtr<TNumericEntry<float>> LightDirectionFields[3];
    TArray<FVectorRow>               VectorRows;
    TArray<FColorRow>                ColorRows;
    FActor*                          BuiltForActor;
    int32                            SelectedMaterialIndex;
    bool                             bRebuildRequested;
};
