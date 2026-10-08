#include "MetalRHI/MetalShader.h"

FMetalShader::FMetalShader(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , CompiledShader()
    , ThreadGroupSizeX(0)
    , ThreadGroupSizeY(0)
    , ThreadGroupSizeZ(0)
    , ShaderConstantsSize(0)
{
}

FMetalShader::~FMetalShader() = default;

bool FMetalShader::Initialize(const FShaderCodeView& InCode)
{
    const TArrayView<const FShaderResourceBinding> ShaderBindings = InCode.GetBindings();
    const TArrayView<const FMSLBindingSlot>        MSLSlots       = InCode.GetMSLSlots();

    if (MSLSlots.Size() != ShaderBindings.Size())
    {
        LOG_ERROR("Shader code has %d bindings but %d MSL slots", ShaderBindings.Size(), MSLSlots.Size());
        return false;
    }

    Bindings.Reset(ShaderBindings.Size());
    for (int32 Index = 0; Index < ShaderBindings.Size(); Index++)
    {
        FMSLShaderBinding& Binding = Bindings[Index];
        Binding.BindingType     = GetMSLBindingType(ShaderBindings[Index]);
        Binding.RegisterIndex   = ShaderBindings[Index].Register;
        Binding.SlotIndex       = MSLSlots[Index].Slot;
        Binding.NullTextureType = MSLSlots[Index].NullTextureType;

        if (Binding.BindingType == EMSLBindingType::Unknown)
        {
            LOG_ERROR("Shader binding %d has resource type %s, which has no MSL binding", Index, ToString(ShaderBindings[Index].Type));
            return false;
        }
    }

    const FMSLShaderInfo& MSLInfo = InCode.GetMSLInfo();
    ThreadGroupSizeX    = MSLInfo.ThreadGroupSize[0];
    ThreadGroupSizeY    = MSLInfo.ThreadGroupSize[1];
    ThreadGroupSizeZ    = MSLInfo.ThreadGroupSize[2];
    ShaderConstantsSize = InCode.GetInfo().ShaderConstantsSize;

    CompiledShader = GetDevice()->GetShaderLibraryCache().GetOrCompile(InCode.GetNativeCode(), InCode.GetEntryPoint());
    return CompiledShader != nullptr;
}

FMetalRayTracingShader::FMetalRayTracingShader(FMetalDevice* InDevice)
    : FMetalShader(InDevice)
{
}

FMetalRayTracingShader::~FMetalRayTracingShader() = default;

bool FMetalRayTracingShader::Initialize(const FShaderCodeView& InCode)
{
    if (!FMetalShader::Initialize(InCode))
    {
        return false;
    }

    Identifier = String(CompiledShader->FunctionName);
    return true;
}
