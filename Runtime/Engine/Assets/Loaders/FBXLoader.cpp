#include "FBXLoader.h"

#include "Engine/Assets/VertexFormat.h"
#include "Engine/Assets/MeshUtilities.h"

#include "Core/Math/Matrix4.h"
#include "Core/Containers/HashTable.h"
#include "Core/Utilities/StringUtilities.h"
#include "Core/Application/Log.h"

#include <ofbx.h>

static CString ExtractPath( const CString& FullFilePath )
{
    auto Pos = FullFilePath.ReverseFind( '/' );
    if ( Pos != CString::NPos )
    {
        return FullFilePath.SubString( 0, Pos );
    }
    else
    {
        return FullFilePath;
    }
}

static CMatrix4 ToFloat4x4( const ofbx::Matrix& Matrix )
{
    CMatrix4 Result;
    for ( uint32 y = 0; y < 4; y++ )
    {
        for ( uint32 x = 0; x < 4; x++ )
        {
            const uint32 Index = y * 4 + x;
            Result.f[y][x] = static_cast<float>(Matrix.m[Index]);
        }
    }

    return Result;
}

static void GetMatrix( const ofbx::Object* Mesh, CMatrix4& OutMatrix )
{
    if ( Mesh )
    {
        CMatrix4 Matrix;
        GetMatrix( Mesh->getParent(), Matrix );

        ofbx::Vec3 Scaling = Mesh->getLocalScaling();
        ofbx::Vec3 Rotation = Mesh->getLocalRotation();
        ofbx::Vec3 Translation = Mesh->getLocalTranslation();

        CMatrix4 LocalMatrix = ToFloat4x4( Mesh->evalLocal( Translation, Rotation, Scaling ) );
        OutMatrix = Matrix * LocalMatrix;
    }
    else
    {
        OutMatrix = CMatrix4::Identity();
    }
}

static TSharedPtr<SImage2D> LoadMaterialTexture( const CString& Path, const ofbx::Material* Material, ofbx::Texture::TextureType Type )
{
#if 0
    const ofbx::Texture* MaterialTexture = Material->getTexture( Type );
    if ( MaterialTexture )
    {
        // Non-static buffer to support multi threading
        char StringBuffer[256];
        MaterialTexture->getRelativeFileName().toString( StringBuffer );

        // Make sure that correct slashes are used
        CString Filename = StringBuffer;
        ConvertBackslashes( Filename );

        TSharedPtr<SImage2D> Texture = MakeShared<SImage2D>();
        return Texture;
    }
    else
    #else
    UNREFERENCED_VARIABLE( Path );
    UNREFERENCED_VARIABLE( Material );
    UNREFERENCED_VARIABLE( Type );
#endif
    {
        return TSharedPtr<SImage2D>();
    }
}

