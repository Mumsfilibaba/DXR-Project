#include "OBJLoader.h"
#include "StbImageLoader.h"

#include "Assets/MeshUtilities.h"

#include "Core/Math/MathCommon.h"
#include "Core/Containers/HashTable.h"

#include "Core/Utilities/StringUtilities.h"

#include <tiny_obj_loader.h>

static TSharedPtr<SImage2D> LoadMaterialTexture( const CString& Path, const CString& Filename )
{
    // If filename is empty there is no texture to load
    if ( Filename.IsEmpty() )
    {
        return nullptr;
    }

    CString Fullpath = Path + '/' + Filename;

    // Make sure that correct slashes are used
    ConvertBackslashes( Fullpath );
    return CStbImageLoader::LoadFile( Fullpath );
}

bool COBJLoader::LoadFile( const CString& Filename, SSceneData& OutScene, bool ReverseHandedness )
{
    // Make sure to clear everything
    OutScene.Models.Clear();
    OutScene.Materials.Clear();

    // Load Scene File
    std::string Warning;
    std::string Error;
    std::vector<tinyobj::shape_t>    Shapes;
    std::vector<tinyobj::material_t> Materials;
    tinyobj::attrib_t Attributes;

    CString MTLFiledir = CString( Filename.CStr(), Filename.ReverseFind( '/' ) );
    if ( !tinyobj::LoadObj( &Attributes, &Shapes, &Materials, &Warning, &Error, Filename.CStr(), MTLFiledir.CStr(), true, false ) )
    {
        LOG_WARNING( ("[COBJLoader]: Failed to load '" + Filename + "'." + " Warning: " + Warning.c_str() + " Error: " + Error.c_str()).CStr() );
        return false;
    }
    else
    {
        LOG_INFO( ("[COBJLoader]: Loaded '" + Filename + "'").CStr() );
    }

    // Create All Materials in scene
    for ( tinyobj::material_t& Mat : Materials )
    {
        // Create new material with default properties
        SMaterialData MaterialData;
        MaterialData.MetallicTexture = LoadMaterialTexture( MTLFiledir, Mat.ambient_texname.c_str() );
        MaterialData.DiffuseTexture = LoadMaterialTexture( MTLFiledir, Mat.diffuse_texname.c_str() );
        MaterialData.RoughnessTexture = LoadMaterialTexture( MTLFiledir, Mat.specular_highlight_texname.c_str() );
        MaterialData.NormalTexture = LoadMaterialTexture( MTLFiledir, Mat.bump_texname.c_str() );
        MaterialData.AlphaMaskTexture = LoadMaterialTexture( MTLFiledir, Mat.alpha_texname.c_str() );

        MaterialData.Diffuse = CVector3( Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2] );
        MaterialData.Metallic = Mat.ambient[0];
        MaterialData.AO = 1.0f;
        MaterialData.Roughness = 1.0f;

        OutScene.Materials.Emplace( MaterialData );
    }

    // Construct Scene
    SModelData Data;
    THashTable<Vertex, uint32, VertexHasher> UniqueVertices;
    for ( const tinyobj::shape_t& Shape : Shapes )
    {
        // Start at index zero for eaxh mesh and loop until all indices are processed
        uint32 i = 0;
        uint32 IndexCount = static_cast<uint32>(Shape.mesh.indices.size());
        while ( i < IndexCount )
        {
            // Start a new mesh
            Data.Mesh.Clear();
            UniqueVertices.clear();

            Data.Mesh.Indices.Reserve( IndexCount );

            int32 Face = i / 3;
            const int32 MaterialID = Shape.mesh.material_ids[Face];
            for ( ; i < IndexCount; i++ )
            {
                // Break if material is not the same
                Face = i / 3;
                if ( Shape.mesh.material_ids[Face] != MaterialID )
                {
                    break;
                }

                const tinyobj::index_t& Index = Shape.mesh.indices[i];

                Vertex TempVertex;

                // Normals and texcoords are optional, Positions are required
                Assert( Index.vertex_index >= 0 );

                auto PositionIndex = 3 * Index.vertex_index;
                TempVertex.Position = CVector3( Attributes.vertices[PositionIndex + 0], Attributes.vertices[PositionIndex + 1], Attributes.vertices[PositionIndex + 2] );

                if ( Index.normal_index >= 0 )
                {
                    auto NormalIndex = 3 * Index.normal_index;
                    TempVertex.Normal = CVector3( Attributes.normals[NormalIndex + 0], Attributes.normals[NormalIndex + 1], Attributes.normals[NormalIndex + 2] );
                    TempVertex.Normal.Normalize();
                }

                if ( Index.texcoord_index >= 0 )
                {
                    auto TexCoordIndex = 2 * Index.texcoord_index;
                    TempVertex.TexCoord = CVector2( Attributes.texcoords[TexCoordIndex + 0], Attributes.texcoords[TexCoordIndex + 1] );
                }

                if ( UniqueVertices.count( TempVertex ) == 0 )
                {
                    UniqueVertices[TempVertex] = static_cast<uint32>(Data.Mesh.Vertices.Size());
                    Data.Mesh.Vertices.Push( TempVertex );
                }

                Data.Mesh.Indices.Emplace( UniqueVertices[TempVertex] );
            }

            // Calculate tangents and create mesh
            CMeshUtilities::CalculateTangents( Data.Mesh );

            if ( ReverseHandedness )
            {
                CMeshUtilities::ReverseHandedness( Data.Mesh );
            }

            // Add materialID
            if ( MaterialID >= 0 )
            {
                Data.MaterialIndex = MaterialID;
            }

            Data.Name = Shape.name.c_str();
            OutScene.Models.Emplace( Data );
        }
    }

    OutScene.Models.ShrinkToFit();
    OutScene.Materials.ShrinkToFit();
    return true;
}
