#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/ImportScene.h"
#include "Engine/Assets/IModelImporter.h"

struct ENGINE_API FOBJImporter : public IModelImporter
{
    virtual ~FOBJImporter() = default;

    virtual bool ImportFromFile(const FStringView& Filename, EMeshImportFlags Flags, FImportScene& OutScene) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