bool CFBXLoader::LoadFile( const CString& Filename, SSceneData& OutScene, uint32 Flags ) noexcept
{
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    FILE* File = fopen( Filename.CStr(), "rb" );
    if ( !File )
    {
        LOG_ERROR( ("[CFBXLoader]: Failed to open '" + Filename + "'").CStr() );
        return false;
    }

    // TODO: Utility to read in full file?
    fseek( File, 0, SEEK_END );
    uint32 FileSize = (uint32)ftell( File );
    rewind( File );

    TArray<ofbx::u8> FileContent( FileSize );
    uint32 SizeInBytes = FileContent.SizeInBytes();
    UNREFERENCED_VARIABLE( SizeInBytes );

    ofbx::u8* Bytes = FileContent.Data();

    const uint32 ChunkSize = 1024;

    uint32 NumBytesRead = 0;
    while ( NumBytesRead < FileSize )
    {
        NumBytesRead += (uint32)fread( Bytes, 1, ChunkSize, File );
        Bytes += ChunkSize;
    }

    if ( NumBytesRead != FileSize )
    {
        LOG_ERROR( ("[CFBXLoader]: Failed to load '" + Filename + "'").CStr() );
        return false;
    }

    Bytes = FileContent.Data();

    ofbx::IScene* FBXScene = ofbx::load( Bytes, FileSize, (ofbx::u64)ofbx::LoadFlags::TRIANGULATE );
    if ( !FBXScene )
    {
        LOG_ERROR( ("[CMeshFactory]: Failed to load content '" + Filename + "'").CStr() );
        return false;
    }

    const ofbx::GlobalSettings* Settings = FBXScene->getGlobalSettings();

    // Unique tables
    THashTable<SVertex, uint32, SVertexHasher>  UniqueVertices;
    THashTable<const ofbx::Material*, uint32> UniqueMaterials;

    CString Path = ExtractPath( Filename );

    SModelData Data;

    uint32 MeshCount = FBXScene->getMeshCount();
    for ( uint32 i = 0; i < MeshCount; i++ )
    {
        const ofbx::Mesh* CurrentMesh = FBXScene->getMesh( i );
        const ofbx::Geometry* CurrentGeom = CurrentMesh->getGeometry();

        uint32 MaterialCount = CurrentMesh->getMaterialCount();
        for ( uint32 j = 0; j < MaterialCount; j++ )
        {
            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial( j );
            if ( UniqueMaterials.count( CurrentMaterial ) != 0 )
            {
                continue;
            }

            SMaterialData MaterialData;
            MaterialData.DiffuseTexture = LoadMaterialTexture( Path, CurrentMaterial, ofbx::Texture::TextureType::DIFFUSE );
            MaterialData.NormalTexture = LoadMaterialTexture( Path, CurrentMaterial, ofbx::Texture::TextureType::NORMAL );
            MaterialData.SpecularTexture = LoadMaterialTexture( Path, CurrentMaterial, ofbx::Texture::TextureType::SPECULAR );
            MaterialData.EmissiveTexture = LoadMaterialTexture( Path, CurrentMaterial, ofbx::Texture::TextureType::EMISSIVE );
            MaterialData.AOTexture = LoadMaterialTexture( Path, CurrentMaterial, ofbx::Texture::TextureType::AMBIENT );

            MaterialData.Diffuse = CVector3( CurrentMaterial->getDiffuseColor().r, CurrentMaterial->getDiffuseColor().g, CurrentMaterial->getDiffuseColor().b );
            MaterialData.AO = 1.0f;//  CurrentMaterial->getSpecularColor().r;
            MaterialData.Roughness = 1.0f;// CurrentMaterial->getSpecularColor().g;
            MaterialData.Metallic = 1.0f;// CurrentMaterial->getSpecularColor().b;

            //TODO: Other material properties

            //UniqueMaterials[CurrentMaterial] = OutScene.Materials.Size();
            //OutScene.Materials.Emplace( MaterialData );
        }

        int32 VertexCount = (int32)CurrentGeom->getVertexCount();
        int32 IndexCount = (int32)CurrentGeom->getIndexCount();
        Data.Mesh.Indices.Reserve( IndexCount );
        Data.Mesh.Vertices.Reserve( VertexCount );
        UniqueVertices.reserve( VertexCount );

        const int* Materials = CurrentGeom->getMaterials();

        const ofbx::Vec3* Vertices = CurrentGeom->getVertices();
        Assert( Vertices != nullptr );

        const ofbx::Vec3* Normals = CurrentGeom->getNormals();
        Assert( Normals != nullptr );

        const ofbx::Vec2* TexCoords = CurrentGeom->getUVs( 0 );
        Assert( TexCoords != nullptr );

        const ofbx::Vec3* Tangents = CurrentGeom->getTangents();

        CMatrix4 Matrix = ToFloat4x4( CurrentMesh->getGlobalTransform() );
        CMatrix4 GeometricMatrix = ToFloat4x4( CurrentMesh->getGeometricMatrix() );
        CMatrix4 Transform = Matrix * GeometricMatrix;

        int32 CurrentIndex = 0;
        int32 MaterialIndex = -1;
        int32 LastMaterialIndex = 0;
        while ( CurrentIndex < IndexCount )
        {
            Data.MaterialIndex = -1;
            Data.Mesh.Clear();

            UniqueVertices.clear();

            for ( ; CurrentIndex < IndexCount; CurrentIndex++ )
            {
                if ( Materials )
                {
                    uint32 TriangleIndex = CurrentIndex / 3;
                    LastMaterialIndex = MaterialIndex;
                    MaterialIndex = Materials[TriangleIndex];
                    if ( MaterialIndex != LastMaterialIndex )
                    {
                        break;
                    }
                }

                SVertex TempVertex;

                // Position
                CVector3 Position = CVector3( (float)Vertices[CurrentIndex].x, (float)Vertices[CurrentIndex].y, (float)Vertices[CurrentIndex].z );
                TempVertex.Position = Transform.TransformPosition( Position );

                // Apply the scene scale
                if ( Flags & EFBXFlags::FBXFlags_ApplyScaleFactor )
                {
                    TempVertex.Position /= Settings->UnitScaleFactor;
                }

                // Normal
                CVector3 Normal = CVector3( (float)Normals[CurrentIndex].x, (float)Normals[CurrentIndex].y, (float)Normals[CurrentIndex].z );
                TempVertex.Normal = Transform.TransformDirection( Normal );

                // TexCoords
                TempVertex.TexCoord = CVector2( (float)TexCoords[CurrentIndex].x, (float)TexCoords[CurrentIndex].y );

                // Tangents
                if ( Tangents )
                {
                    CVector3 Tangent = CVector3( (float)Tangents[CurrentIndex].x, (float)Tangents[CurrentIndex].y, (float)Tangents[CurrentIndex].z );
                    TempVertex.Tangent = Transform.TransformDirection( Tangent );
                }

                // Only push unique vertices
                uint32 UniqueIndex = 0;
                if ( UniqueVertices.count( TempVertex ) == 0 )
                {
                    UniqueIndex = static_cast<uint32>(Data.Mesh.Vertices.Size());
                    UniqueVertices[TempVertex] = UniqueIndex;
                    Data.Mesh.Vertices.Push( TempVertex );
                }
                else
                {
                    UniqueIndex = UniqueVertices[TempVertex];
                }

                Data.Mesh.Indices.Emplace( UniqueIndex );
            }

            if ( !Tangents )
            {
                CMeshUtilities::CalculateTangents( Data.Mesh );
            }

            // Convert to left-handed
            if ( Flags & EFBXFlags::FBXFlags_EnsureLeftHanded )
            {
                if ( Settings->CoordAxis == ofbx::CoordSystem_RightHanded )
                {
                    CMeshUtilities::ReverseHandedness( Data.Mesh );
                }
            }

            Data.Name = CurrentMesh->name;

            const ofbx::Material* CurrentMaterial = CurrentMesh->getMaterial( LastMaterialIndex );
            if ( UniqueMaterials.count( CurrentMaterial ) != 0 )
            {
                Data.MaterialIndex = UniqueMaterials[CurrentMaterial];
            }
            else
            {
                Data.MaterialIndex = -1;
            }

            if ( Data.Mesh.Hasdata() )
            {
                OutScene.Models.Emplace( Data );
            }
            else
            {
                LOG_WARNING( "Tried to load mesh without any data" );
            }
        }
    }

    OutScene.Models.ShrinkToFit();
    OutScene.Materials.ShrinkToFit();

    FBXScene->destroy();
    return true;
}
