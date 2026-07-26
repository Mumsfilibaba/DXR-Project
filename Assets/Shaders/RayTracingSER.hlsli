#ifndef RAY_TRACING_SER_HLSLI
#define RAY_TRACING_SER_HLSLI

#include "CoreDefines.hlsli"
#include "RayTracingHelpers.hlsli"

// Shader Execution Reordering (SER) for the Vulkan / SPIR-V backend.
//
// DXC does not lower the HLSL `dx::HitObject` SER API to SPIR-V so on Vulkan we express SER 
// with inline SPIR-V intrinsics from SPV_EXT_shader_invocation_reorder. D3D12 keeps
// using `dx::HitObject` directly. Enum values are taken from the Vulkan SDK spirv.h (unified1):
//   OpTypeHitObjectEXT                    = 5313
//   OpReorderThreadWithHitObjectEXT       = 5315
//   OpHitObjectTraceRayEXT                = 5316
//   OpHitObjectRecordEmptyEXT             = 5318
//   OpHitObjectExecuteShaderEXT           = 5319
//   Capability ShaderInvocationReorderEXT = 5388
//   StorageClass RayPayloadKHR            = 5338
#if SHADER_BACKEND == SHADER_BACKEND_VULKAN && RAY_TRACING_SHADER_EXECUTION_REORDERING
    // The ray payload must live in the RayPayloadKHR storage class so it can be passed to the OpHitObject*EXT payload operands.
    #define SER_RAY_PAYLOAD_STORAGE [[vk::ext_storage_class(/* RayPayloadKHR */ 5338)]]

    // Opaque hit-object handle
    typedef vk::SpirvOpaqueType</* OpTypeHitObjectEXT */ 5313> HitObjectEXT;

    // HitObject
    [[vk::ext_capability(/* ShaderInvocationReorderEXT */ 5388)]]
    [[vk::ext_extension("SPV_EXT_shader_invocation_reorder")]]
    [[vk::ext_instruction(/* OpHitObjectRecordEmptyEXT */ 5318)]]
    void HitObjectRecordEmptyEXT([[vk::ext_reference]] HitObjectEXT HitObject);

    // HitObjectTraceRayEXT
    [[vk::ext_capability(/* ShaderInvocationReorderEXT */ 5388)]]
    [[vk::ext_extension("SPV_EXT_shader_invocation_reorder")]]
    [[vk::ext_instruction(/* OpHitObjectTraceRayEXT */ 5316)]]
    void HitObjectTraceRayEXT(
        [[vk::ext_reference]] HitObjectEXT HitObject,
        RaytracingAccelerationStructure AccelerationStructure,
        uint   RayFlags,
        uint   InstanceCullMask,
        uint   SBTRecordOffset,
        uint   SBTRecordStride,
        uint   MissIndex,
        float3 Origin,
        float  TMin,
        float3 Direction,
        float  TMax,
        [[vk::ext_reference]] [[vk::ext_storage_class(/* RayPayloadKHR */ 5338)]] FRayPayload Payload);

    // ReorderThreadWithHitObjectEXT: Reorder the current invocation for coherence using the recorded hit object.
    [[vk::ext_capability(/* ShaderInvocationReorderEXT */ 5388)]]
    [[vk::ext_extension("SPV_EXT_shader_invocation_reorder")]]
    [[vk::ext_instruction(/* OpReorderThreadWithHitObjectEXT */ 5315)]]
    void ReorderThreadWithHitObjectEXT([[vk::ext_reference]] HitObjectEXT HitObject);

    // HitObjectExecuteShaderEXT: Invoke the closest-hit/miss shader recorded in the hit object, writing into Payload.
    [[vk::ext_capability(/* ShaderInvocationReorderEXT */ 5388)]]
    [[vk::ext_extension("SPV_EXT_shader_invocation_reorder")]]
    [[vk::ext_instruction(/* OpHitObjectExecuteShaderEXT */ 5319)]]
    void HitObjectExecuteShaderEXT(
        [[vk::ext_reference]] HitObjectEXT HitObject,
        [[vk::ext_reference]] [[vk::ext_storage_class(/* RayPayloadKHR */ 5338)]] FRayPayload Payload);
#else
    #define SER_RAY_PAYLOAD_STORAGE
#endif

#endif
