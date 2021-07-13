#include "FBXLoader.h"

// Temporary string converter from .dds to .png
// TODO: Maybe support .dds loading :)
static void FileStringWithDDSToPNG(std::string& OutString)
{
    for (char& Char : OutString)
    {
        Char = tolower(Char);
    }

    const char*  SearchString = ".dds";
    const UInt32 Length = strlen(SearchString);

    size_t Pos = OutString.find(SearchString);
    while (Pos != std::string::npos)
    {
        OutString.replace(Pos, std::string::npos, ".PNG");
        Pos = OutString.find(SearchString, Pos + Length);
    }
}

static std::string ExtractPath(const std::string& FullFilePath)
{
    size_t Pos = FullFilePath.find_last_of('/');
    if (Pos != std::string::npos)
    {
        return FullFilePath.substr(0, Pos);
    }
    else
    {
        return FullFilePath;
    }
}

static XMFLOAT4X4 ToFloat4x4(const ofbx::Matrix& Matrix)
{
    XMFLOAT4X4 Result;
    for (UInt32 y = 0; y < 4; y++)
    {
        for (UInt32 x = 0; x < 4; x++)
        {
            UInt32 Index = y * 4 + x;
            Result.m[y][x] = Matrix.m[Index];
        }
    }

    return Result;
}

static void GetMatrix(const ofbx::Object* Mesh, XMFLOAT4X4& OutMatrix)
{
    if (Mesh)
    {
        XMFLOAT4X4 Matrix;
        GetMatrix(Mesh->getParent(), Matrix);

        ofbx::Vec3 Scaling     = Mesh->getLocalScaling();
        ofbx::Vec3 Rotation    = Mesh->getLocalRotation();
        ofbx::Vec3 Translation = Mesh->getLocalTranslation();

        XMFLOAT4X4 LocalMatrix = ToFloat4x4(Mesh->evalLocal(Translation, Rotation, Scaling));

        XMMATRIX Matrix0 = XMLoadFloat4x4(&Matrix);
        XMMATRIX Matrix1 = XMLoadFloat4x4(&LocalMatrix);
        XMStoreFloat4x4(&OutMatrix, XMMatrixMultiply(Matrix0, Matrix1));
    }
    else
    {
        XMStoreFloat4x4(&OutMatrix, XMMatrixIdentity());
    }
}

