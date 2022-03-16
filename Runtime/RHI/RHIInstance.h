#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHIModule.h"
#include "RHISamplerState.h"
#include "RHIViewport.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct CRHIResourceData;
struct SRHIClearValue;
class CRHIRayTracingGeometry;
class CRHIRayTracingScene;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIShadingRateTier

enum class ERHIShadingRateTier
{
    NotSupported = 0,
    Tier1        = 1,
    Tier2        = 2,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIShadingRateSupport

struct SRHIShadingRateSupport
{
    ERHIShadingRateTier Tier = ERHIShadingRateTier::NotSupported;
    uint32              ShadingRateImageTileSize = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIRayTracingTier

enum class ERHIRayTracingTier
{
    NotSupported = 0,
    Tier1        = 1,
    Tier1_1      = 2,
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRHIRayTracingSupport

struct SRHIRayTracingSupport
{
    ERHIRayTracingTier Tier;
    uint32             MaxRecursionDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIInstance

class CRHIInstance
{
public:

    /**
     * Initialize the RHI instance that the engine should be using
     * 
     * @param bEnableDebug: True if the debug-layer should be enabled
     * @return: Returns true if the initialization was successful
     */
    virtual bool Initialize(bool bEnableDebug) = 0;

    /**
     * Destroys the instance
     */
    virtual void Destroy() { delete this; }

    /**
     * Creates a Texture2D
     * 
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DRef CreateTexture2D(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Creates a Texture2DArray
     *
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DArrayRef CreateTexture2DArray(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Creates a TextureCube
     *
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeRef CreateTextureCube(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Creates a TextureCubeArray
     *
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeArrayRef CreateTextureCubeArray(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Creates a Texture3D
     *
     * @param TextureDesc: Texture description
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @return: Returns the newly created texture
     */
    virtual CRHITexture3DRef CreateTexture3D(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Creates a new Buffer
     * 
     * @param BufferDesc: Buffer description
     * @param InitialState: Initial ResurceState of the Buffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIBufferRef CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const CRHIResourceData* InitalData) = 0;

    /**
     * Create a SamplerState
     * 
     * @param Desc: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerStateRef CreateSamplerState(const class CRHISamplerStateDesc& Desc) = 0;

    /**
     * Create a new Ray tracing scene
     * 
     * @param Flags: Flags for the creation
     * @param Instances: Initial instances to create the acceleration structure with
     * @param NumInstances: Number of instances in the array
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances) = 0;
    
    /**
     * Create a new Ray tracing geometry
     *
     * @param Flags: Flags for the creation
     * @param VertexBuffer: VertexBuffer to create the acceleration structure with
     * @param NumVertices: Number of vertices in the VertexBuffer
     * @param IndexBuffer: IndexBuffer to create the acceleration structure with
     * @param NumIndices: Number of indices in the IndexBuffer
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices) = 0;

    /**
     * Create a new ShaderResourceView
     * 
     * @param Desc: Info about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceView* CreateShaderResourceView(const SRHIShaderResourceViewDesc& Desc) = 0;
    
    /**
     * Create a new UnorderedAccessView
     *
     * @param Desc: Info about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& Desc) = 0;
    
    /**
     * Create a new RenderTargetView
     *
     * @param Desc: Info about the RenderTargetView
     * @return: Returns the newly created RenderTargetView
     */
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewDesc& Desc) = 0;
    
    /**
     * Create a new DepthStencilView
     *
     * @param Desc: Info about the DepthStencilView
     * @return: Returns the newly created DepthStencilView
     */
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewDesc& Desc) = 0;

    /**
     * Creates a new Compute Shader
     * 
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIComputeShader* CreateComputeShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * Creates a new Vertex Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIVertexShader* CreateVertexShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Hull Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIHullShader* CreateHullShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Domain Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIDomainShader* CreateDomainShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Geometry Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIGeometryShader* CreateGeometryShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Mesh Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIMeshShader* CreateMeshShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Amplification Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIAmplificationShader* CreateAmplificationShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Pixel Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIPixelShader* CreatePixelShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * Creates a new Ray-Generation Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayGenShader* CreateRayGenShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Ray Any-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayAnyHitShader* CreateRayAnyHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Ray-Closest-Hit Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayClosestHitShader* CreateRayClosestHitShader(const TArray<uint8>& ShaderCode) = 0;
    
    /**
     * Creates a new Ray-Miss Shader
     *
     * @param ShaderCode: Shader byte-code to create the shader of
     * @return: Returns the newly created shader
     */
    virtual class CRHIRayMissShader* CreateRayMissShader(const TArray<uint8>& ShaderCode) = 0;

