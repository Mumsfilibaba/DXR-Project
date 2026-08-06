#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/Array.h"
#include "Engine/EngineModule.h"
#include "Engine/Assets/VertexFormat.h"
#include "RendererCore/VertexDeclaration.h"

struct FSubMesh
{
    FSubMesh()
        : BaseVertex(0)
        , VertexCount(0)
        , StartIndex(0)
        , IndexCount(0)
        , MaterialIndex(-1)
    {
    }
    
    uint32 BaseVertex;
    uint32 VertexCount;
    uint32 StartIndex;
    uint32 IndexCount;
    int32  MaterialIndex;
};

struct ENGINE_API FMeshData
{
    FMeshData();
    FMeshData(const FMeshData& Other);
    FMeshData(FMeshData&& Other);
    ~FMeshData();

    FMeshData& operator=(const FMeshData& Other);
    FMeshData& operator=(FMeshData&& Other);

    void Subdivide(uint32 Subdivisions = 1);
    void Optimize(uint32 StartVertex = 0);

    void CalculateHardNormals();
    void CalculateSoftNormals();
    void CalculateTangents();
    void CalculateTangentSigns();
    void SplitTangentSeams();

    void ValidateTangents();
    void ReverseHandedness();
    void InvertAxisX();

    bool PackVertexStreams(TArray<uint8> (&OutStreams)[VERTEX_MAX_STREAMS]) const;

    TArray<uint16> GetSmallIndices() const;

    String                Name;
    TArray<FSubMesh>      SubMeshes;
    TArray<uint32>        Indices;
    TArray<FSourceVertex> Vertices;
    FVertexDeclaration    Declaration;
    TArray<uint8>         PackedStreams[VERTEX_MAX_STREAMS];
    int32                 PackedVertexCount;
};
