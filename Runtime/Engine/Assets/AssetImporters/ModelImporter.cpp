#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "Core/Containers/Stream.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/CRC.h"
#include "Project/ProjectManager.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

TSharedPtr<FImportedModel> FModelImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags)
{
    FByteInputStream InputStream;
    
    {
        const FString Filename = FString(InFilename);
        FFileHandleRef File = FPlatformFile::OpenForRead(Filename);
        if (!File)
        {
            return nullptr;
        }

        // Read the full file
        if (!FFileHelpers::ReadFile(File.Get(), InputStream))
        {
            return nullptr;
        }
    }

    // 1) Read file-header
    ModelFormat::FFileHeader FileHeader;
    InputStream.Read(FileHeader);
    
    if (FMemory::Memcmp(FileHeader.Magic, "DXRMESH", sizeof(FileHeader.Magic)) != 0)
    {
        return nullptr;
    }
    
    if (FileHeader.VersionMajor != MODEL_FORMAT_VERSION_MAJOR || FileHeader.VersionMinor != MODEL_FORMAT_VERSION_MINOR)
    {
        return nullptr;
    }
    
    const uint64 DataSize = InputStream.Size() - sizeof(ModelFormat::FFileHeader);
    if (FileHeader.DataSize != DataSize)
    {
        return nullptr;
    }
    
    const uint64 DataCRC = FCRC32::Generate(InputStream.PeekData(), DataSize);
    if (FileHeader.DataCRC != DataCRC)
    {
        return nullptr;
    }
    
    // 2) Model Header
    const ModelFormat::FModelHeader* ModelHeader = InputStream.PeekData<ModelFormat::FModelHeader>();

    // Load MeshData
    TSharedPtr<FImportedModel> ImportedModel = MakeSharedPtr<FImportedModel>();
    ImportedModel->Models.Resize(ModelHeader->NumMeshes);

    // 3) Mesh Headers
    const ModelFormat::FMeshInfo*    MeshHeaders = InputStream.PeekData<ModelFormat::FMeshInfo>(ModelHeader->MeshDataOffset);
    const ModelFormat::FSubMeshInfo* SubMeshData = InputStream.PeekData<ModelFormat::FSubMeshInfo>(ModelHeader->SubMeshDataOffset);

    // 4) Geometry Buffers
    const FVertex* VertexData = InputStream.PeekData<FVertex>(ModelHeader->VertexDataOffset);
    const uint32*  IndexData  = InputStream.PeekData<uint32>(ModelHeader->IndexDataOffset);
    
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader->NumMeshes; ++MeshIdx)
    {
        FMeshData& CurrentMesh = ImportedModel->Models[MeshIdx];
        
        const ModelFormat::FMeshInfo& MeshHeader = MeshHeaders[MeshIdx];
        MAYBE_UNUSED const int32 Length = FCString::Strlen(MeshHeader.Name);
        CHECK(Length < MODEL_FORMAT_MAX_NAME_LENGTH);

        CurrentMesh.Name = MeshHeader.Name;
        LOG_INFO("Loaded Mesh '%s'", MeshHeader.Name);

        const ModelFormat::FSubMeshInfo* SubMeshes = SubMeshData + MeshHeader.FirstSubMesh;
        CurrentMesh.Partitions.Reserve(MeshHeader.NumSubMeshes);
        
        const FVertex* Vertices = VertexData + MeshHeader.FirstVertex;
        CurrentMesh.Vertices.Reset(Vertices, MeshHeader.NumVertices);
        
        const uint32* Indices = IndexData + MeshHeader.FirstIndex;
        CurrentMesh.Indices.Reset(Indices, MeshHeader.NumIndices);

        for (int32 SubMeshIdx = 0; SubMeshIdx < MeshHeader.NumSubMeshes; SubMeshIdx++)
        {
            FMeshPartition& MeshPartition = CurrentMesh.Partitions.Emplace();
            MeshPartition.BaseVertex    = SubMeshes[SubMeshIdx].BaseVertex;
            MeshPartition.VertexCount   = SubMeshes[SubMeshIdx].NumVertices;
            MeshPartition.StartIndex    = SubMeshes[SubMeshIdx].StartIndex;
            MeshPartition.IndexCount    = SubMeshes[SubMeshIdx].NumIndicies;
            MeshPartition.MaterialIndex = SubMeshes[SubMeshIdx].MaterialIndex;
        }
    }

    // Load Textures
    TArray<FTextureRef> LoadedTextures;
    LoadedTextures.Resize(ModelHeader->NumTextures);

    // 5) Textures
    const ModelFormat::FTextureInfo* Textures = InputStream.PeekData<ModelFormat::FTextureInfo>(ModelHeader->TextureDataOffset);
    for (int32 TextureIdx = 0; TextureIdx < ModelHeader->NumTextures; ++TextureIdx)
    {
        const FStringView FilenameView = Textures[TextureIdx].Filepath;
        if (!FPlatformFile::IsFile(FilenameView.GetCString()))
        {
            LOG_ERROR("[FModelImporter] Stored file contains a invalid file reference, file will be reloaded from source");
            return nullptr;
        }

        const FString Filename = FString(FilenameView);
        LoadedTextures[TextureIdx] = FAssetManager::Get().LoadTexture(Filename);
        if (!LoadedTextures[TextureIdx])
        {
            LOG_ERROR("[FModelImporter] Failed to load texture '%s'", Filename.GetCString());
            return nullptr;
        }
    }
    
    // Retrieve a texture from the loaded texture and ensure it is valid
    const auto RetrieveTexture = [&](int32 TextureIdx)
    {
        if (TextureIdx < 0 || TextureIdx >= LoadedTextures.Size())
        {
            return FTexture2DRef(nullptr);
        }
        
        if (const FTextureRef& Texture = LoadedTextures[TextureIdx])
        {
            return MakeSharedRef<FTexture2D>(Texture->GetTexture2D());
        }
        
        return FTexture2DRef(nullptr);
    };

    // Construct Materials
    ImportedModel->Materials.Resize(ModelHeader->NumMaterials);

    // 6) Materials
    const ModelFormat::FMaterialInfo* Materials = InputStream.PeekData<ModelFormat::FMaterialInfo>(ModelHeader->MaterialDataOffset);
    for (int32 Index = 0; Index < ModelHeader->NumMaterials; ++Index)
    {
        FImportedMaterial& Material = ImportedModel->Materials[Index];
        Material.DiffuseTexture   = RetrieveTexture(Materials[Index].DiffuseTextureIdx);
        Material.NormalTexture    = RetrieveTexture(Materials[Index].NormalTextureIdx);
        Material.SpecularTexture  = RetrieveTexture(Materials[Index].SpecularTextureIdx);
        Material.RoughnessTexture = RetrieveTexture(Materials[Index].RoughnessTextureIdx);
        Material.AOTexture        = RetrieveTexture(Materials[Index].AmbientOcclusionTextureIdx);
        Material.MetallicTexture  = RetrieveTexture(Materials[Index].MetallicTextureIdx);
        Material.EmissiveTexture  = RetrieveTexture(Materials[Index].EmissiveTextureIdx);
        Material.AlphaMaskTexture = RetrieveTexture(Materials[Index].AlphaMaskTextureIdx);
        Material.MaterialFlags    = static_cast<EMaterialFlags>(Materials[Index].MaterialFlags);
        Material.Diffuse          = Materials[Index].Diffuse;
        Material.Roughness        = Materials[Index].Roughness;
        Material.AO               = Materials[Index].AO;
        Material.Metallic         = Materials[Index].Metallic;
    }

    return ImportedModel;
}

