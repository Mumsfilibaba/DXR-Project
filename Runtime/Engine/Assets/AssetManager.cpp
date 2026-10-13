#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "RHI/RHICommandList.h"
#include "AssetCore/TextureAssetFactory.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/GPUTextureBlockCompressor.h"
#include "Engine/Assets/MaterialAsset.h"
#include "Engine/Assets/TextureAssetLoader.h"
#include "Engine/Assets/AssetImporters/TextureImporterDDS.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

static TAutoConsoleVariable<bool> CVarEnableAssetConversion(
    TEXT("Engine.AssetManager.EnableAssetConversion"),
    TEXT("Enable conversion and serialization to the engine's custom format instead of loading from source"),
    true,
    EConsoleVariableFlags::Default);

static FString ReplaceExtension(const FString& Filename, const FString& NewExtension)
{
    const int32 Position = Filename.FindLastChar('.');
    if (Position == FString::InvalidIndex)
    {
        return Filename;
    }
    
    FString NewFilename = Filename.SubString(0, Position);
    NewFilename += NewExtension;
    return NewFilename;
}

FAssetManager* FAssetManager::AssetManager = nullptr;

FAssetManager::FAssetManager()
    : AssetRegistry()
    , AssetLoader(AssetRegistry)
    , AssetImporter(FContentMounts::CreateDefault())
    , ModelSerializer(nullptr)
    , ModelImporters()
    , ModelImportersCS()
    , Models()
    , ModelsCS()
    , TextureImporters()
    , TextureImportersCS()
    , Textures()
    , TexturesCS()
{
    ModelImporter   = MakeUniquePtr<FModelImporter>();
    ModelSerializer = MakeUniquePtr<FModelSerializer>();
}

FAssetManager::~FAssetManager()
{
    {
        SCOPED_LOCK(TextureImportersCS);
        TextureImporters.Clear();
    }

    {
        SCOPED_LOCK(ModelImportersCS);
        ModelImporters.Clear();
    }
}

bool FAssetManager::Initialize()
{
    if (!AssetManager)
    {
        AssetManager = new FAssetManager();
        CHECK(AssetManager != nullptr);
        
        // Importers for textures
        AssetManager->RegisterTextureImporter(MakeSharedPtr<FTextureImporterDDS>());
        AssetManager->RegisterTextureImporter(MakeSharedPtr<FTextureImporterBase>());
        
        // Importers for models
        AssetManager->RegisterModelImporter(MakeSharedPtr<FFBXImporter>());
        AssetManager->RegisterModelImporter(MakeSharedPtr<FOBJImporter>());
        AssetManager->RegisterModelImporter(MakeSharedPtr<FModelImporter>());

        AssetManager->AssetImporter.RegisterFactory(MakeSharedPtr<FTextureAssetFactory>(MakeSharedPtr<FGPUTextureBlockCompressor>()));
        AssetManager->AssetLoader.RegisterLoader<FTexture>(&TextureAssetLoader::Load);
        AssetManager->AssetLoader.RegisterLoader<FMaterial>(&MaterialAssets::Load);

        FAssetRegistry& Registry = AssetManager->AssetRegistry;
        Registry.SetMounts(FContentMounts::CreateDefault());

        const FString PrecomputedFile = File::CombinePath(Paths::GetProjectContentDir(), FAssetRegistry::PrecomputedFilename);
        FString       RegistryError;

        if (!Paths::IsPackaged() || !FPlatformFile::IsFile(*PrecomputedFile))
        {
            Registry.Rescan();
        }
        else if (Registry.LoadFromFile(PrecomputedFile, RegistryError))
        {
            LOG_INFO("[FAssetManager]: Loaded %d assets from '%s'", Registry.GetNumAssets(), *PrecomputedFile);
        }
        else
        {
            LOG_WARNING("[FAssetManager]: %s, so the content folders are scanned instead", *RegistryError);
            Registry.Rescan();
        }

        return true;
    }

    return false;
}

void FAssetManager::Release()
{
    if (AssetManager)
    {
        delete AssetManager;
        AssetManager = nullptr;
    }
}

FAssetManager& FAssetManager::Get()
{
    CHECK(AssetManager != nullptr);
    return *AssetManager;
}

