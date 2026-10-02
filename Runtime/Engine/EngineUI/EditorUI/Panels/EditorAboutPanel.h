#pragma once
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Core/Containers/Array.h"

class FPropertyTable;
class FVerticalBox;

class ENGINE_API FEditorAboutPanel final : public FEditorPanel
{
public:
    FEditorAboutPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorAboutPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;

private:
    NODISCARD static TSharedPtr<FVisualElement> CreateValueText(const String& Text);

    /**
     * @brief Adds a collapsible section to the column and returns the table inside it, so each group of
     * facts reads as a card the way it does in the stats and settings panels.
     *
     * @param Column      The column the section is appended to.
     * @param SectionName The title the section header shows.
     * @return The table the section's rows go into.
     */
    NODISCARD TSharedPtr<FPropertyTable> AddSection(const TSharedPtr<FVerticalBox>& Column, const String& SectionName);

    void BuildSections(const TSharedPtr<FVerticalBox>& Column);
    void CopyRow(const TSharedPtr<FPropertyTable>& FromTable, int32 RowIndex);

    TArray<TSharedPtr<FPropertyTable>> Tables;
};