bool FModelImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(".dxrmesh", EStringCaseType::NoCase);
}

bool FModelSerializer::Serialize(const FString& Filename, const TSharedPtr<FImportedModel>& Model)
{
    if (!Model)
    {
        return false;
    }

    ModelFormat::FModelHeader ModelHeader;
    FMemory::Memzero(&ModelHeader, sizeof(ModelFormat::FModelHeader));

    FByteOutputStream OutputStream;
    OutputStream.AddUninitialized<ModelFormat::FModelHeader>();
        
    // Count mesh-primitives and prepare headers
    ModelHeader.NumMeshes      = Model->Models.Size();
    ModelHeader.MeshDataOffset = OutputStream.AddUninitialized<ModelFormat::FMeshInfo>(ModelHeader.NumMeshes);
    
    int32 NumVertices  = 0;
    int32 NumIndicies  = 0;
    int32 NumSubMeshes = 0;
    
    int32 MeshDataOffset = ModelHeader.MeshDataOffset;
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader.NumMeshes; ++MeshIdx)
    {
        const FMeshData& CurrentMesh = Model->Models[MeshIdx];
        
        ModelFormat::FMeshInfo Header;
        FMemory::Memzero(&Header, sizeof(ModelFormat::FMeshInfo));
        
        Header.FirstVertex  = NumVertices;
        Header.NumVertices  = CurrentMesh.GetVertexCount();
        Header.FirstIndex   = NumIndicies;
        Header.NumIndices   = CurrentMesh.GetIndexCount();
        Header.FirstSubMesh = NumSubMeshes;
        Header.NumSubMeshes = CurrentMesh.GetSubMeshCount();
        
        FCString::Strncpy(Header.Name, CurrentMesh.Name.GetCString(), MODEL_FORMAT_MAX_NAME_LENGTH);
        MeshDataOffset += OutputStream.Write(Header, MeshDataOffset);
        
        NumVertices  += Header.NumVertices;
        NumIndicies  += Header.NumIndices;
        NumSubMeshes += Header.NumSubMeshes;
    }

    // Initialize the mesh-primitives
    ModelHeader.VertexDataOffset  = OutputStream.AddUninitialized<FVertex>(NumVertices);
    ModelHeader.NumVertices       = NumVertices;
    ModelHeader.IndexDataOffset   = OutputStream.AddUninitialized<uint32>(NumIndicies);
    ModelHeader.NumIndicies       = NumIndicies;
    ModelHeader.SubMeshDataOffset = OutputStream.AddUninitialized<ModelFormat::FSubMeshInfo>(NumSubMeshes);
    ModelHeader.NumSubMeshes      = NumSubMeshes;

    int32 VertexDataOffset  = ModelHeader.VertexDataOffset;
    int32 IndexDataOffset   = ModelHeader.IndexDataOffset;
    int32 SubMeshDataOffset = ModelHeader.SubMeshDataOffset;
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader.NumMeshes; ++MeshIdx)
    {
        const FMeshData& CurrentMesh = Model->Models[MeshIdx];
        VertexDataOffset += OutputStream.Write(CurrentMesh.Vertices.Data(), CurrentMesh.Vertices.Size(), VertexDataOffset);
        IndexDataOffset  += OutputStream.Write(CurrentMesh.Indices.Data(), CurrentMesh.Indices.Size(), IndexDataOffset);
        
        for (int32 SubMeshIdx = 0; SubMeshIdx < CurrentMesh.GetSubMeshCount(); SubMeshIdx++)
        {
            const FMeshPartition& MeshPartitions = CurrentMesh.Partitions[SubMeshIdx];
            
            // SubMesh vertices and indices are based on the mesh and not the model
            ModelFormat::FSubMeshInfo SubMesh;
            SubMesh.BaseVertex    = MeshPartitions.BaseVertex;
            SubMesh.NumVertices   = MeshPartitions.VertexCount;
            SubMesh.StartIndex    = MeshPartitions.StartIndex;
            SubMesh.NumIndicies   = MeshPartitions.IndexCount;
            SubMesh.MaterialIndex = MeshPartitions.MaterialIndex;
            SubMeshDataOffset += OutputStream.Write(SubMesh, SubMeshDataOffset);
        }
    }
    
    // Prepare material primitives
    ModelHeader.NumMaterials       = Model->Materials.Size();
    ModelHeader.MaterialDataOffset = OutputStream.AddUninitialized<ModelFormat::FMaterialInfo>(ModelHeader.NumMaterials);
    
    // Create a new TextureIndex
    ModelHeader.TextureDataOffset = OutputStream.WriteOffset();
    ModelHeader.NumTextures       = 0;

    const auto CreateTextureIndex = [&](const FTexture2DRef& Texture)
    {
        if (Texture)
        {
            ModelFormat::FTextureInfo TextureHeader;
            FMemory::Memzero(&TextureHeader, sizeof(ModelFormat::FTextureInfo));
            
            Texture->GetFilename().CopyToBuffer(TextureHeader.Filepath, MODEL_FORMAT_MAX_NAME_LENGTH);
            OutputStream.Add(TextureHeader);
            
            return ModelHeader.NumTextures++;
        }
        else
        {
            return MODEL_FORMAT_INVALID_TEXTURE_ID;
        }
    };

    // Serialize MaterialData
    int32 MaterialDataOffset = ModelHeader.MaterialDataOffset;
    for (int32 MaterialIdx = 0; MaterialIdx < ModelHeader.NumMaterials; MaterialIdx++)
    {
        const FImportedMaterial& CurrentMaterial = Model->Materials[MaterialIdx];

        ModelFormat::FMaterialInfo Material;
        FMemory::Memzero(&Material, sizeof(ModelFormat::FMaterialInfo));

        Material.DiffuseTextureIdx          = CreateTextureIndex(CurrentMaterial.DiffuseTexture);
        Material.NormalTextureIdx           = CreateTextureIndex(CurrentMaterial.NormalTexture);
        Material.SpecularTextureIdx         = CreateTextureIndex(CurrentMaterial.SpecularTexture);
        Material.EmissiveTextureIdx         = CreateTextureIndex(CurrentMaterial.EmissiveTexture);
        Material.AmbientOcclusionTextureIdx = CreateTextureIndex(CurrentMaterial.AOTexture);
        Material.RoughnessTextureIdx        = CreateTextureIndex(CurrentMaterial.RoughnessTexture);
        Material.MetallicTextureIdx         = CreateTextureIndex(CurrentMaterial.MetallicTexture);
        Material.AlphaMaskTextureIdx        = CreateTextureIndex(CurrentMaterial.AlphaMaskTexture);
        Material.Diffuse                    = CurrentMaterial.Diffuse;
        Material.AO                         = CurrentMaterial.AO;
        Material.Roughness                  = CurrentMaterial.Roughness;
        Material.Metallic                   = CurrentMaterial.Metallic;
        Material.MaterialFlags              = static_cast<int32>(CurrentMaterial.MaterialFlags);
        
        MaterialDataOffset += OutputStream.Write(Material, MaterialDataOffset);
    }
   
    OutputStream.Write(ModelHeader, 0);
    
    {
        FFileHandleRef File = FPlatformFile::OpenForWrite(Filename);
        if (!File)
        {
            return false;
        }

        ModelFormat::FFileHeader FileHeader;
        FMemory::Memzero(&FileHeader, sizeof(ModelFormat::FFileHeader));
        
        FMemory::Memcpy(FileHeader.Magic, "DXRMESH", sizeof(FileHeader.Magic));
        FileHeader.DataCRC      = FCRC32::Generate(OutputStream.Data(), OutputStream.Size());
        FileHeader.DataSize     = OutputStream.Size();
        FileHeader.VersionMajor = MODEL_FORMAT_VERSION_MAJOR;
        FileHeader.VersionMinor = MODEL_FORMAT_VERSION_MINOR;
        
        // 1) FileHeader
        File->Write(reinterpret_cast<const uint8*>(&FileHeader), sizeof(ModelFormat::FFileHeader));
        
        // 2) Data
        File->Write(OutputStream.Data(), OutputStream.Size());
    }

    return true;
}
