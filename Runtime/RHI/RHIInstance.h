#pragma once
#include "RHITypes.h"
#include "RHIResources.h"
#include "RHIResourceViews.h"
#include "RHICommandList.h"
#include "RHIModule.h"
#include "RHISamplerState.h"

#include "CoreApplication/Interface/PlatformWindow.h"

struct SRHIResourceData;
struct SClearValue;
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
     * @param Format: Format of the texture
     * @param Width: Width of the texture
     * @param Height: Height of the texture
     * @param NumMips: Number of MipLevels of the texture
     * @param NumSamples: Number of samples of the texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @param OptizedClearValue: Optimal clear-value for the texture
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2D* CreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) = 0;

    /**
     * Creates a Texture2DArray
     *
     * @param Format: Format of the texture
     * @param Width: Width of the texture
     * @param Height: Height of the texture
     * @param NumMips: Number of MipLevels of the texture
     * @param NumSamples: Number of samples of the texture
     * @param NumArraySlices: Number of slices in the texture-array
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @param OptizedClearValue: Optimal clear-value for the texture
     * @return: Returns the newly created texture
     */
    virtual CRHITexture2DArray* CreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) = 0;

    /**
     * Creates a TextureCube
     *
     * @param Format: Format of the texture
     * @param Size: Width and Height each side of the texture
     * @param NumMips: Number of MipLevels of the texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @param OptizedClearValue: Optimal clear-value for the texture
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCube* CreateTextureCube(EFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) = 0;

    /**
     * Creates a TextureCubeArray
     *
     * @param Format: Format of the texture
     * @param Size: Width and Height each side of the texture
     * @param NumMips: Number of MipLevels of the texture
     * @param NumArraySlices: Number of elements in the array
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @param OptizedClearValue: Optimal clear-value for the texture
     * @return: Returns the newly created texture
     */
    virtual CRHITextureCubeArray* CreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) = 0;

    /**
     * Creates a Texture3D
     *
     * @param Format: Format of the texture
     * @param Width: Width of the texture
     * @param Height: Height of the texture
     * @param Depth: Depth of the texture
     * @param NumMips: Number of MipLevels of the texture
     * @param InitialState: Initial ResourceState of the texture
     * @param InitialData: Initial data of the texture, can be nullptr
     * @param OptizedClearValue: Optimal clear-value for the texture
     * @return: Returns the newly created texture
     */
    virtual CRHITexture3D* CreateTexture3D(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitalData, const SClearValue& OptimizedClearValue) = 0;

    /**
     * Create a SamplerState
     * 
     * @param CreateInfo: Structure with information about the SamplerState
     * @return: Returns the newly created SamplerState (Could be the same as a already created sampler state and a reference is added)
     */
    virtual CRHISamplerStateRef CreateSamplerState(const struct CRHISamplerStateDesc& CreateInfo) = 0;

    /**
     * Creates a new Buffer
     * 
     * @param BufferDesc: Buffer description
     * @param InitialState: Initial ResurceState of the Buffer
     * @param InitialData: Initial data supplied to the Buffer
     * @return: Returns the newly created Buffer
     */
    virtual CRHIBufferRef CreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const SRHIResourceData* InitalData) = 0;

    /**
     * Create a new Ray tracing scene
     * 
     * @param Flags: Flags for the creation
     * @param Instances: Initial instances to create the acceleration structure with
     * @param NumInstances: Number of instances in the array
     * @return: Returns the newly created Ray tracing Scene
     */
    virtual CRHIRayTracingScene* CreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances) = 0;
    
    /**
     * Create a new Ray tracing geometry
     *
     * @param Flags: Flags for the creation
     * @param VertexBuffer: VertexBuffer the acceleration structure with
     * @param IndexBuffer: IndexBuffer the acceleration structure with
     * @return: Returns the newly created Ray tracing Geometry
     */
    virtual CRHIRayTracingGeometry* CreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer) = 0;

    /**
     * Create a new ShaderResourceView
     * 
     * @param CreateInfo: Info about the ShaderResourceView
     * @return: Returns the newly created ShaderResourceView
     */
    virtual CRHIShaderResourceView* CreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo) = 0;
    
    /**
     * Create a new UnorderedAccessView
     *
     * @param CreateInfo: Info about the UnorderedAccessView
     * @return: Returns the newly created UnorderedAccessView
     */
    virtual CRHIUnorderedAccessView* CreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo) = 0;
    
    /**
     * Create a new RenderTargetView
     *
     * @param CreateInfo: Info about the RenderTargetView
     * @return: Returns the newly created RenderTargetView
     */
    virtual CRHIRenderTargetView* CreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo) = 0;
    
    /**
     * Create a new DepthStencilView
     *
     * @param CreateInfo: Info about the DepthStencilView
     * @return: Returns the newly created DepthStencilView
     */
    virtual CRHIDepthStencilView* CreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo) = 0;

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
     * @param CreateInfo: Info about a DepthStencilState
     * @return: Returns the newly created DepthStencilState
     */
    virtual class CRHIDepthStencilState* CreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo) = 0;

    /**
     * Create a new RasterizerState
     *
     * @param CreateInfo: Info about a RasterizerState
     * @return: Returns the newly created RasterizerState
     */
    virtual class CRHIRasterizerState* CreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo) = 0;

    /**
     * Create a new BlendState
     *
     * @param CreateInfo: Info about a BlendState
     * @return: Returns the newly created BlendState
     */
    virtual class CRHIBlendState* CreateBlendState(const SRHIBlendStateInfo& CreateInfo) = 0;

    /**
     * Create a new InputLayoutState
     *
     * @param CreateInfo: Info about a InputLayoutState
     * @return: Returns the newly created InputLayoutState
     */
    virtual class CRHIInputLayoutState* CreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo) = 0;

    /**
     * Create a Graphics PipelineState
     * 
     * @param CreateInfo: Info about the Graphics PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIGraphicsPipelineState* CreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo) = 0;
    
    /**
     * Create a Compute PipelineState
     *
     * @param CreateInfo: Info about the Compute PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIComputePipelineState* CreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo) = 0;
    
    /**
     * Create a Ray-Tracing PipelineState
     *
     * @param CreateInfo: Info about the Ray-Tracing PipelineState
     * @return: Returns the newly created PipelineState
     */
    virtual class CRHIRayTracingPipelineState* CreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo) = 0;

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
    virtual class CRHIViewport* CreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat) = 0;

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
    virtual bool UAVSupportsFormat(EFormat Format) const
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

