#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/Resources/Material.h"

class FActor;
class FEditorEngine;

class FEditorPropertiesWidget
{
public:
    FEditorPropertiesWidget(FEditorEngine* InEditorEngine);
    ~FEditorPropertiesWidget();

    void Draw();
    void DrawWindowContents();

    bool IsVisible() const
    {
        return bVisible;
    }

    void SetVisible(bool bInVisible)
    {
        bVisible = bInVisible;
    }

private:
    ImTextureID GetTexturePreview(EMaterialTextureSlot::Type Slot, const FRHITextureRef& Texture);

    FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
    const FActor*   MaterialSelectionOwner;
    int32           SelectedMaterialIndex;
    FImGuiTexture   MaterialTexturePreviews[EMaterialTextureSlot::Count];
};
