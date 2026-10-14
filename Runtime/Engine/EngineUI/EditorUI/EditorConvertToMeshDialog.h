#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Engine/EngineModule.h"

class FEditorEngine;
class FPrimitiveShapeComponent;
class FVisualElement;

struct ENGINE_API FEditorConvertToMeshDialog
{
    /**
     * @brief Write a shape component's current shape as a mesh asset named Name in the virtual folder Folder, such as
     * "/Project/Meshes", and replace the component with a static mesh component that uses it and the same materials
     *
     * @return Returns false with OutError saying why, leaving the component as it was
     */
    static bool Convert(FPrimitiveShapeComponent* Component, const FString& Folder, const FString& Name, FString& OutError);

    /** @brief Open a dialog asking for the folder and name, centered on the window AnchorElement belongs to, that converts on confirm */
    static bool Open(const TSharedPtr<FVisualElement>& AnchorElement, FEditorEngine* EditorEngine, FPrimitiveShapeComponent* Component);
};
