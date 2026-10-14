#pragma once
#include "Core/Containers/Function.h"
#include "Engine/Assets/ModelData.h"

struct ENGINE_API ImportSceneBaking
{
    /**
     * @brief Bake the scene into the FModelData LoadModel uses: a mesh for each node that has one, from ImportScenes::BakeNodeMesh
     * @param LoadTexture Loads a scene texture's source file, or returns nullptr to leave it out
     */
    static bool BakeToModelData(const FImportScene& Scene, FModelData& OutModelData, TFunction<TSharedPtr<FTexture2D>(const FString&)> LoadTexture);
};
