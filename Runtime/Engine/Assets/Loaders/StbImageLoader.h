#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FSTBImageLoader

class ENGINE_API FSTBImageLoader
{
public:
    static FImage2DPtr LoadFile(const FString& Filename);
};