bool CFFBXLoader::LoadFBXFile(const String& Filename)
{
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    FILE* File = fopen(Filename.c_str(), "rb");
    if (!File)
    {
        LOG_ERROR("[MeshFactory]: Failed to open '" +  Filename + "'");
        return false;
    }

    fseek(File, 0, SEEK_END);
    UInt32 FileSize = ftell(File);
    rewind(File);

    TArray<ofbx::u8> FileContent(FileSize);
    UInt32 SizeInBytes = FileContent.SizeInBytes();
    
    ofbx::u8* Bytes = FileContent.Data();

    const UInt32 ChunkSize = 1024;

    UInt32 NumBytesRead = 0;
    while (NumBytesRead < FileSize)
    {
        NumBytesRead += fread(Bytes, 1, ChunkSize, File);
        Bytes += ChunkSize;
    }

    if (NumBytesRead != FileSize)
    {
        LOG_ERROR("[MeshFactory]: Failed to load '" + Filename + "'");
        return false;
    }

    Bytes = FileContent.Data();

    ofbx::IScene* FBXScene = ofbx::load(Bytes, FileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE);
    if (!FBXScene)
    {
        LOG_ERROR("[MeshFactory]: Failed to load content '" + Filename + "'");
        return false;
    }
    
    const ofbx::GlobalSettings* Settings = FBXScene->getGlobalSettings();
    OutScene.ScaleFactor = 1.0f / Settings->UnitScaleFactor;

    std::unordered_map<Vertex, UInt32, VertexHasher>  UniqueVertices;
    std::unordered_map<const ofbx::Material*, UInt32> UniqueMaterials;

    static char StrBuffer[256];

    std::string Path = ExtractPath(Filename);

    ModelData Data;

    UInt32 MeshCount = FBXScene->getMeshCount();
    for (UInt32 i = 0; i < MeshCount; i++)
    {
        const ofbx::Mesh*     CurrentMesh = FBXScene->getMesh(i);
        const ofbx::Geometry* CurrentGeom = CurrentMesh->getGeometry();

        UInt32 MaterialCount = CurrentMesh->getMaterialCount();
        for (UInt32 j = 0; j < MaterialCount; j++)
        {
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(j);
            if (UniqueMaterials.count(CurrentMaterial) != 0)
            {
                continue;
            }

            MaterialData MaterialData;
            MaterialData.Diffuse   = XMFLOAT3(CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b);
            MaterialData.TexPath   = Path;
            MaterialData.AO        = 1.0f;//  CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness = 1.0f;// CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic  = 1.0f;// CurrentMaterial->getSpecularColor().b;

            //TODO: Other material properties

            const ofbx::Texture* AmbientTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::AMBIENT);
            if (AmbientTex)
            {
                AmbientTex->getRelativeFileName().toString(StrBuffer);
                MaterialData.AOTexname = StrBuffer;

                ConvertBackslashes(MaterialData.AOTexname);
                // TODO: We should support .dds in the future
                FileStringWithDDSToPNG(MaterialData.AOTexname);
            }

            const ofbx::Texture* DiffuseTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::DIFFUSE);
            if (DiffuseTex)
            {
                DiffuseTex->getRelativeFileName().toString(StrBuffer);
                MaterialData.DiffTexName = StrBuffer;

                ConvertBackslashes(MaterialData.DiffTexName);
                // TODO: We should support .dds in the future
                FileStringWithDDSToPNG(MaterialData.DiffTexName);
            }

            const ofbx::Texture* NormalTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::NORMAL);
            if (NormalTex)
            {
                NormalTex->getRelativeFileName().toString(StrBuffer);
                MaterialData.NormalTexname = StrBuffer;

                ConvertBackslashes(MaterialData.NormalTexname);
                // TODO: We should support .dds in the future
                FileStringWithDDSToPNG(MaterialData.NormalTexname);
            }

            const ofbx::Texture* SpecularTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::SPECULAR);
            if (SpecularTex)
            {
                SpecularTex->getRelativeFileName().toString(StrBuffer);
                MaterialData.SpecTexName = StrBuffer;

                ConvertBackslashes(MaterialData.SpecTexName);
                // TODO: We should support .dds in the future
                FileStringWithDDSToPNG(MaterialData.SpecTexName);
            }

            const ofbx::Texture* ReflectionTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::REFLECTION);
            if (ReflectionTex)
            {
                // TODO: Load this properly
            }

            const ofbx::Texture* ShininessTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::SHININESS);
            if (ShininessTex)
            {
                // TODO: Load this properly
            }

            const ofbx::Texture* EmissiveTex = CurrentMaterial->getTexture(ofbx::Texture::TextureType::EMISSIVE);
            if (EmissiveTex)
            {
                EmissiveTex->getRelativeFileName().toString(StrBuffer);
                MaterialData.EmissiveTexName = StrBuffer;

                ConvertBackslashes(MaterialData.EmissiveTexName);
                // TODO: We should support .dds in the future
                FileStringWithDDSToPNG(MaterialData.EmissiveTexName);
            }

            UniqueMaterials[CurrentMaterial] = OutScene.Materials.Size();
            OutScene.Materials.EmplaceBack(MaterialData);
        }

        UInt32 VertexCount = CurrentGeom->getVertexCount();
        UInt32 IndexCount  = CurrentGeom->getIndexCount();
        Data.Mesh.Indices.Reserve(IndexCount);
        Data.Mesh.Vertices.Reserve(VertexCount);
        UniqueVertices.reserve(VertexCount);

        const int* Materials = CurrentGeom->getMaterials();
        const int* Indices   = CurrentGeom->getFaceIndices();
        Assert(Indices != nullptr);

        const ofbx::Vec3* Vertices = CurrentGeom->getVertices();
        Assert(Vertices != nullptr);

        const ofbx::Vec3* Normals = CurrentGeom->getNormals();
        Assert(Normals != nullptr);

        const ofbx::Vec2* TexCoords = CurrentGeom->getUVs(0);
        Assert(TexCoords != nullptr);

        const ofbx::Vec3* Tangents = CurrentGeom->getTangents();

        XMFLOAT4X4 Matrix = ToFloat4x4(CurrentMesh->getGlobalTransform());
        XMFLOAT4X4 GeometricMatrix = ToFloat4x4(CurrentMesh->getGeometricMatrix());
        XMMATRIX XMTransform = XMMatrixMultiply(XMLoadFloat4x4(&Matrix), XMLoadFloat4x4(&GeometricMatrix));

        UInt32 CurrentIndex  = 0;
        UInt32 MaterialIndex = 0;
        UInt32 LastMaterialIndex = 0;
        while (CurrentIndex < IndexCount)
        {
            Data.MaterialIndex = -1;
            Data.Mesh.Clear();

            UniqueVertices.clear();

            for (; CurrentIndex < IndexCount; CurrentIndex++)
            {
                if (Materials)
                {
                    UInt32 TriangleIndex = CurrentIndex / 3;
                    LastMaterialIndex = MaterialIndex;
                    MaterialIndex     = Materials[TriangleIndex];
                    if (MaterialIndex != LastMaterialIndex)
                    {
                        break;
                    }
                }

                Vertex TempVertex;

                XMVECTOR XmPosition = XMVectorSet(Vertices[CurrentIndex].x, Vertices[CurrentIndex].y, Vertices[CurrentIndex].z, 1.0f);
                XmPosition = XMVector3Transform(XmPosition, XMTransform);
                XMStoreFloat3(&TempVertex.Position, XmPosition);

                XMVECTOR XmNormal = XMVectorSet(Normals[CurrentIndex].x, Normals[CurrentIndex].y, Normals[CurrentIndex].z, 0.0f);
                XmNormal = XMVector3TransformNormal(XmNormal, XMTransform);
                XMStoreFloat3(&TempVertex.Normal, XmNormal);

                TempVertex.TexCoord =
                {
                    (float)TexCoords[CurrentIndex].x,
                    (float)TexCoords[CurrentIndex].y,
                };

                if (Tangents)
                {
                    XMVECTOR XmTangent = XMVectorSet(Tangents[CurrentIndex].x, Tangents[CurrentIndex].y, Tangents[CurrentIndex].z, 0.0f);
                    XmTangent = XMVector3TransformNormal(XmTangent, XMTransform);
                    XMStoreFloat3(&TempVertex.Tangent, XmTangent);
                }

                if (UniqueVertices.count(TempVertex) == 0)
                {
                    UniqueVertices[TempVertex] = static_cast<UInt32>(Data.Mesh.Vertices.Size());
                    Data.Mesh.Vertices.PushBack(TempVertex);
                }

                Data.Mesh.Indices.EmplaceBack(UniqueVertices[TempVertex]);
            }

            if (!Tangents)
            {
                MeshFactory::CalculateTangents(Data.Mesh);
            }

            // Convert to lefthanded
            if (Settings->CoordAxis == ofbx::CoordSystem_RightHanded)
            {
                MeshFactory::LeftHandedConversion(Data.Mesh);
            }

            Data.Name = CurrentMesh->name;
            
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial(LastMaterialIndex);
            Data.MaterialIndex = UniqueMaterials[CurrentMaterial];

            OutScene.Models.EmplaceBack(Data);
        }
    }

    OutScene.Models.ShrinkToFit();
    OutScene.Materials.ShrinkToFit();

    FBXScene->destroy();
    return true;
}