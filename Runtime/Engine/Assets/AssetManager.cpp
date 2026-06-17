#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/ScopedLock.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/Paths.h"
#include "RHI/RHICommandList.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/TextureImporterDDS.h"
#include "Engine/Assets/AssetImporters/TextureImporterBase.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

static TAutoConsoleVariable<bool> CVarEnableAssetConversion(
    "Engine.AssetManager.EnableAssetConversion",
    "Enable conversion and serialization to the engine's custom format instead of loading from source",
    true,
    EConsoleVariableFlags::Default);

static String ReplaceExtension(const String& Filename, const String& NewExtension)
{
    const int32 Position = Filename.FindLastChar('.');
    if (Position == String::InvalidIndex)
    {
        return Filename;
    }
    
    String NewFilename = Filename.SubString(0, Position);
    NewFilename += NewExtension;
    return NewFilename;
}

FAssetRegistry::FAssetRegistry()
    : RegistryMap()
    , RegistryFilename()
{
    const String AssetPath   = Paths::GetAssetDir();
    const String ProjectName = Paths::GetProjectName();
    RegistryFilename = AssetPath + "/" + ProjectName + ".assetregistry";
}

FAssetRegistry::~FAssetRegistry()
{
}

String* FAssetRegistry::FindFile(const String& SrcFilename)
{
    return RegistryMap.Find(SrcFilename);
}

void FAssetRegistry::AddEntry(const String& SrcFilename, const String& Filename)
{
    RegistryMap.Add(SrcFilename, Filename);
    UpdateRegistryFile();
}

void FAssetRegistry::RemoveEntry(const String& SrcFilename)
{
    RegistryMap.Remove(SrcFilename);
    UpdateRegistryFile();
}

void FAssetRegistry::LoadRegistryFile()
{
    TArray<CHAR> FileContents;
    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(RegistryFilename);
        if (!FileHandle)
        {
            return;
        }

        // Read the full file
        if (!File::ReadTextFile(FileHandle.Get(), FileContents))
        {
            return;
        }
    }

    CHAR* Start = FileContents.Data();
    while (Start && (*Start != '\0'))
    {
        // Skip newline chars
        while (*Start == '\n')
            ++Start;

        CHAR* LineStart = Start;
        Parse::ParseLine(&Start);

        // End string at the end of line
        if (*Start == '\n')
        {
            *(Start++) = '\0';
        }

        // Skip any spaces at the beginning of the line
        Parse::ParseWhiteSpace(&LineStart);
        
        if (CHAR* EqualSign = CString::Strchr(LineStart, '='))
        {
            *EqualSign = '\0';

            CHAR* KeyEnd = EqualSign - 1;
            while (*KeyEnd == ' ')
                *(KeyEnd--) = '\0';

            // The parsed key
            CHAR* Key = LineStart;
            LineStart = EqualSign + 1;
            
            Parse::ParseWhiteSpace(&LineStart);

            // Find the end of the value
            CHAR* Value    = LineStart;
            CHAR* ValueEnd = CString::Strchr(LineStart, ' ');

            // Use line-end as backup
            if (!ValueEnd)
            {
                ValueEnd = Start;
            }

            while (*ValueEnd == ' ')
                *(ValueEnd--) = '\0';

            String NewValue      = Value;
            String OriginalValue = Key;
            RegistryMap.Emplace(Move(OriginalValue), Move(NewValue));
        }
    }
}

void FAssetRegistry::UpdateRegistryFile()
{
    String FileContents;
    for (TMap<String, String>::IteratorType Iterator = RegistryMap.CreateIterator(); !Iterator.IsEnd(); Iterator++)
    {
        FileContents.AppendFormat("%s = %s\n", *Iterator.GetKey(), *Iterator.GetValue());
    }

    {
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(RegistryFilename);
        if (!FileHandle)
        {
            return;
        }

        // Write the full file
        if (!File::WriteTextFile(FileHandle.Get(), FileContents))
        {
            return;
        }
    }
}

FAssetManager* FAssetManager::GAssetManager = nullptr;

FAssetManager::FAssetManager()
    : AssetRegistry(nullptr)
    , ModelSerializer(nullptr)
    , ModelImporters()
    , ModelImportersCS()
    , ModelsMap()
    , Models()
    , ModelsCS()
    , TextureImporters()
    , TextureImportersCS()
    , TextureMap()
    , Textures()
    , TexturesCS()
{
    AssetRegistry = MakeUniquePtr<FAssetRegistry>();
    AssetRegistry->LoadRegistryFile();
    
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
    if (!GAssetManager)
    {
        GAssetManager = new FAssetManager();
        CHECK(GAssetManager != nullptr);
        
        // Importers for textures
        GAssetManager->RegisterTextureImporter(MakeSharedPtr<FTextureImporterDDS>());
        GAssetManager->RegisterTextureImporter(MakeSharedPtr<FTextureImporterBase>());
        
        // Importers for models
        GAssetManager->RegisterModelImporter(MakeSharedPtr<FFBXImporter>());
        GAssetManager->RegisterModelImporter(MakeSharedPtr<FOBJImporter>());
        GAssetManager->RegisterModelImporter(MakeSharedPtr<FModelImporter>());
        return true;
    }

    return false;
}