    /**
     * Create a new DepthStencilState
     * 
     * @param Desc: Info about a DepthStencilState
     * @return: Returns the newly created DepthStencilState
     */
    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SRHIDepthStencilStateDesc& Desc) = 0;

    /**
     * Create a new RasterizerState
     *
     * @param Desc: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual class CRHIRasterizerState* CreateRasterizerState(const SRHIRasterizerStateDesc& Desc) = 0;

    /**
     * Create a new BlendState
     *
     * @param Desc: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual class CRHIBlendState* CreateBlendState(const SRHIBlendStateDesc& Desc) = 0;

    /**
     * Create a new InputLayoutState
     *
     * @param Desc: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual class CRHIInputLayoutState* CreateInputLayout(const SRHIInputLayoutStateDesc& Desc) = 0;

    /**
     * Create a Graphics PipelineState
     * 
     * @param Desc: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& Desc) = 0;
    
    /**
     * Create a Compute PipelineState
     *
     * @param Desc: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SRHIComputePipelineStateDesc& Desc) = 0;
    
    /**
     * Create a Ray-Tracing PipelineState
     *
     * @param Desc: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& Desc) = 0;

    /**
     * Create a new Timestamp Query
     * 
     * @return: Returns the newly created Timestamp Query
     */
    virtual class CRHITimestampQuery* CreateTimestampQuery() = 0;


    /**
     * Create a new Viewport
     * 
     * @param Window: Window to bind to the viewport
     * @param Width: Width of the viewport
     * @param Height: Height of the viewport
     * @param ColorFormat: Format for the color
     * @param DepthFormat: Format for the depth
     * @return: Returns the newly created viewport
     */
    virtual CRHIViewportRef CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat) = 0;

    /**
     * Retrieve the default CommandContext
     * 
     * @return: Returns the default CommandContext
     */
    virtual class IRHICommandContext* GetDefaultCommandContext() = 0;

    /**
     * Retrieve the name of the Adapter
     * 
     * @return: Returns a string with the Adapter name
     */
    virtual String GetAdapterName() const
    {
        return String();
    }

    /**
     * Check for Ray tracing support
     * 
     * @param OutSupport: Struct containing the Ray tracing support for the system and current RHI
     */
    virtual void CheckRayTracingSupport(SRHIRayTracingSupport& OutSupport) const = 0;

    /**
     * Check for Shading-rate support
     *
     * @param OutSupport: Struct containing the Shading-rate support for the system and current RHI
     */
    virtual void CheckShadingRateSupport(SRHIShadingRateSupport& OutSupport) const = 0;

    /**
     * Check if the current RHI supports UnorderedAccessViews for the specified format
     * 
     * @param Format: Format to check
     * @return: Returns true if the current RHI supports UnorderedAccessViews with the specified format
     */
    virtual bool UAVSupportsFormat(ERHIFormat Format) const
    {
        UNREFERENCED_VARIABLE(Format);
        return false;
    }

    /**
     * retrieve the current API that is used
     * 
     * @return: Returns the current RHI's API
     */
    FORCEINLINE ERHIType GetApi() const
    {
        return CurrentRHI;
    }

protected:

    CRHIInstance(ERHIType InCurrentRHI)
        : CurrentRHI(InCurrentRHI)
    {
    }

    virtual ~CRHIInstance() = default;

