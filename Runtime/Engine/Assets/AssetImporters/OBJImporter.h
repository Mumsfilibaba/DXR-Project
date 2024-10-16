#pragma once
#include "Core/Core.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/Assets/SceneData.h"
#include "Engine/Assets/IModelImporter.h"

struct ENGINE_API FOBJImporter : public IModelImporter
{
    virtual ~FOBJImporter() = default;

    virtual TSharedPtr<FImportedModel> ImportFromFile(const FStringView& InFileName, EMeshImportFlags Flags) override final;
    virtual bool MatchExtenstion(const FStringView& FileName) override final;
};
