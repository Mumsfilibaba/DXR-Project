#if PLATFORM_WINDOWS
#include "ShaderCompiler/Reflection/D3DReflectionUtils.h"

struct D3DShaderRequires
{
    static constexpr uint64 EarlyDepthStencil                   = 0x00000002;
    static constexpr uint64 TiledResources                      = 0x00000100;
    static constexpr uint64 StencilRef                          = 0x00000200;
    static constexpr uint64 InnerCoverage                       = 0x00000400;
    static constexpr uint64 TypedUAVLoadAdditionalFormats       = 0x00000800;
    static constexpr uint64 ROVs                                = 0x00001000;
    static constexpr uint64 VPAndRTArrayIndexFromAnyShader      = 0x00002000;
    static constexpr uint64 WaveOps                             = 0x00004000;
    static constexpr uint64 Int64Ops                            = 0x00008000;
    static constexpr uint64 ViewID                              = 0x00010000;
    static constexpr uint64 Barycentrics                        = 0x00020000;
    static constexpr uint64 Native16BitOps                      = 0x00040000;
    static constexpr uint64 ShadingRate                         = 0x00080000;
    static constexpr uint64 RaytracingTier1_1                   = 0x00100000;
    static constexpr uint64 SamplerFeedback                     = 0x00200000;
    static constexpr uint64 AtomicInt64OnTypedResource          = 0x00400000;
    static constexpr uint64 AtomicInt64OnGroupShared            = 0x00800000;
    static constexpr uint64 DerivativesInMeshAndAmpShaders      = 0x01000000;
    static constexpr uint64 ResourceDescriptorHeapIndexing      = 0x02000000;
    static constexpr uint64 SamplerDescriptorHeapIndexing       = 0x04000000;
    static constexpr uint64 WaveMMA                             = 0x08000000;
    static constexpr uint64 AtomicInt64OnDescriptorHeapResource = 0x10000000;
};

bool FD3DReflectionUtils::TranslateInputType(D3D_SHADER_INPUT_TYPE Type, D3D_SRV_DIMENSION Dimension, EShaderResourceType& OutType, bool& bOutHasCounter)
{
    bOutHasCounter = false;

    switch (Type)
    {
        case D3D_SIT_CBUFFER:                 OutType = EShaderResourceType::ConstantBuffer;        return true;
        case D3D_SIT_SAMPLER:                 OutType = EShaderResourceType::Sampler;               return true;
        case D3D_SIT_TBUFFER:                 OutType = EShaderResourceType::TypedBuffer;           return true;
        case D3D_SIT_STRUCTURED:              OutType = EShaderResourceType::StructuredBuffer;      return true;
        case D3D_SIT_BYTEADDRESS:             OutType = EShaderResourceType::ByteAddressBuffer;     return true;
        case D3D_SIT_RTACCELERATIONSTRUCTURE: OutType = EShaderResourceType::AccelerationStructure; return true;
        case D3D_SIT_UAV_RWSTRUCTURED:        OutType = EShaderResourceType::RWStructuredBuffer;    return true;
        case D3D_SIT_UAV_RWBYTEADDRESS:       OutType = EShaderResourceType::RWByteAddressBuffer;   return true;

        // Buffer<T> and RWBuffer<T> reflect as textures, only the dimension separates them
        case D3D_SIT_TEXTURE:     OutType = Dimension == D3D_SRV_DIMENSION_BUFFER ? EShaderResourceType::TypedBuffer   : EShaderResourceType::Texture;   return true;
        case D3D_SIT_UAV_RWTYPED: OutType = Dimension == D3D_SRV_DIMENSION_BUFFER ? EShaderResourceType::RWTypedBuffer : EShaderResourceType::RWTexture; return true;

        case D3D_SIT_UAV_APPEND_STRUCTURED:
        case D3D_SIT_UAV_CONSUME_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        {
            OutType        = EShaderResourceType::RWStructuredBuffer;
            bOutHasCounter = true;
            return true;
        }

        default:
        {
            return false;
        }
    }
}

EShaderResourceDimension FD3DReflectionUtils::TranslateDimension(D3D_SRV_DIMENSION Dimension)
{
    switch (Dimension)
    {
        case D3D_SRV_DIMENSION_BUFFER:           return EShaderResourceDimension::Buffer;
        case D3D_SRV_DIMENSION_BUFFEREX:         return EShaderResourceDimension::BufferEx;
        case D3D_SRV_DIMENSION_TEXTURE1D:        return EShaderResourceDimension::Texture1D;
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:   return EShaderResourceDimension::Texture1DArray;
        case D3D_SRV_DIMENSION_TEXTURE2D:        return EShaderResourceDimension::Texture2D;
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:   return EShaderResourceDimension::Texture2DArray;
        case D3D_SRV_DIMENSION_TEXTURE2DMS:      return EShaderResourceDimension::Texture2DMS;
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: return EShaderResourceDimension::Texture2DMSArray;
        case D3D_SRV_DIMENSION_TEXTURE3D:        return EShaderResourceDimension::Texture3D;
        case D3D_SRV_DIMENSION_TEXTURECUBE:      return EShaderResourceDimension::TextureCube;
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: return EShaderResourceDimension::TextureCubeArray;
        default:                                 return EShaderResourceDimension::Unknown;
    }
}