    ERHIType CurrentRHI;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helper functions

FORCEINLINE CRHITexture2DRef RHICreateTexture2D(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTexture2D(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHITexture2DArrayRef RHICreateTexture2DArray(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTexture2DArray(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHITextureCubeRef RHICreateTextureCube(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTextureCube(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHITextureCubeArrayRef RHICreateTextureCubeArray(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTextureCubeArray(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHITexture3DRef RHICreateTexture3D(const CRHITextureDesc& TextureDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData = nullptr)
{
    return GRHIInstance->CreateTexture3D(TextureDesc, InitialState, InitialData);
}

FORCEINLINE CRHISamplerStateRef RHICreateSamplerState(const class CRHISamplerStateDesc& Desc)
{
    return GRHIInstance->CreateSamplerState(Desc);
}

FORCEINLINE CRHIBufferRef RHICreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return GRHIInstance->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateVertexBuffer(NumVertices, Stride, Flags), InitialState, InitialData);
}

template<typename T>
FORCEINLINE CRHIBufferRef RHICreateVertexBuffer(uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateVertexBuffer(sizeof(T), NumVertices, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateIndexBuffer(NumIndices, Format, Flags), InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateConstantBuffer(Size, Flags), InitialState, InitialData);
}

template<typename StructureType>
FORCEINLINE CRHIBufferRef RHICreateConstantBuffer(uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateConstantBuffer(sizeof(StructureType), Flags, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateStructuredBuffer(NumElements, Stride, Flags), InitialState, InitialData);
}

template<typename StructureType>
FORCEINLINE CRHIBufferRef RHICreateStructuredBuffer(uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const CRHIResourceData* InitialData)
{
    return RHICreateStructuredBuffer(sizeof(StructureType), NumElements, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIRayTracingScene* RHICreateRayTracingScene(uint32 Flags, SRHIRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return GRHIInstance->CreateRayTracingScene(Flags, Instances, NumInstances);
}

FORCEINLINE CRHIRayTracingGeometry* RHICreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, uint32 NumVertices, ERHIIndexFormat IndexFormat, CRHIBuffer* IndexBuffer, uint32 NumIndices)
{
    return GRHIInstance->CreateRayTracingGeometry(Flags, VertexBuffer, NumVertices, IndexFormat, IndexBuffer, NumIndices);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(const SRHIShaderResourceViewDesc& Desc)
{
    return GRHIInstance->CreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2D* Texture, ERHIFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::Texture2D);
    Desc.Texture2D.Texture    = Texture;
    Desc.Texture2D.Format     = Format;
    Desc.Texture2D.Mip        = Mip;
    Desc.Texture2D.NumMips    = NumMips;
    Desc.Texture2D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2DArray* Texture, ERHIFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::Texture2DArray);
    Desc.Texture2DArray.Texture        = Texture;
    Desc.Texture2DArray.Format         = Format;
    Desc.Texture2DArray.Mip            = Mip;
    Desc.Texture2DArray.NumMips        = NumMips;
    Desc.Texture2DArray.ArraySlice     = ArraySlice;
    Desc.Texture2DArray.NumArraySlices = NumArraySlices;
    Desc.Texture2DArray.MinMipBias     = MinMipBias;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCube* Texture, ERHIFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::TextureCube);
    Desc.TextureCube.Texture    = Texture;
    Desc.TextureCube.Format     = Format;
    Desc.TextureCube.Mip        = Mip;
    Desc.TextureCube.NumMips    = NumMips;
    Desc.TextureCube.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCubeArray* Texture, ERHIFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::TextureCubeArray);
    Desc.TextureCubeArray.Texture        = Texture;
    Desc.TextureCubeArray.Format         = Format;
    Desc.TextureCubeArray.Mip            = Mip;
    Desc.TextureCubeArray.NumMips        = NumMips;
    Desc.TextureCubeArray.ArraySlice     = ArraySlice;
    Desc.TextureCubeArray.NumArraySlices = NumArraySlices;
    Desc.TextureCubeArray.MinMipBias     = MinMipBias;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture3D* Texture, ERHIFormat Format, uint32 Mip, uint32 NumMips, uint32 DepthSlice, uint32 NumDepthSlices, float MinMipBias)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::Texture3D);
    Desc.Texture3D.Texture        = Texture;
    Desc.Texture3D.Format         = Format;
    Desc.Texture3D.Mip            = Mip;
    Desc.Texture3D.NumMips        = NumMips;
    Desc.Texture3D.DepthSlice     = DepthSlice;
    Desc.Texture3D.NumDepthSlices = NumDepthSlices;
    Desc.Texture3D.MinMipBias     = MinMipBias;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIShaderResourceViewDesc Desc(SRHIShaderResourceViewDesc::EType::VertexBuffer);
    Desc.VertexBuffer.Buffer      = Buffer;
    Desc.VertexBuffer.FirstVertex = FirstVertex;
    Desc.VertexBuffer.NumVertices = NumVertices;
    return RHICreateShaderResourceView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const SRHIUnorderedAccessViewDesc& Desc)
{
    return GRHIInstance->CreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2D* Texture, ERHIFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::Texture2D);
    Desc.Texture2D.Texture = Texture;
    Desc.Texture2D.Format  = Format;
    Desc.Texture2D.Mip     = Mip;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2DArray* Texture, ERHIFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::Texture2DArray);
    Desc.Texture2DArray.Texture        = Texture;
    Desc.Texture2DArray.Format         = Format;
    Desc.Texture2DArray.Mip            = Mip;
    Desc.Texture2DArray.ArraySlice     = ArraySlice;
    Desc.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCube* Texture, ERHIFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::TextureCube);
    Desc.TextureCube.Texture = Texture;
    Desc.TextureCube.Format  = Format;
    Desc.TextureCube.Mip     = Mip;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCubeArray* Texture, ERHIFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::TextureCubeArray);
    Desc.TextureCubeArray.Texture        = Texture;
    Desc.TextureCubeArray.Format         = Format;
    Desc.TextureCubeArray.Mip            = Mip;
    Desc.TextureCubeArray.ArraySlice     = ArraySlice;
    Desc.TextureCubeArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture3D* Texture, ERHIFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::Texture3D);
    Desc.Texture3D.Texture        = Texture;
    Desc.Texture3D.Format         = Format;
    Desc.Texture3D.Mip            = Mip;
    Desc.Texture3D.DepthSlice     = DepthSlice;
    Desc.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIUnorderedAccessViewDesc Desc(SRHIUnorderedAccessViewDesc::EType::VertexBuffer);
    Desc.VertexBuffer.Buffer      = Buffer;
    Desc.VertexBuffer.FirstVertex = FirstVertex;
    Desc.VertexBuffer.NumVertices = NumVertices;
    return RHICreateUnorderedAccessView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(const SRHIRenderTargetViewDesc& Desc)
{
    return GRHIInstance->CreateRenderTargetView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2D* Texture, ERHIFormat Format, uint32 Mip)
{
    SRHIRenderTargetViewDesc Desc(SRHIRenderTargetViewDesc::EType::Texture2D);
    Desc.Format            = Format;
    Desc.Texture2D.Texture = Texture;
    Desc.Texture2D.Mip     = Mip;
    return RHICreateRenderTargetView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2DArray* Texture, ERHIFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIRenderTargetViewDesc Desc(SRHIRenderTargetViewDesc::EType::Texture2DArray);
    Desc.Format                        = Format;
    Desc.Texture2DArray.Texture        = Texture;
    Desc.Texture2DArray.Mip            = Mip;
    Desc.Texture2DArray.ArraySlice     = ArraySlice;
    Desc.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateRenderTargetView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCube* Texture, ERHIFormat Format, ERHICubeFace CubeFace, uint32 Mip)
{
    SRHIRenderTargetViewDesc Desc(SRHIRenderTargetViewDesc::EType::TextureCube);
    Desc.Format               = Format;
    Desc.TextureCube.Texture  = Texture;
    Desc.TextureCube.Mip      = Mip;
    Desc.TextureCube.CubeFace = CubeFace;
    return RHICreateRenderTargetView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCubeArray* Texture, ERHIFormat Format, ERHICubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIRenderTargetViewDesc Desc(SRHIRenderTargetViewDesc::EType::TextureCubeArray);
    Desc.Format                      = Format;
    Desc.TextureCubeArray.Texture    = Texture;
    Desc.TextureCubeArray.Mip        = Mip;
    Desc.TextureCubeArray.ArraySlice = ArraySlice;
    Desc.TextureCubeArray.CubeFace   = CubeFace;
    return RHICreateRenderTargetView(Desc);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture3D* Texture, ERHIFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIRenderTargetViewDesc Desc(SRHIRenderTargetViewDesc::EType::Texture3D);
    Desc.Format                   = Format;
    Desc.Texture3D.Texture        = Texture;
    Desc.Texture3D.Mip            = Mip;
    Desc.Texture3D.DepthSlice     = DepthSlice;
    Desc.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateRenderTargetView(Desc);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(const SRHIDepthStencilViewDesc& Desc)
{
    return GRHIInstance->CreateDepthStencilView(Desc);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2D* Texture, ERHIFormat Format, uint32 Mip)
{
    SRHIDepthStencilViewDesc Desc(SRHIDepthStencilViewDesc::EType::Texture2D);
    Desc.Format            = Format;
    Desc.Texture2D.Texture = Texture;
    Desc.Texture2D.Mip     = Mip;
    return RHICreateDepthStencilView(Desc);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2DArray* Texture, ERHIFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIDepthStencilViewDesc Desc(SRHIDepthStencilViewDesc::EType::Texture2DArray);
    Desc.Format                        = Format;
    Desc.Texture2DArray.Texture        = Texture;
    Desc.Texture2DArray.Mip            = Mip;
    Desc.Texture2DArray.ArraySlice     = ArraySlice;
    Desc.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateDepthStencilView(Desc);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCube* Texture, ERHIFormat Format, ERHICubeFace CubeFace, uint32 Mip)
{
    SRHIDepthStencilViewDesc Desc(SRHIDepthStencilViewDesc::EType::TextureCube);
    Desc.Format               = Format;
    Desc.TextureCube.Texture  = Texture;
    Desc.TextureCube.Mip      = Mip;
    Desc.TextureCube.CubeFace = CubeFace;
    return RHICreateDepthStencilView(Desc);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCubeArray* Texture, ERHIFormat Format, ERHICubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIDepthStencilViewDesc Desc(SRHIDepthStencilViewDesc::EType::TextureCubeArray);
    Desc.Format                      = Format;
    Desc.TextureCubeArray.Texture    = Texture;
    Desc.TextureCubeArray.Mip        = Mip;
    Desc.TextureCubeArray.ArraySlice = ArraySlice;
    Desc.TextureCubeArray.CubeFace   = CubeFace;
    return RHICreateDepthStencilView(Desc);
}

FORCEINLINE CRHIComputeShader* RHICreateComputeShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateComputeShader(ShaderCode);
}