void FAssetManager::Release()
{
    if (GAssetManager)
    {
        delete GAssetManager;
        GAssetManager = nullptr;
    }
}

FAssetManager& FAssetManager::Get()
{
    CHECK(GAssetManager != nullptr);
    return *GAssetManager;
}

TSharedRef<FTexture> FAssetManager::LoadTexture(const String& Filename, bool bGenerateMips)
{
    SCOPED_LOCK(TexturesCS);

    // Convert backslashes
    String FinalPath = Filename;
    FinalPath.ReplaceAll('\\', '/');
    
    if (int32* TextureID = TextureMap.Find(FinalPath))
    {
        const int32 TextureIndex = *TextureID;
        return Textures[TextureIndex];
    }

    TSharedRef<FTexture> NewTexture;
    
    {
        SCOPED_LOCK(TextureImportersCS);
        
        for (TSharedPtr<ITextureImporter> Importer : TextureImporters)
        {
            const StringView FileNameView(FinalPath);
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
    const int32 Index = Textures.Size();
    Textures.Emplace(NewTexture);
    TextureMap.Add(FinalPath, Index);
    return NewTexture;
}

TSharedRef<FModel> FAssetManager::LoadModel(const String& Filename, EMeshImportFlags Flags)
{
    SCOPED_LOCK(ModelsCS);

    // Convert backslashes
    String FinalPath = Filename;
    FinalPath.ReplaceAll('\\', '/');

    if (int32* MeshID = ModelsMap.Find(FinalPath))
    {
        const int32 MeshIndex = *MeshID;
        return Models[MeshIndex];
    }

    // Insert the a new model into the AssetManager
    const auto InsertModel = [this](const String& Filename, const FModelCreateInfo& InCreateInfo)
    {
        TSharedRef<FModel> NewModel = new FModel();
        if (!NewModel->Init(InCreateInfo))
        {
            return TSharedRef<FModel>(nullptr);
        }

        const int32 Index = Models.Size();
        Models.Emplace(NewModel);
        ModelsMap.Add(Filename, Index);
        return NewModel;
    };

    TSharedRef<FModel> NewModel;
    FModelCreateInfo NewCreateInfo;

    // If we have enabled serialization then look up the cached model
    const bool bEnableAssetConversion = CVarEnableAssetConversion.GetValue();
    if (bEnableAssetConversion)
    {
        if (String* ExistingPath = AssetRegistry->FindFile(FinalPath))
        {
            const StringView FileNameView(*ExistingPath);

            if (ModelImporter->ImportFromFile(FileNameView, Flags, NewCreateInfo))
            {
                NewModel = InsertModel(FinalPath, NewCreateInfo);
            }

            if (NewModel)
            {
                LOG_INFO("[FAssetManager]: Loaded Mesh '%s'", *FinalPath);
                return NewModel;
            }
            else
            {
                AssetRegistry->RemoveEntry(FinalPath);
            }
        }
    }

    // If we did not load custom format, reload source file
    bool bResult = false;

    {
        SCOPED_LOCK(ModelImportersCS);

        for (TSharedPtr<IModelImporter> Importer : ModelImporters)
        {
            const StringView FileNameView(FinalPath);
            if (Importer->MatchExtenstion(FileNameView))
            {
                bResult = Importer->ImportFromFile(FileNameView, Flags, NewCreateInfo);
                break;
            }
        }
    }

    if (bResult)
    {
        NewModel = InsertModel(FinalPath, NewCreateInfo);
    }

    if (!NewModel)
    {
        LOG_ERROR("[FAssetManager]: Unsupported mesh format. Failed to load '%s'.", *FinalPath);
        return nullptr;
    }

    if (bEnableAssetConversion)
    {
        const String NewFilename = ReplaceExtension(FinalPath, ".dxrmesh");
        if (ModelSerializer->Serialize(NewFilename, NewCreateInfo))
        {
            AssetRegistry->AddEntry(FinalPath, NewFilename);
        }
    }

    LOG_INFO("[FAssetManager]: Loaded Mesh '%s'", *FinalPath);
    return NewModel;
}

void FAssetManager::UnloadModel(const TSharedRef<FModel>& InModel)
{
    SCOPED_LOCK(ModelsCS);
        
    Models.Remove(InModel);
    // TODO: Remove mesh from map
}

void FAssetManager::UnloadTexture(const FTextureRef& Texture)
{
    SCOPED_LOCK(TexturesCS);
    
    const String& Filename = Texture->GetFilename();
    TextureMap.Remove(Filename);
    Textures.Remove(Texture);
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
