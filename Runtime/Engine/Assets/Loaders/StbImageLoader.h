#pragma once
#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

#include "Engine/Assets/SceneData.h"

class ENGINE_API CStbImageLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const CString& Filename );
};
