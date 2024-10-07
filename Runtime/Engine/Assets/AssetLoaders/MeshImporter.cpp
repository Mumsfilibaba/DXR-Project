#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Project/ProjectManager.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetLoaders/MeshImporter.h"
#include "Engine/Assets/AssetLoaders/FBXLoader.h"
#include "Engine/Assets/AssetLoaders/OBJLoader.h"

static TAutoConsoleVariable<bool> CVarEnableMeshCache(
    "Engine.EnableMeshCache",
    "Enable mesh-cache",
    false,
    EConsoleVariableFlags::Default);

FMeshImporter::FMeshImporter()
    : Cache()
{
    LoadCacheFile();
}

FMeshImporter::~FMeshImporter()
{
    UpdateCacheFile();
}

TSharedRef<FSceneData> FMeshImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags Flags)
{
    const FString Filename = FString(InFilename);
    
    const bool bEnableCache = CVarEnableMeshCache.GetValue();
    if (bEnableCache)
    {
        if (FString* MeshName = Cache.Find(Filename))
        {
            TSharedRef<FSceneData> ExistingMesh = LoadCustom(*MeshName);
            if (ExistingMesh)
            {
                return ExistingMesh;
            }

            Cache.Remove(Filename);
            UpdateCacheFile();
        }
    }

    if (Filename.EndsWith(".fbx", EStringCaseType::NoCase))
    {
        EFBXFlags FBXFlags = EFBXFlags::None;
        if ((Flags & EMeshImportFlags::ApplyScaleFactor) != EMeshImportFlags::None)
        {
            FBXFlags |= EFBXFlags::ApplyScaleFactor;
        }
        
        if ((Flags & EMeshImportFlags::EnsureLeftHanded) != EMeshImportFlags::None)
        {
            FBXFlags |= EFBXFlags::ForceLeftHanded;
        }

        TSharedRef<FSceneData> Mesh = FFBXLoader::LoadFile(Filename, FBXFlags);
        if (Mesh)
        {
            if (!bEnableCache)
            {
                return Mesh;
            }
            
            const int32 Count = FMath::Max<int32>(Filename.Size() - 4, 0);
            FString NewFileName = Filename.SubString(0, Count);
            NewFileName += ".dxrmesh";
            
            if (AddCacheEntry(Filename, NewFileName, Mesh))
            {
                return Mesh;
            }
        }
    }
    else if (Filename.EndsWith(".obj", EStringCaseType::NoCase))
    {
        const bool bReverseHandedness = ((Flags & EMeshImportFlags::Default) == EMeshImportFlags::None);
        
        TSharedRef<FSceneData> Mesh = FOBJLoader::LoadFile(Filename, bReverseHandedness);
        if (Mesh)
        {
            if (!bEnableCache)
            {
                return Mesh;
            }
            
            const int32 Count = FMath::Max<int32>(Filename.Size() - 4, 0);
            FString NewFileName = Filename.SubString(0, Count);
            NewFileName += ".dxrmesh";
            
            if (AddCacheEntry(Filename, NewFileName, Mesh))
            {
                return Mesh;
            }
        }
    }

    return nullptr;
}

bool FMeshImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(".fbx", EStringCaseType::NoCase) || FileName.EndsWith(".obj", EStringCaseType::NoCase);
}

void FMeshImporter::LoadCacheFile()
{
    TArray<CHAR> FileContents;
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(FString(FProjectManager::Get().GetAssetPath()) + "/MeshCache.txt");
        if (!File)
        {
            return;
        }

        // Read the full file
        if (!FFileHelpers::ReadTextFile(File.Get(), FileContents))
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
        FParse::ParseLine(&Start);

        // End string at the end of line
        if (*Start == '\n')
        {
            *(Start++) = '\0';
        }

        // Skip any spaces at the beginning of the line
        FParse::ParseWhiteSpace(&LineStart);
        
        if (CHAR* EqualSign = FCString::Strchr(LineStart, '='))
        {
            *EqualSign = '\0';

            CHAR* KeyEnd = EqualSign - 1;
            while (*KeyEnd == ' ')
                *(KeyEnd--) = '\0';

            // The parsed key
            CHAR* Key = LineStart;
            LineStart = EqualSign + 1;
            
            FParse::ParseWhiteSpace(&LineStart);

            // Find the end of the value
            CHAR* Value    = LineStart;
            CHAR* ValueEnd = FCString::Strchr(LineStart, ' ');

            // Use line-end as backup
            if (!ValueEnd)
            {
                ValueEnd = Start;
            }

            while (*ValueEnd == ' ')
                *(ValueEnd--) = '\0';

            FString OriginalValue = Key;
            FString NewValue      = Value;
            Cache.Emplace(Move(OriginalValue), Move(NewValue));
        }
    }
}

