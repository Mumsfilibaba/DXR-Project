#include "MeshImporter.h"
#include "FBXLoader.h"
#include "OBJLoader.h"

#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceLogger.h"

#include "Engine/Assets/AssetManager.h"
#include "Engine/Project/ProjectManager.h"

FMeshImporter* FMeshImporter::GInstance = nullptr;

FMeshImporter::FMeshImporter()
    : Cache()
{}

bool FMeshImporter::Initialize()
{
    CHECK(GInstance == nullptr);
    GInstance = new FMeshImporter();
    CHECK(GInstance != nullptr);

    GInstance->LoadCacheFile();
    return true;
}

void FMeshImporter::Release()
{
    CHECK(GInstance != nullptr);
    GInstance->UpdateCacheFile();
    delete GInstance;
    GInstance = nullptr;
}

bool FMeshImporter::LoadMesh(const FString& Filename, FSceneData& OutScene, EMeshImportFlags Flags)
{
    auto CachedMesh = Cache.find(Filename);
    if (CachedMesh != Cache.end())
    {
        if (LoadCustom(CachedMesh->second, OutScene))
        {
            return true;
        }

        Cache.erase(CachedMesh);
        UpdateCacheFile();
    }

    if (Filename.EndsWithNoCase(".fbx"))
    {
        EFBXFlags FBXFlags = EFBXFlags::None;
        if ((Flags & EMeshImportFlags::ApplyScaleFactor) != EMeshImportFlags::None)
            FBXFlags |= EFBXFlags::ApplyScaleFactor;
        if ((Flags & EMeshImportFlags::EnsureLeftHanded) != EMeshImportFlags::None)
            FBXFlags |= EFBXFlags::EnsureLeftHanded;

        if (FFBXLoader::LoadFile(Filename, OutScene, FBXFlags))
        {
            const auto Count = NMath::Max<int32>(Filename.GetSize() - 4, 0);

            FString NewFileName = Filename.SubString(0, Count);
            NewFileName += ".dxrmesh";
            return AddCacheEntry(Filename, NewFileName, OutScene);
        }
    }
    else if (Filename.EndsWithNoCase(".obj"))
    {
        const bool bReverseHandedness = ((Flags & EMeshImportFlags::Default) == EMeshImportFlags::None);
        if (FOBJLoader::LoadFile(Filename, OutScene, bReverseHandedness))
        {
            const auto Count = NMath::Max<int32>(Filename.GetSize() - 4, 0);

            FString NewFileName = Filename.SubString(0, Count);
            NewFileName += ".dxrmesh";
            return AddCacheEntry(Filename, NewFileName, OutScene);
        }
    }

    return false;
}

void FMeshImporter::LoadCacheFile()
{
    TArray<CHAR> FileContents;
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(FString(FProjectManager::GetAssetPath()) + "/MeshCache.txt");
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

    CHAR* Start = FileContents.GetData();
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

            const FString OriginalValue = Key;
            const FString NewValue = Value;
            Cache.emplace(std::make_pair(OriginalValue, NewValue));
        }
    }
}

