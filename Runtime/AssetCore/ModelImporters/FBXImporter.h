#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "AssetCore/IModelImporter.h"
#include "AssetCore/ImportScene.h"

struct ASSETCORE_API FFBXImporter : public IModelImporter
{
    virtual ~FFBXImporter() = default;

    virtual bool ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags, FImportScene& OutScene) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
