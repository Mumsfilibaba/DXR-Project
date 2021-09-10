#pragma once
#include "Assets/SceneData.h"

#include "Core/Containers/String.h"

class CStbImageLoader
{
public:
    static TSharedPtr<SImage2D> LoadFile( const CString& Filename );
};