void FMeshImporter::UpdateCacheFile()
{
    FString FileContents;
    for (auto Entry : Cache)
    {
        FileContents.AppendFormat("%s = %s\n", Entry.First.GetCString(), Entry.Second.GetCString());
    }

    {
        FFileHandleRef File = FPlatformFile::OpenForWrite(FString(FProjectManager::Get().GetAssetPath()) + "/MeshCache.txt");
        if (!File)
        {
            return;
        }

        // Write the full file
        if (!FFileHelpers::WriteTextFile(File.Get(), FileContents))
        {
            return;
        }
    } 
}

bool FMeshImporter::AddCacheEntry(const FString& OriginalFile, const FString& NewFile, const TSharedRef<FSceneData>& Scene)
{
    FCustomScene SceneHeader;
    SceneHeader.NumModels    = Scene->Models.Size();
    SceneHeader.NumMaterials = Scene->Materials.Size();

    TArray<FCustomModel> Models;
    Models.Resize(SceneHeader.NumModels);

    int64 NumTotalVertices = 0;
    int64 NumTotalIndices  = 0;
    for (int32 ModelIndex = 0; ModelIndex < SceneHeader.NumModels; ++ModelIndex)
    {
        FCustomModel& CurrentModel = Models[ModelIndex];
        FMemory::Memzero(CurrentModel.Name, FCustomModel::MaxNameLength);
        
        const FModelData& SceneModel = Scene->Models[ModelIndex];
        SceneModel.Name.CopyToBuffer(CurrentModel.Name, FCustomModel::MaxNameLength);
        
        if (FCString::Strlen(CurrentModel.Name) == 0)
        {
            LOG_WARNING("Model has no name");
        }
        
        CurrentModel.MaterialIndex = SceneModel.MaterialIndex;
        CurrentModel.NumVertices   = SceneModel.Mesh.GetVertexCount();
        CurrentModel.NumIndices    = SceneModel.Mesh.GetIndexCount();

        NumTotalVertices += CurrentModel.NumVertices;
        NumTotalIndices  += CurrentModel.NumIndices;
    }

    SceneHeader.NumTotalVertices = NumTotalVertices;
    SceneHeader.NumTotalIndices  = NumTotalIndices;

    TArray<FVertex> SceneVertices;
    SceneVertices.Reserve(int32(NumTotalVertices));

    TArray<uint32> SceneIndicies;
    SceneIndicies.Reserve(int32(NumTotalIndices));

    for (const FModelData& Model : Scene->Models)
    {
        SceneVertices.Append(Model.Mesh.Vertices);
        SceneIndicies.Append(Model.Mesh.Indices);
    }

    int32 NumTextures = 0;
    for (const FMaterialData& Material : Scene->Materials)
    {
        if (Material.DiffuseTexture)
            NumTextures++;
        if (Material.NormalTexture)
            NumTextures++;
        if (Material.SpecularTexture)
            NumTextures++;
        if (Material.EmissiveTexture)
            NumTextures++;
        if (Material.AOTexture)
            NumTextures++;
        if (Material.RoughnessTexture)
            NumTextures++;
        if (Material.MetallicTexture)
            NumTextures++;
        if (Material.AlphaMaskTexture)
            NumTextures++;
    }

    SceneHeader.NumTextures = NumTextures;

    TArray<FTextureHeader> TextureNames;
    TextureNames.Resize(NumTextures);

    // Zero all names
    FMemory::Memzero(TextureNames.Data(), TextureNames.SizeInBytes());

    TArray<FCustomMaterial> Materials;
    Materials.Resize(SceneHeader.NumMaterials);

    int32 CurrentTexture = 0;
    for (int32 Index = 0; Index < SceneHeader.NumMaterials; ++Index)
    {
        FCustomMaterial& CustomMaterial = Materials[Index];
        FMemory::Memzero(&CustomMaterial, sizeof(FCustomMaterial));

        const FMaterialData& CurrentMaterial = Scene->Materials[Index];
        if (CurrentMaterial.DiffuseTexture)
        {
            CurrentMaterial.DiffuseTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.DiffuseTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.DiffuseTexIndex = -1;
        }

        if (CurrentMaterial.NormalTexture)
        {
            CurrentMaterial.NormalTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.NormalTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.NormalTexIndex = -1;
        }

        if (CurrentMaterial.SpecularTexture)
        {
            CurrentMaterial.SpecularTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.SpecularTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.SpecularTexIndex = -1;
        }

        if (CurrentMaterial.EmissiveTexture)
        {
            CurrentMaterial.EmissiveTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.EmissiveTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.EmissiveTexIndex = -1;
        }

        if (CurrentMaterial.AOTexture)
        {
            CurrentMaterial.AOTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.AOTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.AOTexIndex = -1;
        }

        if (CurrentMaterial.RoughnessTexture)
        {
            CurrentMaterial.RoughnessTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.RoughnessTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.RoughnessTexIndex = -1;
        }

        if (CurrentMaterial.MetallicTexture)
        {
            CurrentMaterial.MetallicTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.MetallicTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.MetallicTexIndex = -1;
        }

        if (CurrentMaterial.AlphaMaskTexture)
        {
            CurrentMaterial.AlphaMaskTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            CustomMaterial.AlphaMaskTexIndex = CurrentTexture++;
        }
        else
        {
            CustomMaterial.AlphaMaskTexIndex = -1;
        }

        CustomMaterial.Diffuse       = CurrentMaterial.Diffuse;
        CustomMaterial.AO            = CurrentMaterial.AO;
        CustomMaterial.Roughness     = CurrentMaterial.Roughness;
        CustomMaterial.Metallic      = CurrentMaterial.Metallic;
        CustomMaterial.MaterialFlags = CurrentMaterial.MaterialFlags;
    }

    {
        FFileHandleRef File = FPlatformFile::OpenForWrite(NewFile);
        if (!File)
        {
            return false;
        }

        // 1) Header
        File->Write(reinterpret_cast<const uint8*>(&SceneHeader), sizeof(FCustomScene));
        
        // 2) Model-Headers
        CHECK(SceneHeader.NumModels > 0);
        File->Write(reinterpret_cast<const uint8*>(Models.Data()), Models.SizeInBytes());
        
        // 3) All the vertices
        CHECK(SceneHeader.NumTotalVertices > 0);
        File->Write(reinterpret_cast<const uint8*>(SceneVertices.Data()), SceneVertices.SizeInBytes());
        
        // 4) All the indices
        CHECK(SceneHeader.NumTotalIndices > 0);
        File->Write(reinterpret_cast<const uint8*>(SceneIndicies.Data()), SceneIndicies.SizeInBytes());
        
        // 5) All Textures
        if (SceneHeader.NumTextures)
        {
            File->Write(reinterpret_cast<const uint8*>(TextureNames.Data()), TextureNames.SizeInBytes());
        }
        
        // 6) All Materials
        if (SceneHeader.NumMaterials)
        {
            File->Write(reinterpret_cast<const uint8*>(Materials.Data()), Materials.SizeInBytes());
        }
    }

    Cache.Add(OriginalFile, NewFile);
    UpdateCacheFile();
    return true;
}

