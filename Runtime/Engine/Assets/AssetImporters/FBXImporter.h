#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/IModelImporter.h"

enum class EFBXFlags : uint8
{
    None             = 0,
    ApplyScaleFactor = BIT(1),
    ForceLeftHanded  = BIT(2),

    Default = ForceLeftHanded
};

ENUM_CLASS_OPERATORS(EFBXFlags);

struct ENGINE_API FFBXImporter : public IModelImporter
{
    virtual ~FFBXImporter() = default;

    virtual TSharedPtr<FImportedModel> ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
