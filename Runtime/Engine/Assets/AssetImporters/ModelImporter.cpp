#include "Core/Platform/PlatformFile.h"
#include "Core/Filesystem/File.h"
#include "Core/Templates/CString.h"
#include "Core/Containers/Stream.h"
#include "Core/Misc/Parse.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/CRC.h"
#include "Engine/Assets/AssetManager.h"
#include "Engine/Assets/AssetImporters/ModelImporter.h"
#include "Engine/Assets/AssetImporters/FBXImporter.h"
#include "Engine/Assets/AssetImporters/OBJImporter.h"

bool FModelImporter::ImportFromFile(const StringView& InFilename, EMeshImportFlags, FModelData& OutModelData)
{
    FByteInputStream InputStream;

    {
        const String Filename = String(InFilename);
        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
        if (!FileHandle)
        {
            return false;
        }

        // Read the full file
        if (!File::ReadFile(FileHandle.Get(), InputStream))
        {
            return false;
        }
    }

    // 1) Read file-header
    ModelFormat::FFileHeader FileHeader;
    InputStream.Read(FileHeader);

    if (Memory::Memcmp(FileHeader.Magic, "DXRMESH", sizeof(FileHeader.Magic)) != 0)
    {
        return false;
    }

    if (FileHeader.VersionMajor != MODEL_FORMAT_VERSION_MAJOR || FileHeader.VersionMinor != MODEL_FORMAT_VERSION_MINOR)
    {
        return false;
    }

    const uint64 DataSize = InputStream.Size() - sizeof(ModelFormat::FFileHeader);
    if (FileHeader.DataSize != DataSize)
    {
        return false;
    }

    const uint64 DataCRC = CRC32::Generate(InputStream.PeekData(), DataSize);
    if (FileHeader.DataCRC != DataCRC)
    {
        return false;
    }

    // 2) Model Header
    const ModelFormat::FModelHeader* ModelHeader = InputStream.PeekData<ModelFormat::FModelHeader>();

    // Load MeshData
    OutModelData.Meshes.Resize(ModelHeader->NumMeshes);

    // 3) Mesh Headers
    const ModelFormat::FMeshInfo*    MeshHeaders = InputStream.PeekData<ModelFormat::FMeshInfo>(ModelHeader->MeshDataOffset);
    const ModelFormat::FSubMeshInfo* SubMeshData = InputStream.PeekData<ModelFormat::FSubMeshInfo>(ModelHeader->SubMeshDataOffset);

    // 4) Geometry Buffers
    const uint32* IndexData = InputStream.PeekData<uint32>(ModelHeader->IndexDataOffset);

    uint16 StreamStrides[VERTEX_MAX_STREAMS] = {};
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader->NumMeshes; ++MeshIdx)
    {
        const FVertexDeclaration& Declaration = FVertexDeclaration::GetOrCreate(static_cast<EVertexAttributeFlags>(MeshHeaders[MeshIdx].AttributeFlags));
        for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
        {
            StreamStrides[StreamIndex] = Math::Max(StreamStrides[StreamIndex], Declaration.GetStreamStride(StreamIndex));
        }
    }

    for (int32 MeshIdx = 0; MeshIdx < ModelHeader->NumMeshes; ++MeshIdx)
    {
        FMeshData& MeshData = OutModelData.Meshes[MeshIdx];

        const ModelFormat::FMeshInfo& MeshHeader = MeshHeaders[MeshIdx];
        MAYBE_UNUSED const int32 Length = CString::Strlen(MeshHeader.Name);
        CHECK(Length < MODEL_FORMAT_MAX_NAME_LENGTH);

        MeshData.Name = MeshHeader.Name;
        LOG_INFO("Loaded Mesh '%s'", MeshHeader.Name);

        const ModelFormat::FSubMeshInfo* SubMeshes = SubMeshData + MeshHeader.FirstSubMesh;
        MeshData.SubMeshes.Resize(MeshHeader.NumSubMeshes);

        MeshData.Declaration       = FVertexDeclaration::GetOrCreate(static_cast<EVertexAttributeFlags>(MeshHeader.AttributeFlags));
        MeshData.PackedVertexCount = MeshHeader.NumVertices;

        for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
        {
            const uint16 Stride = MeshData.Declaration.GetStreamStride(StreamIndex);
            if (Stride == 0)
            {
                continue;
            }

            const int32  ByteOffset = ModelHeader->StreamDataOffset[StreamIndex] + (MeshHeader.FirstVertex * StreamStrides[StreamIndex]);
            const uint8* StreamData = InputStream.PeekData<uint8>(ByteOffset);
            MeshData.PackedStreams[StreamIndex].Reset(StreamData, MeshHeader.NumVertices * Stride);
        }

        const uint32* Indices = IndexData + MeshHeader.FirstIndex;
        MeshData.Indices.Reset(Indices, MeshHeader.NumIndices);

        for (int32 SubMeshIdx = 0; SubMeshIdx < MeshHeader.NumSubMeshes; SubMeshIdx++)
        {
            FSubMesh& SubMesh     = MeshData.SubMeshes[SubMeshIdx];
            SubMesh.BaseVertex    = SubMeshes[SubMeshIdx].BaseVertex;
            SubMesh.VertexCount   = SubMeshes[SubMeshIdx].NumVertices;
            SubMesh.StartIndex    = SubMeshes[SubMeshIdx].StartIndex;
            SubMesh.IndexCount    = SubMeshes[SubMeshIdx].NumIndicies;
            SubMesh.MaterialIndex = SubMeshes[SubMeshIdx].MaterialIndex;
        }
    }

    // Load Textures
    TArray<FTextureRef> LoadedTextures;
    LoadedTextures.Resize(ModelHeader->NumTextures);

    // 5) Textures
    const ModelFormat::FTextureInfo* Textures = InputStream.PeekData<ModelFormat::FTextureInfo>(ModelHeader->TextureDataOffset);
    for (int32 TextureIdx = 0; TextureIdx < ModelHeader->NumTextures; ++TextureIdx)
    {
        const StringView FilenameView = Textures[TextureIdx].Filepath;
        if (!FPlatformFile::IsFile(*FilenameView))
        {
            LOG_ERROR("[FModelImporter] Stored file contains a invalid file reference, file will be reloaded from source");
            return false;
        }

        const String Filename = String(FilenameView);
        LoadedTextures[TextureIdx] = FAssetManager::Get().LoadTexture(Filename);
        if (!LoadedTextures[TextureIdx])
        {
            LOG_ERROR("[FModelImporter] Failed to load texture '%s'", *Filename);
            return false;
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
    OutModelData.Materials.Resize(ModelHeader->NumMaterials);

    // 6) Materials
    const ModelFormat::FMaterialInfo* Materials = InputStream.PeekData<ModelFormat::FMaterialInfo>(ModelHeader->MaterialDataOffset);
    for (int32 Index = 0; Index < ModelHeader->NumMaterials; ++Index)
    {
        FMaterialData& MaterialData = OutModelData.Materials[Index];
        MaterialData.Textures[EMaterialTexture::Diffuse]          = RetrieveTexture(Materials[Index].DiffuseTextureIdx);
        MaterialData.Textures[EMaterialTexture::Normal]           = RetrieveTexture(Materials[Index].NormalTextureIdx);
        MaterialData.Textures[EMaterialTexture::Specular]         = RetrieveTexture(Materials[Index].SpecularTextureIdx);
        MaterialData.Textures[EMaterialTexture::Roughness]        = RetrieveTexture(Materials[Index].RoughnessTextureIdx);
        MaterialData.Textures[EMaterialTexture::AmbientOcclusion] = RetrieveTexture(Materials[Index].AmbientOcclusionTextureIdx);
        MaterialData.Textures[EMaterialTexture::Metallic]         = RetrieveTexture(Materials[Index].MetallicTextureIdx);
        MaterialData.Textures[EMaterialTexture::Emissive]         = RetrieveTexture(Materials[Index].EmissiveTextureIdx);
        MaterialData.Textures[EMaterialTexture::AlphaMask]        = RetrieveTexture(Materials[Index].AlphaMaskTextureIdx);
        
        MaterialData.MaterialFlags = static_cast<EMaterialFlags>(Materials[Index].MaterialFlags);
        MaterialData.Diffuse       = Materials[Index].Diffuse;
        MaterialData.Roughness     = Materials[Index].Roughness;
        MaterialData.AmbientFactor = Materials[Index].AO;
        MaterialData.Metallic      = Materials[Index].Metallic;
    }

    return true;
}

bool FModelImporter::MatchExtenstion(const StringView& FileName)
{
    return FileName.EndsWith(".dxrmesh", EStringCaseType::NoCase);
}

bool FModelSerializer::Serialize(const String& Filename, const FModelData& ModelData)
{
    ModelFormat::FModelHeader ModelHeader;
    Memory::Memzero(&ModelHeader, sizeof(ModelFormat::FModelHeader));

    FByteOutputStream OutputStream;
    OutputStream.AddUninitialized<ModelFormat::FModelHeader>();

    // Count mesh-primitives and prepare headers
    ModelHeader.NumMeshes      = ModelData.Meshes.Size();
    ModelHeader.MeshDataOffset = OutputStream.AddUninitialized<ModelFormat::FMeshInfo>(ModelHeader.NumMeshes);

    struct FPackedMesh
    {
        TArray<uint8> Streams[VERTEX_MAX_STREAMS];
    };

    TArray<FPackedMesh> PackedMeshes;
    PackedMeshes.Resize(ModelData.Meshes.Size());

    for (int32 MeshIdx = 0; MeshIdx < ModelData.Meshes.Size(); ++MeshIdx)
    {
        if (!ModelData.Meshes[MeshIdx].PackVertexStreams(PackedMeshes[MeshIdx].Streams))
        {
            return false;
        }
    }

    int32 NumVertices  = 0;
    int32 NumIndicies  = 0;
    int32 NumSubMeshes = 0;

    int32 MeshDataOffset = ModelHeader.MeshDataOffset;
    for (int32 MeshIdx = 0; MeshIdx < ModelHeader.NumMeshes; ++MeshIdx)
    {
        const FMeshData& MeshData = ModelData.Meshes[MeshIdx];

        ModelFormat::FMeshInfo Header;
        Memory::Memzero(&Header, sizeof(ModelFormat::FMeshInfo));

        Header.FirstVertex    = NumVertices;
        Header.NumVertices    = MeshData.Vertices.Size();
        Header.FirstIndex     = NumIndicies;
        Header.NumIndices     = MeshData.Indices.Size();
        Header.FirstSubMesh   = NumSubMeshes;
        Header.NumSubMeshes   = MeshData.SubMeshes.Size();
        Header.AttributeFlags = static_cast<int32>(MeshData.Declaration.GetAttributeFlags());

        CString::Strncpy(Header.Name, *MeshData.Name, MODEL_FORMAT_MAX_NAME_LENGTH);
        MeshDataOffset += OutputStream.Write(Header, MeshDataOffset);

        NumVertices  += Header.NumVertices;
        NumIndicies  += Header.NumIndices;
        NumSubMeshes += Header.NumSubMeshes;
    }

    uint16 StreamStrides[VERTEX_MAX_STREAMS] = {};
    for (const FMeshData& MeshData : ModelData.Meshes)
    {
        for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
        {
            StreamStrides[StreamIndex] = Math::Max(StreamStrides[StreamIndex], MeshData.Declaration.GetStreamStride(StreamIndex));
        }
    }

    for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
    {
        ModelHeader.StreamDataOffset[StreamIndex] = StreamStrides[StreamIndex] > 0
            ? OutputStream.AddUninitialized<uint8>(NumVertices * StreamStrides[StreamIndex])
            : 0;
    }

    ModelHeader.NumVertices       = NumVertices;
    ModelHeader.IndexDataOffset   = OutputStream.AddUninitialized<uint32>(NumIndicies);
    ModelHeader.NumIndicies       = NumIndicies;
    ModelHeader.SubMeshDataOffset = OutputStream.AddUninitialized<ModelFormat::FSubMeshInfo>(NumSubMeshes);
    ModelHeader.NumSubMeshes      = NumSubMeshes;

    int32 StreamDataOffset[VERTEX_MAX_STREAMS];
    for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
    {
        StreamDataOffset[StreamIndex] = ModelHeader.StreamDataOffset[StreamIndex];
    }

    int32 IndexDataOffset   = ModelHeader.IndexDataOffset;
    int32 SubMeshDataOffset = ModelHeader.SubMeshDataOffset;

    for (int32 MeshIdx = 0; MeshIdx < ModelHeader.NumMeshes; ++MeshIdx)
    {
        const FMeshData& MeshData = ModelData.Meshes[MeshIdx];

        for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; StreamIndex++)
        {
            if (StreamStrides[StreamIndex] == 0)
            {
                continue;
            }

            const TArray<uint8>& StreamData = PackedMeshes[MeshIdx].Streams[StreamIndex];
            if (!StreamData.IsEmpty())
            {
                OutputStream.Write(StreamData.Data(), StreamData.Size(), StreamDataOffset[StreamIndex]);
            }

            StreamDataOffset[StreamIndex] += MeshData.Vertices.Size() * StreamStrides[StreamIndex];
        }

        IndexDataOffset += OutputStream.Write(MeshData.Indices.Data(), MeshData.Indices.Size(), IndexDataOffset);

        for (int32 SubMeshIdx = 0; SubMeshIdx < MeshData.SubMeshes.Size(); SubMeshIdx++)
        {
            const FSubMesh& SubMesh = MeshData.SubMeshes[SubMeshIdx];

            // SubMesh vertices and indices are based on the mesh and not the model
            ModelFormat::FSubMeshInfo SubMeshHeader;
            SubMeshHeader.BaseVertex    = SubMesh.BaseVertex;
            SubMeshHeader.NumVertices   = SubMesh.VertexCount;
            SubMeshHeader.StartIndex    = SubMesh.StartIndex;
            SubMeshHeader.NumIndicies   = SubMesh.IndexCount;
            SubMeshHeader.MaterialIndex = SubMesh.MaterialIndex;
            SubMeshDataOffset += OutputStream.Write(SubMeshHeader, SubMeshDataOffset);
        }
    }

    // Prepare material primitives
    ModelHeader.NumMaterials       = ModelData.Materials.Size();
    ModelHeader.MaterialDataOffset = OutputStream.AddUninitialized<ModelFormat::FMaterialInfo>(ModelHeader.NumMaterials);

    // Create a new TextureIndex
    ModelHeader.TextureDataOffset = OutputStream.WriteOffset();
    ModelHeader.NumTextures       = 0;

    const auto CreateTextureIndex = [&](const FTexture2DRef& Texture)
    {
        if (Texture)
        {
            ModelFormat::FTextureInfo TextureHeader;
            Memory::Memzero(&TextureHeader, sizeof(ModelFormat::FTextureInfo));

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
        const FMaterialData& MaterialData = ModelData.Materials[MaterialIdx];

        ModelFormat::FMaterialInfo Material;
        Memory::Memzero(&Material, sizeof(ModelFormat::FMaterialInfo));

        Material.DiffuseTextureIdx          = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Diffuse]);
        Material.NormalTextureIdx           = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Normal]);
        Material.SpecularTextureIdx         = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Specular]);
        Material.EmissiveTextureIdx         = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Emissive]);
        Material.AmbientOcclusionTextureIdx = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::AmbientOcclusion]);
        Material.RoughnessTextureIdx        = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Roughness]);
        Material.MetallicTextureIdx         = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::Metallic]);
        Material.AlphaMaskTextureIdx        = CreateTextureIndex(MaterialData.Textures[EMaterialTexture::AlphaMask]);
        Material.Diffuse                    = MaterialData.Diffuse;
        Material.AO                         = MaterialData.AmbientFactor;
        Material.Roughness                  = MaterialData.Roughness;
        Material.Metallic                   = MaterialData.Metallic;
        Material.MaterialFlags              = static_cast<int32>(MaterialData.MaterialFlags);
        
        MaterialDataOffset += OutputStream.Write(Material, MaterialDataOffset);
    }

    OutputStream.Write(ModelHeader, 0);

    {
        TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(Filename);
        if (!File)
        {
            return false;
        }

        ModelFormat::FFileHeader FileHeader;
        Memory::Memzero(&FileHeader, sizeof(ModelFormat::FFileHeader));

        Memory::Memcpy(FileHeader.Magic, "DXRMESH", sizeof(FileHeader.Magic));
        FileHeader.DataCRC      = CRC32::Generate(OutputStream.Data(), OutputStream.Size());
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