FORCEINLINE CRHIVertexShader* RHICreateVertexShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateVertexShader(ShaderCode);
}

FORCEINLINE CRHIHullShader* RHICreateHullShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateHullShader(ShaderCode);
}

FORCEINLINE CRHIDomainShader* RHICreateDomainShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateDomainShader(ShaderCode);
}

FORCEINLINE CRHIGeometryShader* RHICreateGeometryShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateGeometryShader(ShaderCode);
}

FORCEINLINE CRHIMeshShader* RHICreateMeshShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateMeshShader(ShaderCode);
}

FORCEINLINE CRHIAmplificationShader* RHICreateAmplificationShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateAmplificationShader(ShaderCode);
}

FORCEINLINE CRHIPixelShader* RHICreatePixelShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreatePixelShader(ShaderCode);
}

FORCEINLINE CRHIRayGenShader* RHICreateRayGenShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayGenShader(ShaderCode);
}

FORCEINLINE CRHIRayAnyHitShader* RHICreateRayAnyHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayAnyHitShader(ShaderCode);
}

FORCEINLINE CRHIRayClosestHitShader* RHICreateRayClosestHitShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayClosestHitShader(ShaderCode);
}

FORCEINLINE CRHIRayMissShader* RHICreateRayMissShader(const TArray<uint8>& ShaderCode)
{
    return GRHIInstance->CreateRayMissShader(ShaderCode);
}

