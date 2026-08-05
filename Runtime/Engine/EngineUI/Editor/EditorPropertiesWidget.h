#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"

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
    ImTextureID GetTexturePreview(int32 Slot, const FRHITextureRef& Texture);

    enum : int32
    {
        MaterialTextureSlot_Albedo = 0,
        MaterialTextureSlot_Normal,
        MaterialTextureSlot_Height,
        MaterialTextureSlot_Material,
        MaterialTextureSlot_Count,
    };

    FEditorEngine*  EditorEngine;
    FDelegateHandle ImGuiDelegateHandle;
    bool            bVisible;
    const FActor*   MaterialSelectionOwner;
    int32           SelectedMaterialIndex;
    FImGuiTexture   MaterialTexturePreviews[MaterialTextureSlot_Count];
};