FORCEINLINE CRHITexture2D* RHICreateTexture2D(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData = nullptr, const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInstance->CreateTexture2D(Format, Width, Height, NumMips, NumSamples, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITexture2DArray* RHICreateTexture2DArray(EFormat Format, uint32 Width, uint32 Height, uint32 NumMips, uint32 NumSamples, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData = nullptr, const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInstance->CreateTexture2DArray(Format, Width, Height, NumMips, NumSamples, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITextureCube* RHICreateTextureCube(EFormat Format, uint32 Size, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData = nullptr, const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInstance->CreateTextureCube(Format, Size, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITextureCubeArray* RHICreateTextureCubeArray(EFormat Format, uint32 Size, uint32 NumMips, uint32 NumArraySlices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData = nullptr, const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInstance->CreateTextureCubeArray(Format, Size, NumMips, NumArraySlices, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHITexture3D* RHICreateTexture3D(EFormat Format, uint32 Width, uint32 Height, uint32 Depth, uint32 NumMips, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData = nullptr, const SClearValue& OptimizedClearValue = SClearValue())
{
    return GRHIInstance->CreateTexture3D(Format, Width, Height, Depth, NumMips, Flags, InitialState, InitialData, OptimizedClearValue);
}

FORCEINLINE CRHISamplerStateRef RHICreateSamplerState(const class CRHISamplerStateDesc& CreateInfo)
{
    return GRHIInstance->CreateSamplerState(CreateInfo);
}

FORCEINLINE CRHIBufferRef RHICreateBuffer(const CRHIBufferDesc& BufferDesc, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
	return GRHIInstance->CreateBuffer(BufferDesc, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateVertexBuffer(uint32 Stride, uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateVertexBuffer(NumVertices, Stride, Flags), InitialState, InitialData);
}

template<typename T>
FORCEINLINE CRHIBufferRef RHICreateVertexBuffer(uint32 NumVertices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateVertexBuffer(sizeof(T), NumVertices, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateIndexBuffer(ERHIIndexFormat Format, uint32 NumIndices, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateIndexBuffer(NumIndices, Format, Flags), InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateConstantBuffer(uint32 Size, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateConstantBuffer(Size, Flags), InitialState, InitialData);
}

template<typename StructureType>
FORCEINLINE CRHIBufferRef RHICreateConstantBuffer(uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateConstantBuffer(sizeof(StructureType), Flags, InitialState, InitialData);
}

FORCEINLINE CRHIBufferRef RHICreateStructuredBuffer(uint32 Stride, uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateBuffer(CRHIBufferDesc::CreateStructuredBuffer(NumElements, Stride, Flags), InitialState, InitialData);
}

template<typename StructureType>
FORCEINLINE CRHIBufferRef RHICreateStructuredBuffer(uint32 NumElements, uint32 Flags, ERHIResourceState InitialState, const SRHIResourceData* InitialData)
{
    return RHICreateStructuredBuffer(sizeof(StructureType), NumElements, Flags, InitialState, InitialData);
}

FORCEINLINE CRHIRayTracingScene* RHICreateRayTracingScene(uint32 Flags, SRayTracingGeometryInstance* Instances, uint32 NumInstances)
{
    return GRHIInstance->CreateRayTracingScene(Flags, Instances, NumInstances);
}

FORCEINLINE CRHIRayTracingGeometry* RHICreateRayTracingGeometry(uint32 Flags, CRHIBuffer* VertexBuffer, CRHIBuffer* IndexBuffer)
{
    return GRHIInstance->CreateRayTracingGeometry(Flags, VertexBuffer, IndexBuffer);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(const SRHIShaderResourceViewInfo& CreateInfo)
{
    return GRHIInstance->CreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    CreateInfo.Texture2D.NumMips = NumMips;
    CreateInfo.Texture2D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.NumMips = NumMips;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    CreateInfo.Texture2DArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCube* Texture, EFormat Format, uint32 Mip, uint32 NumMips, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.NumMips = NumMips;
    CreateInfo.TextureCube.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 ArraySlice, uint32 NumArraySlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.NumMips = NumMips;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    CreateInfo.TextureCubeArray.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 NumMips, uint32 DepthSlice, uint32 NumDepthSlices, float MinMipBias)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.NumMips = NumMips;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    CreateInfo.Texture3D.MinMipBias = MinMipBias;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIShaderResourceView* RHICreateShaderResourceView(CRHIBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIShaderResourceViewInfo CreateInfo(SRHIShaderResourceViewInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateShaderResourceView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(const SRHIUnorderedAccessViewInfo& CreateInfo)
{
    return GRHIInstance->CreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture2D);
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Format = Format;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture2DArray);
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Format = Format;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCube* Texture, EFormat Format, uint32 Mip)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::TextureCube);
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Format = Format;
    CreateInfo.TextureCube.Mip = Mip;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITextureCubeArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::TextureCubeArray);
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Format = Format;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.NumArraySlices = NumArraySlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::Texture3D);
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Format = Format;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIUnorderedAccessView* RHICreateUnorderedAccessView(CRHIBuffer* Buffer, uint32 FirstVertex, uint32 NumVertices)
{
    SRHIUnorderedAccessViewInfo CreateInfo(SRHIUnorderedAccessViewInfo::EType::VertexBuffer);
    CreateInfo.VertexBuffer.Buffer = Buffer;
    CreateInfo.VertexBuffer.FirstVertex = FirstVertex;
    CreateInfo.VertexBuffer.NumVertices = NumVertices;
    return RHICreateUnorderedAccessView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(const SRHIRenderTargetViewInfo& CreateInfo)
{
    return GRHIInstance->CreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCube* Texture, EFormat Format, ERHICubeFace CubeFace, uint32 Mip)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITextureCubeArray* Texture, EFormat Format, ERHICubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIRenderTargetView* RHICreateRenderTargetView(CRHITexture3D* Texture, EFormat Format, uint32 Mip, uint32 DepthSlice, uint32 NumDepthSlices)
{
    SRHIRenderTargetViewInfo CreateInfo(SRHIRenderTargetViewInfo::EType::Texture3D);
    CreateInfo.Format = Format;
    CreateInfo.Texture3D.Texture = Texture;
    CreateInfo.Texture3D.Mip = Mip;
    CreateInfo.Texture3D.DepthSlice = DepthSlice;
    CreateInfo.Texture3D.NumDepthSlices = NumDepthSlices;
    return RHICreateRenderTargetView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(const SRHIDepthStencilViewInfo& CreateInfo)
{
    return GRHIInstance->CreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2D* Texture, EFormat Format, uint32 Mip)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::Texture2D);
    CreateInfo.Format = Format;
    CreateInfo.Texture2D.Texture = Texture;
    CreateInfo.Texture2D.Mip = Mip;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITexture2DArray* Texture, EFormat Format, uint32 Mip, uint32 ArraySlice, uint32 NumArraySlices)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::Texture2DArray);
    CreateInfo.Format = Format;
    CreateInfo.Texture2DArray.Texture = Texture;
    CreateInfo.Texture2DArray.Mip = Mip;
    CreateInfo.Texture2DArray.ArraySlice = ArraySlice;
    CreateInfo.Texture2DArray.NumArraySlices = NumArraySlices;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCube* Texture, EFormat Format, ERHICubeFace CubeFace, uint32 Mip)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::TextureCube);
    CreateInfo.Format = Format;
    CreateInfo.TextureCube.Texture = Texture;
    CreateInfo.TextureCube.Mip = Mip;
    CreateInfo.TextureCube.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
}

FORCEINLINE CRHIDepthStencilView* RHICreateDepthStencilView(CRHITextureCubeArray* Texture, EFormat Format, ERHICubeFace CubeFace, uint32 Mip, uint32 ArraySlice)
{
    SRHIDepthStencilViewInfo CreateInfo(SRHIDepthStencilViewInfo::EType::TextureCubeArray);
    CreateInfo.Format = Format;
    CreateInfo.TextureCubeArray.Texture = Texture;
    CreateInfo.TextureCubeArray.Mip = Mip;
    CreateInfo.TextureCubeArray.ArraySlice = ArraySlice;
    CreateInfo.TextureCubeArray.CubeFace = CubeFace;
    return RHICreateDepthStencilView(CreateInfo);
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

FORCEINLINE CRHIInputLayoutState* RHICreateInputLayout(const SRHIInputLayoutStateInfo& CreateInfo)
{
    return GRHIInstance->CreateInputLayout(CreateInfo);
}

FORCEINLINE CRHIDepthStencilState* RHICreateDepthStencilState(const SRHIDepthStencilStateInfo& CreateInfo)
{
    return GRHIInstance->CreateDepthStencilState(CreateInfo);
}

FORCEINLINE CRHIRasterizerState* RHICreateRasterizerState(const SRHIRasterizerStateInfo& CreateInfo)
{
    return GRHIInstance->CreateRasterizerState(CreateInfo);
}

FORCEINLINE CRHIBlendState* RHICreateBlendState(const SRHIBlendStateInfo& CreateInfo)
{
    return GRHIInstance->CreateBlendState(CreateInfo);
}

FORCEINLINE CRHIComputePipelineState* RHICreateComputePipelineState(const SRHIComputePipelineStateInfo& CreateInfo)
{
    return GRHIInstance->CreateComputePipelineState(CreateInfo);
}

FORCEINLINE CRHIGraphicsPipelineState* RHICreateGraphicsPipelineState(const SRHIGraphicsPipelineStateInfo& CreateInfo)
{
    return GRHIInstance->CreateGraphicsPipelineState(CreateInfo);
}

FORCEINLINE CRHIRayTracingPipelineState* RHICreateRayTracingPipelineState(const SRHIRayTracingPipelineStateInfo& CreateInfo)
{
    return GRHIInstance->CreateRayTracingPipelineState(CreateInfo);
}

FORCEINLINE class CRHITimestampQuery* RHICreateTimestampQuery()
{
    return GRHIInstance->CreateTimestampQuery();
}

FORCEINLINE class CRHIViewport* RHICreateViewport(PlatformWindowHandle WindowHandle, uint32 Width, uint32 Height, EFormat ColorFormat, EFormat DepthFormat)
{
    return GRHIInstance->CreateViewport(WindowHandle, Width, Height, ColorFormat, DepthFormat);
}

FORCEINLINE bool RHIUAVSupportsFormat(EFormat Format)
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
