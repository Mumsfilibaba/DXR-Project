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

TSharedPtr<FModelCreateInfo> FModelImporter::ImportFromFile(const FStringView& InFilename, EMeshImportFlags)
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
    TSharedPtr<FModelCreateInfo> ModelCreateInfo = MakeSharedPtr<FModelCreateInfo>();
    ModelCreateInfo->Meshes.Resize(ModelHeader->NumMeshes);

    // 3) Mesh Headers
    const ModelFormat::FMeshInfo*    MeshHeaders = InputStream.PeekData<ModelFormat::FMeshInfo>(ModelHeader->MeshDataOffset);
    const ModelFormat::FSubMeshInfo* SubMeshData = InputStream.PeekData<ModelFormat::FSubMeshInfo>(ModelHeader->SubMeshDataOffset);

    // 4) Geometry Buffers
    const FVertex* VertexData = InputStream.PeekData<FVertex>(ModelHeader->VertexDataOffset);
    const uint32*  IndexData  = InputStream.PeekData<uint32>(ModelHeader->IndexDataOffset);

    for (int32 MeshIdx = 0; MeshIdx < ModelHeader->NumMeshes; ++MeshIdx)
    {
        FMeshCreateInfo& MeshCreateInfo = ModelCreateInfo->Meshes[MeshIdx];

        const ModelFormat::FMeshInfo& MeshHeader = MeshHeaders[MeshIdx];
        MAYBE_UNUSED const int32 Length = FCString::Strlen(MeshHeader.Name);
        CHECK(Length < MODEL_FORMAT_MAX_NAME_LENGTH);

        MeshCreateInfo.Name = MeshHeader.Name;
        LOG_INFO("Loaded Mesh '%s'", MeshHeader.Name);

        const ModelFormat::FSubMeshInfo* SubMeshes = SubMeshData + MeshHeader.FirstSubMesh;
        MeshCreateInfo.SubMeshes.Resize(MeshHeader.NumSubMeshes);

        const FVertex* Vertices = VertexData + MeshHeader.FirstVertex;
        MeshCreateInfo.Vertices.Reset(Vertices, MeshHeader.NumVertices);

        const uint32* Indices = IndexData + MeshHeader.FirstIndex;
        MeshCreateInfo.Indices.Reset(Indices, MeshHeader.NumIndices);

        for (int32 SubMeshIdx = 0; SubMeshIdx < MeshHeader.NumSubMeshes; SubMeshIdx++)
        {
            FSubMeshInfo& SubMeshInfo = MeshCreateInfo.SubMeshes[SubMeshIdx];
            SubMeshInfo.BaseVertex    = SubMeshes[SubMeshIdx].BaseVertex;
            SubMeshInfo.VertexCount   = SubMeshes[SubMeshIdx].NumVertices;
            SubMeshInfo.StartIndex    = SubMeshes[SubMeshIdx].StartIndex;
            SubMeshInfo.IndexCount    = SubMeshes[SubMeshIdx].NumIndicies;
            SubMeshInfo.MaterialIndex = SubMeshes[SubMeshIdx].MaterialIndex;
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
    ModelCreateInfo->Materials.Resize(ModelHeader->NumMaterials);

    // 6) Materials
    const ModelFormat::FMaterialInfo* Materials = InputStream.PeekData<ModelFormat::FMaterialInfo>(ModelHeader->MaterialDataOffset);
    for (int32 Index = 0; Index < ModelHeader->NumMaterials; ++Index)
    {
        FMaterialCreateInfo& MaterialCreateInfo = ModelCreateInfo->Materials[Index];
        MaterialCreateInfo.Textures[EMaterialTexture::Diffuse]          = RetrieveTexture(Materials[Index].DiffuseTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::Normal]           = RetrieveTexture(Materials[Index].NormalTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::Specular]         = RetrieveTexture(Materials[Index].SpecularTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::Roughness]        = RetrieveTexture(Materials[Index].RoughnessTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::AmbientOcclusion] = RetrieveTexture(Materials[Index].AmbientOcclusionTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::Metallic]         = RetrieveTexture(Materials[Index].MetallicTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::Emissive]         = RetrieveTexture(Materials[Index].EmissiveTextureIdx);
        MaterialCreateInfo.Textures[EMaterialTexture::AlphaMask]        = RetrieveTexture(Materials[Index].AlphaMaskTextureIdx);
        
        MaterialCreateInfo.MaterialFlags = static_cast<EMaterialFlags>(Materials[Index].MaterialFlags);
        MaterialCreateInfo.Diffuse       = Materials[Index].Diffuse;
        MaterialCreateInfo.Roughness     = Materials[Index].Roughness;
        MaterialCreateInfo.AmbientFactor = Materials[Index].AO;
        MaterialCreateInfo.Metallic      = Materials[Index].Metallic;
    }

    return ModelCreateInfo;
}

bool FModelImporter::MatchExtenstion(const FStringView& FileName)
{
    return FileName.EndsWith(".dxrmesh", EStringCaseType::NoCase);
}

bool FModelSerializer::Serialize(const FString& Filename, const TSharedPtr<FModelCreateInfo>& Model)
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
    ModelHeader.NumMeshes      = Model->Meshes.Size();
    ModelHeader.MeshDataOffset = OutputStream.AddUninitialized<ModelFormat::FMeshInfo>(ModelHeader.NumMeshes);

    int32 NumVertices  = 0;
    int32 NumIndicies  = 0;
    int32 NumSubMeshes = 0;

    int32 MeshDataOffset = ModelHeader.MeshDataOffset;
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader.NumMeshes; ++MeshIdx)
    {
        const FMeshCreateInfo& MeshCreateInfo = Model->Meshes[MeshIdx];

        ModelFormat::FMeshInfo Header;
        FMemory::Memzero(&Header, sizeof(ModelFormat::FMeshInfo));

        Header.FirstVertex  = NumVertices;
        Header.NumVertices  = MeshCreateInfo.Vertices.Size();
        Header.FirstIndex   = NumIndicies;
        Header.NumIndices   = MeshCreateInfo.Indices.Size();
        Header.FirstSubMesh = NumSubMeshes;
        Header.NumSubMeshes = MeshCreateInfo.SubMeshes.Size();

        FCString::Strncpy(Header.Name, MeshCreateInfo.Name.GetCString(), MODEL_FORMAT_MAX_NAME_LENGTH);
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
        const FMeshCreateInfo& MeshCreateInfo = Model->Meshes[MeshIdx];
        VertexDataOffset += OutputStream.Write(MeshCreateInfo.Vertices.Data(), MeshCreateInfo.Vertices.Size(), VertexDataOffset);
        IndexDataOffset  += OutputStream.Write(MeshCreateInfo.Indices.Data(), MeshCreateInfo.Indices.Size(), IndexDataOffset);

        for (int32 SubMeshIdx = 0; SubMeshIdx < MeshCreateInfo.SubMeshes.Size(); SubMeshIdx++)
        {
            const FSubMeshInfo& SubMeshInfo = MeshCreateInfo.SubMeshes[SubMeshIdx];

            // SubMesh vertices and indices are based on the mesh and not the model
            ModelFormat::FSubMeshInfo SubMesh;
            SubMesh.BaseVertex    = SubMeshInfo.BaseVertex;
            SubMesh.NumVertices   = SubMeshInfo.VertexCount;
            SubMesh.StartIndex    = SubMeshInfo.StartIndex;
            SubMesh.NumIndicies   = SubMeshInfo.IndexCount;
            SubMesh.MaterialIndex = SubMeshInfo.MaterialIndex;
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
        const FMaterialCreateInfo& MaterialCreateInfo = Model->Materials[MaterialIdx];

        ModelFormat::FMaterialInfo Material;
        FMemory::Memzero(&Material, sizeof(ModelFormat::FMaterialInfo));

        Material.DiffuseTextureIdx          = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Diffuse]);
        Material.NormalTextureIdx           = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Normal]);
        Material.SpecularTextureIdx         = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Specular]);
        Material.EmissiveTextureIdx         = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Emissive]);
        Material.AmbientOcclusionTextureIdx = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::AmbientOcclusion]);
        Material.RoughnessTextureIdx        = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Roughness]);
        Material.MetallicTextureIdx         = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::Metallic]);
        Material.AlphaMaskTextureIdx        = CreateTextureIndex(MaterialCreateInfo.Textures[EMaterialTexture::AlphaMask]);
        Material.Diffuse                    = MaterialCreateInfo.Diffuse;
        Material.AO                         = MaterialCreateInfo.AmbientFactor;
        Material.Roughness                  = MaterialCreateInfo.Roughness;
        Material.Metallic                   = MaterialCreateInfo.Metallic;
        Material.MaterialFlags              = static_cast<int32>(MaterialCreateInfo.MaterialFlags);
        
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