TSharedRef<FSceneData> FMeshImporter::LoadCustom(const FString& InFilename)
{
    TArray<uint8> FileContents;
    
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(InFilename);
        if (!File)
        {
            return nullptr;
        }

        // Read the full file
        if (!FFileHelpers::ReadFile(File.Get(), FileContents))
        {
            return nullptr;
        }
    }

    // 1) Scene Header
    FCustomScene* SceneHeader = reinterpret_cast<FCustomScene*>(FileContents.Data());
    CHECK(SceneHeader->NumModels <= FCustomScene::MaxModels);
    CHECK(SceneHeader->NumMaterials <= FCustomScene::MaxMaterials);

    // 2) Model Headers
    FCustomModel* ModelHeaders = reinterpret_cast<FCustomModel*>(SceneHeader + 1);
    
    // 3) Vertices
    FVertex* Vertices = reinterpret_cast<FVertex*>(ModelHeaders + SceneHeader->NumModels);
    
    // 4) Indices
    uint32* Indices = reinterpret_cast<uint32*>(Vertices + SceneHeader->NumTotalVertices);
    
    // 5) Indices
    FTextureHeader* Textures = reinterpret_cast<FTextureHeader*>(Indices + SceneHeader->NumTotalIndices);
    
    // 6) Materials
    FCustomMaterial* Materials = reinterpret_cast<FCustomMaterial*>(Textures + SceneHeader->NumTextures);

    // Load all textures
    TArray<FTextureResourceRef> LoadedTextures;
    LoadedTextures.Resize(SceneHeader->NumTextures);

    for (int32 Index = 0; Index < SceneHeader->NumTextures; ++Index)
    {
        const FString Filename = Textures[Index].Filepath;
        if (!FPlatformFile::IsFile(Filename.GetCString()))
        {
            LOG_ERROR("Stored file contains a invalid file reference, file will be reloaded from source");
            return nullptr;
        }

        LoadedTextures[Index] = FAssetManager::Get().LoadTexture(Filename);
    }

    // Reconstruct the data
    TSharedRef<FSceneData> Scene = new FSceneData();
    Scene->Models.Resize(SceneHeader->NumModels);
    
    for (int32 Index = 0; Index < SceneHeader->NumModels; ++Index)
    {
        FModelData& CurrentModel = Scene->Models[Index];
        
        MAYBE_UNUSED const int32 Length = FCString::Strlen(ModelHeaders[Index].Name);
        CHECK(Length < FCustomModel::MaxNameLength);

        CurrentModel.Name          = ModelHeaders[Index].Name;
        CurrentModel.MaterialIndex = ModelHeaders[Index].MaterialIndex;

        LOG_INFO("Loaded Mesh '%s'", ModelHeaders[Index].Name);

        const int32 NumVertices = ModelHeaders[Index].NumVertices;
        CurrentModel.Mesh.Vertices.Resize(NumVertices);
        FMemory::Memcpy(CurrentModel.Mesh.Vertices.Data(), Vertices, NumVertices * sizeof(FVertex));
        Vertices += NumVertices;

        const int32 NumIndices = ModelHeaders[Index].NumIndices;
        CurrentModel.Mesh.Indices.Resize(NumIndices);
        FMemory::Memcpy(CurrentModel.Mesh.Indices.Data(), Indices, NumIndices * sizeof(uint32));
        Indices += NumIndices;

        CurrentModel.MaterialIndex = ModelHeaders[Index].MaterialIndex;
    }

    Scene->Materials.Resize(SceneHeader->NumMaterials);
    
    for (int32 Index = 0; Index < SceneHeader->NumMaterials; ++Index)
    {
        FMaterialData& Material = Scene->Materials[Index];
        if (Materials[Index].DiffuseTexIndex >= 0)
        {
            Material.DiffuseTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].DiffuseTexIndex]->GetTexture2D());
        }
        if (Materials[Index].NormalTexIndex >= 0)
        {
            Material.NormalTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].NormalTexIndex]->GetTexture2D());
        }
        if (Materials[Index].SpecularTexIndex >= 0)
        {
            Material.SpecularTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].SpecularTexIndex]->GetTexture2D());
        }
        if (Materials[Index].RoughnessTexIndex >= 0)
        {
            Material.RoughnessTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].RoughnessTexIndex]->GetTexture2D());
        }
        if (Materials[Index].AOTexIndex >= 0)
        {
            Material.AOTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].AOTexIndex]->GetTexture2D());
        }
        if (Materials[Index].MetallicTexIndex >= 0)
        {
            Material.MetallicTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].MetallicTexIndex]->GetTexture2D());
        }
        if (Materials[Index].EmissiveTexIndex >= 0)
        {
            Material.EmissiveTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].EmissiveTexIndex]->GetTexture2D());
        }
        if (Materials[Index].AlphaMaskTexIndex >= 0)
        {
            Material.AlphaMaskTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].AlphaMaskTexIndex]->GetTexture2D());
        }

        Material.MaterialFlags = static_cast<EMaterialFlags>(Materials[Index].MaterialFlags);
        Material.Diffuse       = Materials[Index].Diffuse;
        Material.Roughness     = Materials[Index].Roughness;
        Material.AO            = Materials[Index].AO;
        Material.Metallic      = Materials[Index].Metallic;
    }

    return Scene;
}
