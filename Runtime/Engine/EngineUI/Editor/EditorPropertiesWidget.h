#pragma once
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "Engine/Resources/Material.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"

class FActor;
class FEditorEngine;
class FStaticMeshComponent;

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
    const TArray<const CHAR*>& GetMaterialLabels(const FStaticMeshComponent* MeshComponent, int32 NumMaterials);

    FEditorEngine*              EditorEngine;
    FDelegateHandle             ImGuiDelegateHandle;
    bool                        bVisible;
    const FActor*               MaterialSelectionOwner;
    int32                       SelectedMaterialIndex;
    FImGuiTexture               MaterialTexturePreviews[EMaterialTextureSlot::Count];
    TArray<String>              MaterialLabels;
    TArray<const CHAR*>         MaterialLabelText;
    const FStaticMeshComponent* MaterialLabelOwner;
    int32                       MaterialLabelCount;
};
