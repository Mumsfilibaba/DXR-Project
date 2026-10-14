#pragma once
#include "Core/Math/Vector3.h"
#include "Engine/Assets/PrimitiveShapes.h"

class FActor;
class FEditorEngine;

struct ENGINE_API FEditorPrimitiveShapeDialog
{
    /** @return Returns the values a shape was placed with last, kept per shape in the project's User/EditorSettings.ini, or its defaults */
    NODISCARD static FPrimitiveShape LoadRemembered(EPrimitiveShape Type);

    static void SaveRemembered(const FPrimitiveShape& Shape);

    /**
     * @brief Place a shape at Location and select it. With Shift held it takes the values used last; otherwise a dialog
     * opens whose values change the placed shape live, and Place keeps it while Cancel removes it.
     *
     * @return Returns the placed actor, or nullptr when it couldn't be created
     */
    static FActor* Place(FEditorEngine* EditorEngine, EPrimitiveShape Type, const FVector3& Location);
};