TSharedPtr<FTexture> FAssetManager::LoadTexture(const FString& Filename, bool bGenerateMips)
{
    SCOPED_LOCK(TexturesCS);

    // Convert backslashes
    FString FinalPath = Filename;
    FinalPath.ReplaceAll('\\', '/');
    
    if (TSharedPtr<FTexture>* ExistingTexture = Textures.Find(FinalPath))
    {
        return *ExistingTexture;
    }

    TSharedPtr<FTexture> NewTexture;
    
    {
        SCOPED_LOCK(TextureImportersCS);
        
        for (TSharedPtr<ITextureImporter> Importer : TextureImporters)
        {
            const FStringView FileNameView(FinalPath);
            if (Importer->MatchExtenstion(FileNameView))
            {
                NewTexture = Importer->ImportFromFile(FileNameView);
                break;
            }
        }
    }

    if (!NewTexture)
    {
        LOG_ERROR("[FAssetManager]: Unsupported texture format. Failed to load '%s'.", *FinalPath);
        return nullptr;
    }

    if (IsBlockCompressed(NewTexture->GetFormat()))
    {
        bGenerateMips = false;
    }

    if (!NewTexture->CreateRHITexture(bGenerateMips))
    {
        LOG_ERROR("[FAssetManager]: Failed to create RHI texture for image '%s'.", *FinalPath);
        return nullptr;
    }

    LOG_INFO("[FAssetManager]: Loaded Texture '%s'", *FinalPath);

    // Set name 
    NewTexture->SetDebugName(FinalPath); // For the RHI
    NewTexture->SetFilename(FinalPath);  // For the AssetSystem

    // Release the data after the texture is loaded
    NewTexture->ReleaseData();

    // Insert the new texture
    Textures.Add(FinalPath, NewTexture);
    return NewTexture;
}

TSharedPtr<FModel> FAssetManager::LoadModel(const FString& Filename, EMeshImportFlags Flags)
{
    SCOPED_LOCK(ModelsCS);

    // Convert backslashes
    FString FinalPath = Filename;
    FinalPath.ReplaceAll('\\', '/');

    if (TSharedPtr<FModel>* ExistingModel = Models.Find(FinalPath))
    {
        return *ExistingModel;
    }

    // Insert the a new model into the AssetManager
    const auto InsertModel = [this](const FString& Filename, const FModelData& InModelData)
    {
        TSharedPtr<FModel> NewModel = FModel::Create(InModelData);
        if (!NewModel)
        {
            return TSharedPtr<FModel>(nullptr);
        }

        Models.Add(Filename, NewModel);
        return NewModel;
    };

    TSharedPtr<FModel> NewModel;
    FModelData NewModelData;

    const bool    bEnableAssetConversion = CVarEnableAssetConversion.GetValue();
    const FString ConvertedPath          = ReplaceExtension(FinalPath, TEXT(".dxrmesh"));

    if (bEnableAssetConversion && FPlatformFile::IsFile(*ConvertedPath))
    {
        const FStringView FileNameView(ConvertedPath);
        if (ModelImporter->ImportFromFile(FileNameView, Flags, NewModelData))
        {
            NewModel = InsertModel(FinalPath, NewModelData);
        }

        if (NewModel)
        {
            LOG_INFO("[FAssetManager]: Loaded Mesh '%s'", *FinalPath);
            return NewModel;
        }
    }

    // If we did not load custom format, reload source file
    bool bResult = false;

    {
        SCOPED_LOCK(ModelImportersCS);

        for (TSharedPtr<IModelImporter> Importer : ModelImporters)
        {
            const FStringView FileNameView(FinalPath);
            if (Importer->MatchExtenstion(FileNameView))
            {
                bResult = Importer->ImportFromFile(FileNameView, Flags, NewModelData);
                break;
            }
        }
    }

    if (bResult)
    {
        NewModel = InsertModel(FinalPath, NewModelData);
    }

    if (!NewModel)
    {
        LOG_ERROR("[FAssetManager]: Unsupported mesh format. Failed to load '%s'.", *FinalPath);
        return nullptr;
    }

    if (bEnableAssetConversion)
    {
        ModelSerializer->Serialize(ConvertedPath, NewModelData);
    }

    LOG_INFO("[FAssetManager]: Loaded Mesh '%s'", *FinalPath);
    return NewModel;
}

void FAssetManager::UnloadModel(const TSharedPtr<FModel>& InModel)
{
    SCOPED_LOCK(ModelsCS);

    FString ModelPath;
    bool    bFound = false;

    Models.Foreach([&](const FString& Path, const TSharedPtr<FModel>& Model)
    {
        if (Model == InModel)
        {
            ModelPath = Path;
            bFound    = true;
        }
    });

    if (bFound)
    {
        Models.Remove(ModelPath);
    }
}

void FAssetManager::UnloadTexture(const TSharedPtr<FTexture>& Texture)
{
    SCOPED_LOCK(TexturesCS);
    Textures.Remove(Texture->GetFilename());
}

void FAssetManager::RegisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter)
{
    SCOPED_LOCK(TextureImportersCS);
    TextureImporters.AddUnique(InImporter);
}

void FAssetManager::UnregisterTextureImporter(const TSharedPtr<ITextureImporter>& InImporter)
{
    SCOPED_LOCK(TextureImportersCS);
    TextureImporters.Remove(InImporter);
}

void FAssetManager::RegisterModelImporter(const TSharedPtr<IModelImporter>& InImporter)
{
    SCOPED_LOCK(ModelImportersCS);
    ModelImporters.AddUnique(InImporter);
}

void FAssetManager::UnregisterModelImporter(const TSharedPtr<IModelImporter>& InImporter)
{
    SCOPED_LOCK(ModelImportersCS);
    ModelImporters.Remove(InImporter);
}
