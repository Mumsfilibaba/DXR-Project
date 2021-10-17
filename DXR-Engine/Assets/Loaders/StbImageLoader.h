#pragma once
#include "Core/CoreAPI.h"
#include "Core/Containers/String.h"

#include "Assets/SceneData.h"

class CORE_API CStbImageLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const CString& Filename );
};
