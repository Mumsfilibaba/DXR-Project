#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/SceneData.h"

struct ENGINE_API FOBJLoader
{
    static TSharedPtr<FImportedModel> LoadFile(const FString& Filename, bool bReverseHandedness = false);
};