void FMeshImporter::UpdateCacheFile()
{
    FString FileContents;
    for (const auto& Entry : Cache)
    {
        FileContents.AppendFormat("%s = %s\n", Entry.first.GetCString(), Entry.second.GetCString());
    }

    {
        FFileHandleRef File = FPlatformFile::OpenForWrite(FString(FProjectManager::GetAssetPath()) + "/MeshCache.txt");
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

bool FMeshImporter::AddCacheEntry(const FString& OriginalFile, const FString& NewFile, const FSceneData& Scene)
{
    FCustomScene SceneHeader;
    SceneHeader.NumModels    = Scene.Models.GetSize();
    SceneHeader.NumMaterials = Scene.Materials.GetSize();

    TArray<FCustomModel> Models;
    Models.Resize(SceneHeader.NumModels);

    int64 NumTotalVertices = 0;
    int64 NumTotalIndices  = 0;
    for (int32 ModelIndex = 0; ModelIndex < SceneHeader.NumModels; ++ModelIndex)
    {
        FMemory::Memzero(Models[ModelIndex].Name, FCustomModel::MaxNameLength);
        Scene.Models[ModelIndex].Name.CopyToBuffer(Models[ModelIndex].Name, FCustomModel::MaxNameLength);
        
        Models[ModelIndex].MaterialIndex = Scene.Models[ModelIndex].MaterialIndex;
        Models[ModelIndex].NumVertices   = Scene.Models[ModelIndex].Mesh.GetVertexCount();
        Models[ModelIndex].NumIndices    = Scene.Models[ModelIndex].Mesh.GetIndexCount();

        NumTotalVertices += Models[ModelIndex].NumVertices;
        NumTotalIndices  += Models[ModelIndex].NumIndices;
    }

    SceneHeader.NumTotalVertices = NumTotalVertices;
    SceneHeader.NumTotalIndices  = NumTotalIndices;

    TArray<FVertex> SceneVertices;
    SceneVertices.Reserve(int32(NumTotalVertices));

    TArray<uint32> SceneIndicies;
    SceneIndicies.Reserve(int32(NumTotalIndices));

    for (const auto& Model : Scene.Models)
    {
        SceneVertices.Append(Model.Mesh.Vertices);
        SceneIndicies.Append(Model.Mesh.Indices);
    }

    int32 NumTextures = 0;
    for (const auto& Material : Scene.Materials)
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
    FMemory::Memzero(TextureNames.GetData(), TextureNames.SizeInBytes());

    TArray<FCustomMaterial> Materials;
    Materials.Resize(SceneHeader.NumMaterials);

    int32 CurrentTexture = 0;
    for (int32 Index = 0; Index < SceneHeader.NumMaterials; ++Index)
    {
        const auto& CurrentMaterial = Scene.Materials[Index];
        if (CurrentMaterial.DiffuseTexture)
        {
            CurrentMaterial.DiffuseTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].DiffuseTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].DiffuseTexIndex = -1;
        }

        if (CurrentMaterial.NormalTexture)
        {
            CurrentMaterial.NormalTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].NormalTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].NormalTexIndex = -1;
        }

        if (CurrentMaterial.SpecularTexture)
        {
            CurrentMaterial.SpecularTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].SpecularTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].SpecularTexIndex = -1;
        }

        if (CurrentMaterial.EmissiveTexture)
        {
            CurrentMaterial.EmissiveTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].EmissiveTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].EmissiveTexIndex = -1;
        }

        if (CurrentMaterial.AOTexture)
        {
            CurrentMaterial.AOTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].AOTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].AOTexIndex = -1;
        }

        if (CurrentMaterial.RoughnessTexture)
        {
            CurrentMaterial.RoughnessTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].RoughnessTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].RoughnessTexIndex = -1;
        }

        if (CurrentMaterial.MetallicTexture)
        {
            CurrentMaterial.MetallicTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].MetallicTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].MetallicTexIndex = -1;
        }

        if (CurrentMaterial.AlphaMaskTexture)
        {
            CurrentMaterial.AlphaMaskTexture->GetFilename().CopyToBuffer(TextureNames[CurrentTexture].Filepath, FTextureHeader::MaxNameLength);
            Materials[Index].AlphaMaskTexIndex = CurrentTexture++;
        }
        else
        {
            Materials[Index].AlphaMaskTexIndex = -1;
        }


        Materials[Index].Diffuse   = CurrentMaterial.Diffuse;
        Materials[Index].AO        = CurrentMaterial.AO;
        Materials[Index].Roughness = CurrentMaterial.Roughness;
        Materials[Index].Metallic  = CurrentMaterial.Metallic;

        Materials[Index].bAlphaDiffuseCombined = CurrentMaterial.bAlphaDiffuseCombined;
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
        File->Write(reinterpret_cast<const uint8*>(Models.GetData()), Models.SizeInBytes());
        // 3) All the vertices
        File->Write(reinterpret_cast<const uint8*>(SceneVertices.GetData()), SceneVertices.SizeInBytes());
        // 4) All the indices
        File->Write(reinterpret_cast<const uint8*>(SceneIndicies.GetData()), SceneIndicies.SizeInBytes());
        // 5) All Textures
        File->Write(reinterpret_cast<const uint8*>(TextureNames.GetData()), TextureNames.SizeInBytes());
        // 6) All Materials
        File->Write(reinterpret_cast<const uint8*>(Materials.GetData()), Materials.SizeInBytes());
    }

    Cache.emplace(std::make_pair(OriginalFile, NewFile));

    UpdateCacheFile();
    return true;
}

