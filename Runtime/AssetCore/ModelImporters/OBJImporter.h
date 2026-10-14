#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "AssetCore/ImportScene.h"
#include "AssetCore/IModelImporter.h"

struct ASSETCORE_API FOBJImporter : public IModelImporter
{
    virtual ~FOBJImporter() = default;

    virtual bool ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags, FImportScene& OutScene) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
