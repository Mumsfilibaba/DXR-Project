#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CStbImageLoader

class ENGINE_API CStbImageLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile(const FString& Filename);
};