EShaderFeatureFlags FD3DReflectionUtils::TranslateRequiresFlags(uint64 Mask)
{
    struct FFlagMapping
    {
        uint64              Mask;
        EShaderFeatureFlags Flag;
    };

    static constexpr FFlagMapping Mappings[] =
    {
        { D3DShaderRequires::ResourceDescriptorHeapIndexing,      EShaderFeatureFlags::RequiresResourceDescriptorHeapIndexing },
        { D3DShaderRequires::SamplerDescriptorHeapIndexing,       EShaderFeatureFlags::RequiresSamplerDescriptorHeapIndexing },
        { D3DShaderRequires::EarlyDepthStencil,                   EShaderFeatureFlags::RequiresEarlyDepthStencil },
        { D3DShaderRequires::StencilRef,                          EShaderFeatureFlags::RequiresStencilRef },
        { D3DShaderRequires::InnerCoverage,                       EShaderFeatureFlags::RequiresInnerCoverage },
        { D3DShaderRequires::ROVs,                                EShaderFeatureFlags::RequiresROVs },
        { D3DShaderRequires::WaveOps,                             EShaderFeatureFlags::RequiresWaveOps },
        { D3DShaderRequires::Int64Ops,                            EShaderFeatureFlags::RequiresInt64Ops },
        { D3DShaderRequires::Native16BitOps,                      EShaderFeatureFlags::RequiresNative16BitOps },
        { D3DShaderRequires::Barycentrics,                        EShaderFeatureFlags::RequiresBarycentrics },
        { D3DShaderRequires::ViewID,                              EShaderFeatureFlags::RequiresViewID },
        { D3DShaderRequires::ShadingRate,                         EShaderFeatureFlags::RequiresShadingRate },
        { D3DShaderRequires::RaytracingTier1_1,                   EShaderFeatureFlags::RequiresRaytracingTier1_1 },
        { D3DShaderRequires::SamplerFeedback,                     EShaderFeatureFlags::RequiresSamplerFeedback },
        { D3DShaderRequires::TiledResources,                      EShaderFeatureFlags::RequiresTiledResources },
        { D3DShaderRequires::TypedUAVLoadAdditionalFormats,       EShaderFeatureFlags::RequiresTypedUAVLoadAdditionalFormats },
        { D3DShaderRequires::VPAndRTArrayIndexFromAnyShader,      EShaderFeatureFlags::RequiresVPAndRTArrayIndexFromAnyShader },
        { D3DShaderRequires::AtomicInt64OnTypedResource,          EShaderFeatureFlags::RequiresAtomicInt64OnTypedResource },
        { D3DShaderRequires::AtomicInt64OnGroupShared,            EShaderFeatureFlags::RequiresAtomicInt64OnGroupShared },
        { D3DShaderRequires::AtomicInt64OnDescriptorHeapResource, EShaderFeatureFlags::RequiresAtomicInt64OnDescriptorHeapResource },
        { D3DShaderRequires::WaveMMA,                             EShaderFeatureFlags::RequiresWaveMMA },
        { D3DShaderRequires::DerivativesInMeshAndAmpShaders,      EShaderFeatureFlags::RequiresDerivativesInMeshAndAmpShaders },
    };

    EShaderFeatureFlags Result = EShaderFeatureFlags::None;
    for (const FFlagMapping& Mapping : Mappings)
    {
        if ((Mask & Mapping.Mask) != 0)
        {
            Result |= Mapping.Flag;
        }
    }

    return Result;
}

EShaderComponentType FD3DReflectionUtils::TranslateComponentType(D3D_REGISTER_COMPONENT_TYPE ComponentType, D3D_MIN_PRECISION MinPrecision)
{
    switch (ComponentType)
    {
        case D3D_REGISTER_COMPONENT_FLOAT32:
        {
            const bool bHalf = MinPrecision == D3D_MIN_PRECISION_FLOAT_16 || MinPrecision == D3D_MIN_PRECISION_FLOAT_2_8;
            return bHalf ? EShaderComponentType::Float16 : EShaderComponentType::Float32;
        }

        case D3D_REGISTER_COMPONENT_SINT32:
        {
            return MinPrecision == D3D_MIN_PRECISION_SINT_16 ? EShaderComponentType::Int16 : EShaderComponentType::Int32;
        }

        case D3D_REGISTER_COMPONENT_UINT32:
        {
            return MinPrecision == D3D_MIN_PRECISION_UINT_16 ? EShaderComponentType::Uint16 : EShaderComponentType::Uint32;
        }

        default:
        {
            return EShaderComponentType::Unknown;
        }
    }
}

#endif
