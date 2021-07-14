#include "OBJLoader.h"
#include "MeshUtilities.h"

#include "Math/MathCommon.h"

#include "Core/Containers/HashTable.h"

#include <tiny_obj_loader.h>

bool COBJLoader::LoadFile( const String& Filename, SSceneData& OutScene, bool ReverseHandedness )
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

    std::string MTLFiledir = std::string( Filename.begin(), Filename.begin() + Filename.find_last_of( '/' ) );
    if ( !tinyobj::LoadObj( &Attributes, &Shapes, &Materials, &Warning, &Error, Filename.c_str(), MTLFiledir.c_str(), true, false ) )
    {
        LOG_WARNING( "[COBJLoader]: Failed to load '" + Filename + "'." + " Warning: " + Warning + " Error: " + Error );
        return false;
    }
    else
    {
        LOG_INFO( "[COBJLoader]: Loaded '" + Filename + "'" );
    }

    // Create All Materials in scene
    for ( tinyobj::material_t& Mat : Materials )
    {
        // Create new material with default properties
        SMaterialData MaterialData;
        //MaterialData.Metallic = Mat.ambient[0];
        //MaterialData.Diffuse = XMFLOAT3( Mat.diffuse[0], Mat.diffuse[1], Mat.diffuse[2] );
        //MaterialData.AO = 1.0f;
        //MaterialData.Roughness = 1.0f;
        //MaterialData.TexPath = MTLFiledir + '/';

        // Metallic
        if ( !Mat.ambient_texname.empty() )
        {
            //ConvertBackslashes( Mat.ambient_texname );
            //MaterialData.MetallicTexname = Mat.ambient_texname;
        }

        // Albedo
        if ( !Mat.diffuse_texname.empty() )
        {
            //ConvertBackslashes( Mat.diffuse_texname );
            //MaterialData.DiffTexName = Mat.diffuse_texname;
        }

        // Roughness
        if ( !Mat.specular_highlight_texname.empty() )
        {
            //ConvertBackslashes( Mat.specular_highlight_texname );
            //MaterialData.RoughnessTexname = Mat.specular_highlight_texname;
        }

        // Normal
        if ( !Mat.bump_texname.empty() )
        {
            //ConvertBackslashes( Mat.bump_texname );
            //MaterialData.NormalTexname = Mat.bump_texname;
        }

        // Alpha
        if ( !Mat.alpha_texname.empty() )
        {
            //ConvertBackslashes( Mat.alpha_texname );
            //MaterialData.AlphaTexname = Mat.alpha_texname;
        }

        OutScene.Materials.EmplaceBack( MaterialData );
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

            uint32 Face = i / 3;
            const uint32 MaterialID = Shape.mesh.material_ids[Face];
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
                TempVertex.Position = CVector3( Attributes.vertices[PositionIndex + 0], Attributes.vertices[PositionIndex + 1], Attributes.vertices[PositionIndex + 2]);

                if ( Index.normal_index >= 0 )
                {
                    auto NormalIndex = 3 * Index.normal_index;
                    TempVertex.Normal = CVector3( Attributes.normals[NormalIndex + 0], Attributes.normals[NormalIndex + 1], Attributes.normals[NormalIndex + 2]);
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
                    Data.Mesh.Vertices.PushBack( TempVertex );
                }

                Data.Mesh.Indices.EmplaceBack( UniqueVertices[TempVertex] );
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

            Data.Name = Shape.name;
            OutScene.Models.EmplaceBack( Data );
        }
    }

    OutScene.Models.ShrinkToFit();
    OutScene.Materials.ShrinkToFit();
    return true;
}