FORCEINLINE CRHIInputLayoutState* RHICreateInputLayout(const SRHIInputLayoutStateDesc& Desc)
{
    return GRHIInstance->CreateInputLayout(Desc);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const SRHIDepthStencilStateDesc& Desc)
{
    return GRHIInstance->CreateDepthStencilState(Desc);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const SRHIRasterizerStateDesc& Desc)
{
    return GRHIInstance->CreateRasterizerState(Desc);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const SRHIBlendStateDesc& Desc)
{
    return GRHIInstance->CreateBlendState(Desc);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const SRHIComputePipelineStateDesc& Desc)
{
    return GRHIInstance->CreateComputePipelineState(Desc);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const SRHIGraphicsPipelineStateDesc& Desc)
{
    return GRHIInstance->CreateGraphicsPipelineState(Desc);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const SRHIRayTracingPipelineStateDesc& Desc)
{
    return GRHIInstance->CreateRayTracingPipelineState(Desc);
}

FORCEINLINE class CRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInstance->CreateTimestampQuery();
}

FORCEINLINE CRHIViewportRef RHICreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, ERHIFormat ColorFormat, ERHIFormat DepthFormat)
{
    return GRHIInstance->CreateViewport(WindowHandle, Width, Height, ColorFormat, DepthFormat);
}

FORCEINLINE bool RHIUAVSupportsFormat(ERHIFormat Format)
{
    return GRHIInstance->UAVSupportsFormat(Format);
}

FORCEINLINE class IRHICommandContext* RHIGetDefaultCommandContext()
{
    return GRHIInstance->GetDefaultCommandContext();
}

FORCEINLINE String RHIGetAdapterName()
{
    return GRHIInstance->GetAdapterName();
}

FORCEINLINE void RHICheckShadingRateSupport(SRHIShadingRateSupport& OutSupport)
{
    GRHIInstance->CheckShadingRateSupport(OutSupport);
}

FORCEINLINE void RHICheckRayTracingSupport(SRHIRayTracingSupport& OutSupport)
{
    GRHIInstance->CheckRayTracingSupport(OutSupport);
}

FORCEINLINE bool RHISupportsRayTracing()
{
    SRHIRayTracingSupport Support;
    RHICheckRayTracingSupport(Support);

    return (Support.Tier != ERHIRayTracingTier::NotSupported);
}

FORCEINLINE bool RHISupportsVariableRateShading()
{
    SRHIShadingRateSupport Support;
    RHICheckShadingRateSupport(Support);

    return (Support.Tier != ERHIShadingRateTier::NotSupported);
}