bool FMeshImporter::LoadCustom(const FString& InFilename, FSceneData& OutScene)
{
    TArray<uint8> FileContents;
    {
        FFileHandleRef File = FPlatformFile::OpenForRead(InFilename);
        if (!File)
        {
            return false;
        }

        // Read the full file
        if (!FFileHelpers::ReadFile(File.Get(), FileContents))
        {
            return false;
        }
    }

    // 1) Scene Header
    FCustomScene* SceneHeader = reinterpret_cast<FCustomScene*>(FileContents.GetData());
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
        LoadedTextures[Index] = FAssetManager::Get().LoadTexture(Filename);
    }

    // Reconstruct the data
    OutScene.Models.Resize(SceneHeader->NumModels);
    for (int32 Index = 0; Index < SceneHeader->NumModels; ++Index)
    {
        FModelData& CurrentModel = OutScene.Models[Index];
        
        const auto Length = FCString::Strlen(ModelHeaders[Index].Name);
        CHECK(Length < FCustomModel::MaxNameLength);
        CurrentModel.Name          = ModelHeaders[Index].Name;
        CurrentModel.MaterialIndex = ModelHeaders[Index].MaterialIndex;

        LOG_INFO("Loaded Mesh '%s'", ModelHeaders[Index].Name);

        const auto NumVertices = ModelHeaders[Index].NumVertices;
        CurrentModel.Mesh.Vertices.Resize(NumVertices);
        FMemory::Memcpy(CurrentModel.Mesh.Vertices.GetData(), Vertices, NumVertices * sizeof(FVertex));
        Vertices += NumVertices;

        const auto NumIndices = ModelHeaders[Index].NumIndices;
        CurrentModel.Mesh.Indices.Resize(NumIndices);
        FMemory::Memcpy(CurrentModel.Mesh.Indices.GetData(), Indices, NumIndices * sizeof(uint32));
        Indices += NumIndices;

        CurrentModel.MaterialIndex = ModelHeaders[Index].MaterialIndex;
    }

    OutScene.Materials.Resize(SceneHeader->NumMaterials);
    for (int32 Index = 0; Index < SceneHeader->NumMaterials; ++Index)
    {
        FMaterialData& Material = OutScene.Materials[Index];
        if (Materials[Index].DiffuseTexIndex >= 0)
            Material.DiffuseTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].DiffuseTexIndex]->GetTexture2D());
        if (Materials[Index].NormalTexIndex >= 0)
            Material.NormalTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].NormalTexIndex]->GetTexture2D());
        if (Materials[Index].SpecularTexIndex >= 0)
            Material.SpecularTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].SpecularTexIndex]->GetTexture2D());
        if (Materials[Index].RoughnessTexIndex >= 0)
            Material.RoughnessTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].RoughnessTexIndex]->GetTexture2D());
        if (Materials[Index].AOTexIndex >= 0)
            Material.AOTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].AOTexIndex]->GetTexture2D());
        if (Materials[Index].MetallicTexIndex >= 0)
            Material.MetallicTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].MetallicTexIndex]->GetTexture2D());
        if (Materials[Index].EmissiveTexIndex >= 0)
            Material.EmissiveTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].EmissiveTexIndex]->GetTexture2D());
        if (Materials[Index].AlphaMaskTexIndex >= 0)
            Material.AlphaMaskTexture = MakeSharedRef<FTexture2D>(LoadedTextures[Materials[Index].AlphaMaskTexIndex]->GetTexture2D());

        Material.Diffuse   = Materials[Index].Diffuse;
        Material.Roughness = Materials[Index].Roughness;
        Material.AO        = Materials[Index].AO;
        Material.Metallic  = Materials[Index].Metallic;

        Material.bAlphaDiffuseCombined = Materials[Index].bAlphaDiffuseCombined;
    }

    return true;
}
