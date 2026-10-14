#pragma once
#include "AssetCore/MeshStreamLayout.h"
#include "RendererCore/VertexDeclaration.h"

static_assert(MESH_MAX_STREAMS == VERTEX_MAX_STREAMS, "A mesh payload has a stream for each vertex buffer the renderer binds");

struct ENGINE_API MeshStreamLayouts
{
    /** @return Returns the attributes of the vertex declaration that reads a mesh stored with Attributes */
    NODISCARD static EVertexAttributeFlags ToVertexAttributes(EMeshVertexAttributes Attributes);

    /** @return Returns whether the declaration reads the streams exactly as the layout stores them: the same streams, strides, offsets and element sizes */
    NODISCARD static bool MatchesDeclaration(const FMeshStreamLayout& Layout, const FVertexDeclaration& Declaration);
};
