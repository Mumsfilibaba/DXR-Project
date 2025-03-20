#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/IModelImporter.h"

struct ENGINE_API FFBXImporter : public IModelImporter
{
    virtual ~FFBXImporter() = default;

    virtual bool ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags, FModelCreateInfo& OutModelInfo) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
