#pragma once
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FPropertyTable;

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

    void BuildRows();

    TSharedPtr<FPropertyTable> Table;
};
