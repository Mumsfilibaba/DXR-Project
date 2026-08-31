#pragma once
#include "Core/Containers/Array.h"
#include "Core/Math/Vector3.h"
#include "Application/Elements/NumericEntry.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FActor;
class FCameraComponent;
class FCheckBox;
class FDirectionalLightComponent;
class FLightComponent;
class FLightProbeComponent;
class FMaterial;
class FPropertyTable;
class FScrollBox;
class FStaticMeshComponent;
class FTextBlock;
class FVerticalBox;
struct FPropertyRow;

/** @brief Called with the vector a three-field row moved to. */
DECLARE_DELEGATE(FOnVectorChanged, const Vector3& /*NewValue*/);

/** @brief Called once a tick for the value a three-field row shows, on a row that follows the model. */
DECLARE_RETURN_DELEGATE(FOnVectorRead, Vector3);

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
    };

    void RebuildContent();
    void RequestRebuild();

    void BuildTransformSection(const TSharedPtr<FVerticalBox>& InColumn, FActor* Actor);
    void BuildStaticMeshSection(const TSharedPtr<FVerticalBox>& InColumn, FStaticMeshComponent* Component);
    void BuildLightSection(const TSharedPtr<FVerticalBox>& InColumn, FLightComponent* Component);
    void BuildCameraSection(const TSharedPtr<FVerticalBox>& InColumn, FCameraComponent* Component);
    void BuildLightProbeSection(const TSharedPtr<FVerticalBox>& InColumn, FLightProbeComponent* Component);

    void AddMaterialRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddMaterialTextureRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddMaterialFlagRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddParallaxRows(const TSharedPtr<FPropertyTable>& Table, FMaterial* Material);
    void AddLightDirectionRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component);
    void AddCascadeRows(const TSharedPtr<FPropertyTable>& Table, FDirectionalLightComponent* Component);

    void RefreshValues();
    void WriteVectorRow(int32 RowIndex);

    FPropertyRow& AddVectorRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     Label,
        const Vector3&                    Value,
        float                             Step,
        const FOnVectorChanged&           OnChanged,
        bool                              bIsAngular   = false,
        const Vector3*                    DefaultValue = nullptr,
        const FOnVectorRead&              OnRead       = FOnVectorRead());

    FPropertyRow& AddFloatRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     Label,
        float                             Value,
        float                             MinValue,
        float                             MaxValue,
        float                             Step,
        float                             DefaultValue,
        const TDelegate<void(float)>&     OnChanged);

    FPropertyRow& AddBoolRow(
        const TSharedPtr<FPropertyTable>& Table,
        const String&                     Label,
        bool                              bValue,
        const TDelegate<void(bool)>&      OnChanged,
        const bool*                       DefaultValue = nullptr,
        bool                              bIsEnabled   = true);

    NODISCARD TSharedPtr<FVisualElement> MakeRevertableRow(const TSharedPtr<FVisualElement>& Editor, const TDelegate<void()>& OnRevert);
    NODISCARD TSharedPtr<TNumericEntry<float>> MakeFloatEditor(float Value, float Min, float Max, float Step, const TDelegate<void(float)>& OnChanged);
    NODISCARD TSharedPtr<TNumericEntry<int32>> MakeIntEditor(int32 Value, int32 Min, int32 Max, const TDelegate<void(int32)>& OnChanged);
    NODISCARD TSharedPtr<FCheckBox> MakeBoolEditor(bool bValue, const TDelegate<void(bool)>& OnChanged);
    NODISCARD TSharedPtr<FVisualElement> MakeComboEditor(const TArray<String>& Options, int32 SelectedIndex, const TDelegate<void(int32)>& OnChanged);
    NODISCARD TSharedPtr<FTextBlock> MakeTextRow(const String& Text);

    TSharedPtr<FScrollBox>   ScrollBox;
    TSharedPtr<FVerticalBox> Column;
    TSharedPtr<FTextBlock>   LightDirectionText;
    TArray<FVectorRow>       VectorRows;
    FActor*                  BuiltForActor;
    int32                    SelectedMaterialIndex;
    bool                     bRebuildRequested;
};
