#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"

struct FModel;

enum class EMeshImportFlags : uint8
{
    None             = 0,
    ApplyScaleFactor = BIT(1),
    EnsureLeftHanded = BIT(2),
    
    Default          = EnsureLeftHanded
};

ENUM_CLASS_OPERATORS(EMeshImportFlags);

struct IModelImporter
{
    virtual ~IModelImporter() = default;

    virtual TSharedRef<FModel> ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags) = 0;

    /** @Return: Returns true if the FileName matches extension for this importer */
    virtual bool MatchExtenstion(const FStringView& FileName) = 0;
};
