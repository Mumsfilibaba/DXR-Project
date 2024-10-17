#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"

class FModelCreateInfo;

enum class EMeshImportFlags : uint8
{
    None             = 0,
    ApplyScaleFactor = BIT(1),
    ForceLeftHanded  = BIT(2),
    
    Default = ForceLeftHanded
};

ENUM_CLASS_OPERATORS(EMeshImportFlags);

struct IModelImporter
{
    virtual ~IModelImporter() = default;

    virtual bool ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags, FModelCreateInfo& OutModelInfo) = 0;
    virtual bool MatchExtenstion(const FStringView& FileName) = 0;
};
