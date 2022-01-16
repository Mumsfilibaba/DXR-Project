#pragma once
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

class ENGINE_API CDDSLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile(const CString& Filename);
};
