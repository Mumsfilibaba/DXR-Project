#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/ModelCreateInfo.h"
#include "Engine/Assets/IModelImporter.h"

struct ENGINE_API FOBJImporter : public IModelImporter
{
    virtual ~FOBJImporter() = default;

    virtual bool ImportFromFile(const StringView& Filename, EMeshImportFlags Flags, FModelCreateInfo& OutModelInfo) override final;
    virtual bool MatchExtenstion(const StringView& FileName) override final;
};
