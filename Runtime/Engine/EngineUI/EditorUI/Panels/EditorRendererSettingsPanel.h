#pragma once
#include "Core/Containers/Array.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FPropertyTable;
class FScrollBox;
class FSearchBox;
class FVerticalBox;
class IConsoleVariable;

enum class ERendererSettingKind : uint8
{
    Bool,
    Int,
    Float,

    /** @brief An index into the option list, stored in the variable as it stands. */
    Combo,

    /** @brief An index into the option list, stored in the variable as the matching entry of OptionValues. */
    ComboValues,
};

struct FRendererSubsection
{
    /** @brief The name of the nested section, which matches the Section of the settings it holds. */
    const CHAR* Section;

    /** @brief The name of the section it hangs inside. */
    const CHAR* ParentSection;
};

struct FRendererSetting
{
    const CHAR*          Section;
    const CHAR*          Label;
    const CHAR*          CVarName;
    ERendererSettingKind Kind;
    float                MinValue;
    float                MaxValue;
    float                Step;
    const CHAR* const*   Options;
    const int32*         OptionValues;
    int32                NumOptions;
};

class ENGINE_API FEditorRendererSettingsPanel final : public FEditorPanel
{
public:
    FEditorRendererSettingsPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorRendererSettingsPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;

private:
    struct FSettingRow
    {
        const FRendererSetting*    Setting;
        IConsoleVariable*          Variable;
        TSharedPtr<FVisualElement> Editor;
    };

    void RebuildSections();
    void RefreshValues();
    void OnSearchTextChanged(const String& SearchText);

    NODISCARD TSharedPtr<FVisualElement> BuildEditor(const FRendererSetting& Setting, IConsoleVariable* Variable);
    NODISCARD bool PassesFilter(const FRendererSetting& Setting) const;

    TSharedPtr<FScrollBox>   ScrollBox;
    TSharedPtr<FVerticalBox> Column;
    TSharedPtr<FSearchBox>   SearchBox;
    TArray<FSettingRow>      Rows;
    String                   FilterText;
};
