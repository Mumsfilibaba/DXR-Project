#include "D3D12RootSignature.h"
#include "D3D12Device.h"

// TODO: Redo similar to raytracing pipelinestream
Bool D3D12DefaultRootSignatures::CreateRootSignatures(D3D12Device* Device)
{
    constexpr UInt32 ShaderRegisterOffset32BitConstants = 1;

    D3D12_DESCRIPTOR_RANGE CBVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
    Memory::Memzero(CBVRanges, sizeof(CBVRanges));

    D3D12_DESCRIPTOR_RANGE SRVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
    Memory::Memzero(SRVRanges, sizeof(SRVRanges));

    D3D12_DESCRIPTOR_RANGE UAVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
    Memory::Memzero(UAVRanges, sizeof(UAVRanges));

    D3D12_DESCRIPTOR_RANGE SamplerRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
    Memory::Memzero(SamplerRanges, sizeof(SamplerRanges));
    
    for (UInt32 i = 0; i < D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT; i++)
    {
        CBVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        CBVRanges[i].BaseShaderRegister                = ShaderRegisterOffset32BitConstants + i;
        CBVRanges[i].NumDescriptors                    = 1;
        CBVRanges[i].RegisterSpace                     = 0;
        CBVRanges[i].OffsetInDescriptorsFromTableStart = i;

        SRVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        SRVRanges[i].BaseShaderRegister                = i;
        SRVRanges[i].NumDescriptors                    = 1;
        SRVRanges[i].RegisterSpace                     = 0;
        SRVRanges[i].OffsetInDescriptorsFromTableStart = i;

        UAVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        UAVRanges[i].BaseShaderRegister                = i;
        UAVRanges[i].NumDescriptors                    = 1;
        UAVRanges[i].RegisterSpace                     = 0;
        UAVRanges[i].OffsetInDescriptorsFromTableStart = i;

        SamplerRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        SamplerRanges[i].BaseShaderRegister                = i;
        SamplerRanges[i].NumDescriptors                    = 1;
        SamplerRanges[i].RegisterSpace                     = 0;
        SamplerRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    // Graphics
    // 1 For 32 bit constants and 4, one for each type of resource, CBV, SRV, UAV, Samplers
    constexpr UInt32 NumParameters = 1 + 4;

    D3D12_ROOT_PARAMETER GraphicsRootParameters[NumParameters];
    Memory::Memzero(GraphicsRootParameters, sizeof(GraphicsRootParameters));

    GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.Num32BitValues = D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT;
    GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.RegisterSpace  = 0;
    GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.ShaderRegister = 0;
    GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

    for (UInt32 i = 1; i < NumParameters; i++)
    {
        GraphicsRootParameters[i].ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        GraphicsRootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        GraphicsRootParameters[i].DescriptorTable.NumDescriptorRanges = D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
    }

    GraphicsRootParameters[D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges       = CBVRanges;
    GraphicsRootParameters[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges  = SRVRanges;
    GraphicsRootParameters[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges = UAVRanges;
    GraphicsRootParameters[D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges         = SamplerRanges;

    D3D12_ROOT_SIGNATURE_DESC GraphicsRootDesc;
    Memory::Memzero(&GraphicsRootDesc);

    GraphicsRootDesc.NumParameters = NumParameters;
    GraphicsRootDesc.pParameters   = GraphicsRootParameters;
    GraphicsRootDesc.Flags = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    Graphics = Device->CreateRootSignature(GraphicsRootDesc);
    if (Graphics)
    {
        LOG_INFO("Created Default Graphics RootSignature");
        Graphics->SetName("Default Graphics RootSignature");
    }
    else
    {
        return false;
    }

    // Compute
    D3D12_ROOT_PARAMETER ComputeRootParameters[NumParameters];
    Memory::Memzero(ComputeRootParameters, sizeof(ComputeRootParameters));

    ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.Num32BitValues = D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT;
    ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.RegisterSpace  = 0;
    ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.ShaderRegister = 0;
    ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ShaderVisibility         = D3D12_SHADER_VISIBILITY_ALL;

    for (UInt32 i = 1; i < NumParameters; i++)
    {
        ComputeRootParameters[i].ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        ComputeRootParameters[i].ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;
        ComputeRootParameters[i].DescriptorTable.NumDescriptorRanges = D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
    }

    ComputeRootParameters[D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges       = CBVRanges;
    ComputeRootParameters[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges  = SRVRanges;
    ComputeRootParameters[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges = UAVRanges;
    ComputeRootParameters[D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges         = SamplerRanges;

    D3D12_ROOT_SIGNATURE_DESC ComputeRootDesc;
    Memory::Memzero(&ComputeRootDesc);

    ComputeRootDesc.NumParameters = NumParameters;
    ComputeRootDesc.pParameters   = ComputeRootParameters;
    ComputeRootDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS   |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS   |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    if (Device->GetMeshShaderTier() != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        ComputeRootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
    }

    Compute = Device->CreateRootSignature(ComputeRootDesc);
    if (Compute)
    {
        LOG_INFO("Created Default Compute RootSignature");
        Compute->SetName("Default Compute RootSignature");
    }
    else
    {
        return false;
    }

    // End here if raytracing is not supported
    if (Device->GetRayTracingTier() == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        return true;
    }

    // RayTracing Global
    constexpr UInt32 NumGlobalParameters = 4;

    D3D12_ROOT_PARAMETER RayGenRootParameters[NumGlobalParameters];
    Memory::Memzero(RayGenRootParameters, sizeof(RayGenRootParameters));

    for (UInt32 i = 0; i < NumGlobalParameters; i++)
    {
        RayGenRootParameters[i].ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        RayGenRootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        RayGenRootParameters[i].DescriptorTable.NumDescriptorRanges = D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
    }

    D3D12_DESCRIPTOR_RANGE GlobalCBVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
    Memory::Memzero(GlobalCBVRanges, sizeof(GlobalCBVRanges));

    for (UInt32 i = 0; i < D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT; i++)
    {
        GlobalCBVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        GlobalCBVRanges[i].BaseShaderRegister                = i;
        GlobalCBVRanges[i].NumDescriptors                    = 1;
        GlobalCBVRanges[i].RegisterSpace                     = 0;
        GlobalCBVRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    RayGenRootParameters[0].DescriptorTable.pDescriptorRanges = GlobalCBVRanges;
    RayGenRootParameters[1].DescriptorTable.pDescriptorRanges = SRVRanges;
    RayGenRootParameters[2].DescriptorTable.pDescriptorRanges = UAVRanges;
    RayGenRootParameters[3].DescriptorTable.pDescriptorRanges = SamplerRanges;

    D3D12_ROOT_SIGNATURE_DESC RayGenRootDesc;
    Memory::Memzero(&RayGenRootDesc);

    RayGenRootDesc.NumParameters = NumGlobalParameters;
    RayGenRootDesc.pParameters   = RayGenRootParameters;
    RayGenRootDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS   |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS     |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS   |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    if (Device->GetMeshShaderTier() != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
    {
        RayGenRootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
    }

    GlobalRayGen = Device->CreateRootSignature(RayGenRootDesc);
    if (GlobalRayGen)
    {
        LOG_INFO("Created Default GlobalRayGen RootSignature");
        GlobalRayGen->SetName("Default GlobalRayGen RootSignature");
    }
    else
    {
        return false;
    }

    // RayTracing Local
    constexpr UInt32 NumLocalCBV = 4;
    D3D12_DESCRIPTOR_RANGE LocalCBVRanges[NumLocalCBV];
    Memory::Memzero(LocalCBVRanges, sizeof(LocalCBVRanges));

    for (UInt32 i = 0; i < NumLocalCBV; i++)
    {
        LocalCBVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        LocalCBVRanges[i].BaseShaderRegister                = i;
        LocalCBVRanges[i].NumDescriptors                    = 1;
        LocalCBVRanges[i].RegisterSpace                     = 1;
        LocalCBVRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    constexpr UInt32 NumLocalSRV = 8;
    D3D12_DESCRIPTOR_RANGE LocalSRVRanges[NumLocalSRV];
    Memory::Memzero(LocalSRVRanges, sizeof(LocalSRVRanges));

    for (UInt32 i = 0; i < NumLocalSRV; i++)
    {
        LocalSRVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        LocalSRVRanges[i].BaseShaderRegister                = i;
        LocalSRVRanges[i].NumDescriptors                    = 1;
        LocalSRVRanges[i].RegisterSpace                     = 1;
        LocalSRVRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    constexpr UInt32 NumLocalUAV = 4;
    D3D12_DESCRIPTOR_RANGE LocalUAVRanges[NumLocalUAV];
    Memory::Memzero(LocalUAVRanges, sizeof(LocalUAVRanges));

    for (UInt32 i = 0; i < NumLocalUAV; i++)
    {
        LocalUAVRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        LocalUAVRanges[i].BaseShaderRegister                = i;
        LocalUAVRanges[i].NumDescriptors                    = 1;
        LocalUAVRanges[i].RegisterSpace                     = 1;
        LocalUAVRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    constexpr UInt32 NumLocalSamplers = 4;
    D3D12_DESCRIPTOR_RANGE LocalSamplerRanges[NumLocalSamplers];
    Memory::Memzero(LocalSamplerRanges, sizeof(LocalSamplerRanges));

    for (UInt32 i = 0; i < NumLocalSamplers; i++)
    {
        LocalSamplerRanges[i].RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        LocalSamplerRanges[i].BaseShaderRegister                = i;
        LocalSamplerRanges[i].NumDescriptors                    = 1;
        LocalSamplerRanges[i].RegisterSpace                     = 1;
        LocalSamplerRanges[i].OffsetInDescriptorsFromTableStart = i;
    }

    constexpr UInt32 NumLocalParameters = 4;

    D3D12_ROOT_PARAMETER RayLocalRootParameters[NumLocalParameters];
    Memory::Memzero(RayLocalRootParameters, sizeof(RayLocalRootParameters));

    for (UInt32 i = 0; i < NumLocalParameters; i++)
    {
        RayLocalRootParameters[i].ParameterType    = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        RayLocalRootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    RayLocalRootParameters[0].DescriptorTable.pDescriptorRanges   = LocalCBVRanges;
    RayLocalRootParameters[0].DescriptorTable.NumDescriptorRanges = NumLocalCBV;
    RayLocalRootParameters[1].DescriptorTable.pDescriptorRanges   = LocalSRVRanges;
    RayLocalRootParameters[1].DescriptorTable.NumDescriptorRanges = NumLocalSRV;
    RayLocalRootParameters[2].DescriptorTable.pDescriptorRanges   = LocalUAVRanges;
    RayLocalRootParameters[2].DescriptorTable.NumDescriptorRanges = NumLocalUAV;
    RayLocalRootParameters[3].DescriptorTable.pDescriptorRanges   = LocalSamplerRanges;
    RayLocalRootParameters[3].DescriptorTable.NumDescriptorRanges = NumLocalSamplers;

    D3D12_ROOT_SIGNATURE_DESC RayLocalRootDesc;
    Memory::Memzero(&RayLocalRootDesc);

    RayLocalRootDesc.NumParameters = NumLocalParameters;
    RayLocalRootDesc.pParameters   = RayLocalRootParameters;
    RayLocalRootDesc.Flags         = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

    LocalRayGen = Device->CreateRootSignature(RayLocalRootDesc);
    if (LocalRayGen)
    {
        LOG_INFO("Created Default LocalRay RootSignature");
        LocalRayGen->SetName("Default LocalRay RootSignature");

        LocalRayHit  = LocalRayGen;
        LocalRayMiss = LocalRayGen;
        return true;
    }
    else
    {
        return false;
    }
}
