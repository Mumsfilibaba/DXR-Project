#include "D3D12RHI/RayTracing/D3D12RayTracingPipeline.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Stats.h"

static EResourceType::Type GetResourceTypeFromBindingType(ED3D12BindingType BindingType)
{
    switch (BindingType)
    {
    case ED3D12BindingType::ConstantBuffer: return EResourceType::CBV;
    case ED3D12BindingType::SRV:            return EResourceType::SRV;
    case ED3D12BindingType::UAV:            return EResourceType::UAV;
    case ED3D12BindingType::Sampler:        return EResourceType::Sampler;

    default:
        CHECK(false);
        return EResourceType::Unknown;
    }
}

static FD3D12RootSignatureLayout BuildLocalLayoutFromBindingInfo(const FD3D12ShaderBindingInfo& LocalBindingInfo)
{
    FD3D12RootSignatureLayout Layout;
    Layout.SetType(ERootSignatureType::RayTracingLocal);
    Layout.SetAllowInputAssembler(false);

    for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : LocalBindingInfo.ResourceBindings)
    {
        if (Binding.BindingType == ED3D12BindingType::SRV && Binding.bIsTexture)
        {
            Layout.AddLocalTableSRVRegister(EShaderVisibility::All, Binding.OriginalBindingIndex);
        }
        else if (Binding.BindingType == ED3D12BindingType::UAV && Binding.bIsTexture)
        {
            Layout.AddLocalTableUAVRegister(EShaderVisibility::All, Binding.OriginalBindingIndex);
        }
        else if (Binding.BindingType == ED3D12BindingType::Sampler)
        {
            Layout.AddLocalTableSamplerRegister(EShaderVisibility::All, Binding.OriginalBindingIndex);
        }
        else
        {
            Layout.AddRegister(EShaderVisibility::All, GetResourceTypeFromBindingType(Binding.BindingType), Binding.OriginalBindingIndex);
        }
    }

    Layout.SetNumPushConstants(static_cast<uint8>(LocalBindingInfo.NumPushConstants));
    return Layout;
}

static bool MergeHitGroupLocalBindings(const TArray<FD3D12RayTracingShader*>& Members, FD3D12ShaderBindingInfo& OutMerged)
{
    for (FD3D12RayTracingShader* Member : Members)
    {
        CHECK(Member != nullptr);

        const FD3D12ShaderBindingInfo& MemberInfo = Member->GetLocalBindingInfo();
        OutMerged.NumPushConstants = Math::Max<uint32>(OutMerged.NumPushConstants, MemberInfo.NumPushConstants);

        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : MemberInfo.ResourceBindings)
        {
            bool bAlreadyPresent = false;
            for (const FD3D12ShaderBindingInfo::FResourceBinding& Existing : OutMerged.ResourceBindings)
            {
                if (Existing.BindingType == Binding.BindingType && Existing.OriginalBindingIndex == Binding.OriginalBindingIndex)
                {
                    if (Existing.bIsTexture != Binding.bIsTexture)
                    {
                        D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: Hit group shaders declare the same local register with conflicting resource dimensions (texture vs buffer)");
                        return false;
                    }

                    bAlreadyPresent = true;
                    break;
                }
            }

            if (!bAlreadyPresent)
            {
                OutMerged.ResourceBindings.Add(Binding);
            }
        }
    }

    return true;
}

FD3D12RayTracingPipelineStateStream::FD3D12RayTracingPipelineStateStream()
    : GlobalRootSignature(nullptr)
    , StateObjectConfig({ D3D12_STATE_OBJECT_FLAG_NONE })
    , bHasStateObjectConfig(false)
{
    Memory::Memzero(&PipelineConfig, sizeof(PipelineConfig));
    Memory::Memzero(&ShaderConfig, sizeof(ShaderConfig));
    Memory::Memzero(&ShaderConfigAssociation, sizeof(ShaderConfigAssociation));
}

FD3D12RayTracingPipelineStateStream::~FD3D12RayTracingPipelineStateStream() = default;

void FD3D12RayTracingPipelineStateStream::Generate()
{
    uint32 NumSubObjects = Libraries.Size() + HitGroups.Size() + (RootSignatureAssociations.Size() * 2) + 4;
    if (bHasStateObjectConfig)
    {
        ++NumSubObjects;
    }

    SubObjects.Resize(NumSubObjects);

    uint32 SubObjectIndex = 0;
    if (bHasStateObjectConfig)
    {
        D3D12_STATE_SUBOBJECT& ConfigSubObject = SubObjects[SubObjectIndex++];
        ConfigSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_STATE_OBJECT_CONFIG;
        ConfigSubObject.pDesc = &StateObjectConfig;
    }

    for (FD3D12Library& Library : Libraries)
    {
        D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
        SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
        SubObject.pDesc = &Library.Desc;
    }

    for (FD3D12HitGroup& HitGroup : HitGroups)
    {
        D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
        SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
        SubObject.pDesc = &HitGroup.Desc;
    }

    for (FD3D12RootSignatureAssociation& Association : RootSignatureAssociations)
    {
        D3D12_STATE_SUBOBJECT& LocalRootSubObject = SubObjects[SubObjectIndex++];
        LocalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
        LocalRootSubObject.pDesc = &Association.RootSignature;

        Association.ExportAssociation.pExports              = Association.ShaderExportNamesRef.Data();
        Association.ExportAssociation.NumExports            = Association.ShaderExportNamesRef.Size();
        Association.ExportAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

        D3D12_STATE_SUBOBJECT& SubObject = SubObjects[SubObjectIndex++];
        SubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
        SubObject.pDesc = &Association.ExportAssociation;
    }

    D3D12_STATE_SUBOBJECT& GlobalRootSubObject = SubObjects[SubObjectIndex++];
    GlobalRootSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    GlobalRootSubObject.pDesc = &GlobalRootSignature;

    D3D12_STATE_SUBOBJECT& PipelineConfigSubObject = SubObjects[SubObjectIndex++];
    PipelineConfigSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    PipelineConfigSubObject.pDesc = &PipelineConfig;

    D3D12_STATE_SUBOBJECT& ShaderConfigObject = SubObjects[SubObjectIndex++];
    ShaderConfigObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    ShaderConfigObject.pDesc = &ShaderConfig;

    PayLoadExportNamesRef.Resize(PayLoadExportNames.Size());
    for (int32 i = 0; i < PayLoadExportNames.Size(); i++)
    {
        PayLoadExportNamesRef[i] = *PayLoadExportNames[i];
    }

    ShaderConfigAssociation.pExports              = PayLoadExportNamesRef.Data();
    ShaderConfigAssociation.NumExports            = PayLoadExportNamesRef.Size();
    ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects[SubObjectIndex - 1];

    D3D12_STATE_SUBOBJECT& ShaderConfigAssociationSubObject = SubObjects[SubObjectIndex++];
    ShaderConfigAssociationSubObject.Type  = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
    ShaderConfigAssociationSubObject.pDesc = &ShaderConfigAssociation;
}

FD3D12RayTracingPipelineStateRHI::FD3D12RayTracingPipelineStateRHI(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , StateObject(nullptr)
{
}

FD3D12RayTracingPipelineStateRHI::~FD3D12RayTracingPipelineStateRHI() = default;

void FD3D12RayTracingPipelineStateRHI::SetDebugName(const String& InName)
{
    WString WideName = CharToWide(InName);
    StateObject->SetName(*WideName);
    DebugName = InName;
}

void FD3D12RayTracingPipelineStateRHI::GetDebugName(String& OutDebugName) const
{
    OutDebugName = DebugName;
}

void* FD3D12RayTracingPipelineStateRHI::GetRHINativeState() const
{
    return reinterpret_cast<void*>(GetD3D12StateObject());
}

void FD3D12RayTracingPipelineStateRHI::GetExportName(ERayTracingShaderRecordKind Kind, uint32 RecordIndex, String& OutExportName) const
{
    const TArray<String>& Names = GetExportNameArray(Kind);
    if (Names.IsEmpty())
    {
        OutExportName = String();
        return;
    }

    if (RecordIndex < uint32(Names.Size()))
    {
        OutExportName = Names[int32(RecordIndex)];
        return;
    }

    OutExportName = (Kind == ERayTracingShaderRecordKind::HitGroup) ? Names[0] : String();
}

uint32 FD3D12RayTracingPipelineStateRHI::GetNumExportNames(ERayTracingShaderRecordKind Kind) const
{
    return uint32(GetExportNameArray(Kind).Size());
}

bool FD3D12RayTracingPipelineStateRHI::Initialize(const FRHIRayTracingPipelineStateDesc& Desc)
{
    RayTracingPipelineFlags = Desc.Flags;

    FD3D12RayTracingPipelineStateStream PipelineStream;
    TArray<FD3D12Shader*> Shaders;

    FD3D12RootSignatureManager& RootSignatureManager = GetDevice()->GetRootSignatureManager();

    // Collect and add all RayGen-Shaders
    for (FRHIRayGenShader* RayGen : Desc.RayGenShaders)
    {
        FD3D12RayGenShaderRHI* D3D12RayGen = FD3D12DeviceRHI::ResourceCast(RayGen);
        Shaders.Emplace(D3D12RayGen);

        FD3D12RootSignatureLayout RayGenLocalLayout        = BuildLocalLayoutFromBindingInfo(D3D12RayGen->GetLocalBindingInfo());
        FD3D12RootSignatureRef    RayGenLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(RayGenLocalLayout));

        if (!RayGenLocalRootSignature)
        {
            return false;
        }

        LocalRootSignatures.Add(D3D12RayGen->GetIdentifier(), RayGenLocalRootSignature);
        GetExportNameArray(ERayTracingShaderRecordKind::RayGeneration).Emplace(D3D12RayGen->GetIdentifier());

        WString RayGenIdentifier = CharToWide(D3D12RayGen->GetIdentifier());
        PipelineStream.AddLibrary(D3D12RayGen->GetByteCode().GetD3D12Bytecode(), { RayGenIdentifier });
        PipelineStream.AddRootSignatureAssociation(RayGenLocalRootSignature->GetD3D12RootSignature(), { RayGenIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(RayGenIdentifier);
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Desc.HitGroups)
    {
        WString                         ClosestHitName;
        WString                         AnyHitName;
        WString                         IntersectionName;
        TArray<WString>                 HitGroupMemberExports;
        TArray<FD3D12RayTracingShader*> HitGroupMemberShaders;

        for (FRHIRayTracingShader* HitGroupShader : HitGroup.Shaders)
        {
            FD3D12RayTracingShader* D3D12HitGroupShader = GetD3D12RayTracingShader(HitGroupShader);
            if (!D3D12HitGroupShader)
            {
                continue;
            }

            const WString ShaderIdentifier = CharToWide(D3D12HitGroupShader->GetIdentifier());

            Shaders.Emplace(D3D12HitGroupShader);
            PipelineStream.AddLibrary(D3D12HitGroupShader->GetByteCode().GetD3D12Bytecode(), { ShaderIdentifier });
            PipelineStream.PayLoadExportNames.Emplace(ShaderIdentifier);
            HitGroupMemberExports.Emplace(ShaderIdentifier);
            HitGroupMemberShaders.Emplace(D3D12HitGroupShader);

            switch (HitGroupShader->GetShaderStage())
            {
                case EShaderStage::RayClosestHit:
                    ClosestHitName = ShaderIdentifier;
                    break;

                case EShaderStage::RayAnyHit:
                    AnyHitName = ShaderIdentifier;
                    break;

                case EShaderStage::RayIntersection:
                    IntersectionName = ShaderIdentifier;
                    break;

                default:
                    break;
            }
        }

        PipelineStream.AddHitGroup(CharToWide(HitGroup.Name), HitGroup.Type, ClosestHitName, AnyHitName, IntersectionName);
        GetExportNameArray(ERayTracingShaderRecordKind::HitGroup).Emplace(HitGroup.Name);

        if (!HitGroupMemberShaders.IsEmpty())
        {
            FD3D12ShaderBindingInfo MergedLocalBindings;
            if (!MergeHitGroupLocalBindings(HitGroupMemberShaders, MergedLocalBindings))
            {
                return false;
            }

            FD3D12RootSignatureLayout HitLocalLayout        = BuildLocalLayoutFromBindingInfo(MergedLocalBindings);
            FD3D12RootSignatureRef    HitLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(HitLocalLayout));

            if (!HitLocalRootSignature)
            {
                return false;
            }

            LocalRootSignatures.Add(HitGroup.Name, HitLocalRootSignature);

            if (!HitGroupMemberExports.IsEmpty())
            {
                PipelineStream.AddRootSignatureAssociation(HitLocalRootSignature->GetD3D12RootSignature(), HitGroupMemberExports);
            }
        }
    }

    // Collect and add all Miss shaders
    for (FRHIRayMissShader* Miss : Desc.MissShaders)
    {
        FD3D12RayMissShaderRHI* D3D12MissShader = FD3D12DeviceRHI::ResourceCast(Miss);
        Shaders.Emplace(D3D12MissShader);

        FD3D12RootSignatureLayout MissLocalLayout        = BuildLocalLayoutFromBindingInfo(D3D12MissShader->GetLocalBindingInfo());
        FD3D12RootSignatureRef    MissLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(MissLocalLayout));

        if (!MissLocalRootSignature)
        {
            return false;
        }

        LocalRootSignatures.Add(D3D12MissShader->GetIdentifier(), MissLocalRootSignature);
        GetExportNameArray(ERayTracingShaderRecordKind::Miss).Emplace(D3D12MissShader->GetIdentifier());

        WString MissIdentifier = CharToWide(D3D12MissShader->GetIdentifier());
        PipelineStream.AddLibrary(D3D12MissShader->GetByteCode().GetD3D12Bytecode(), { MissIdentifier });
        PipelineStream.AddRootSignatureAssociation(MissLocalRootSignature->GetD3D12RootSignature(), { MissIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(MissIdentifier);
    }

    // Collect and add all Callable shaders
    for (FRHIRayCallableShader* Callable : Desc.CallableShaders)
    {
        FD3D12RayCallableShaderRHI* D3D12CallableShader = FD3D12DeviceRHI::ResourceCast(Callable);
        Shaders.Emplace(D3D12CallableShader);

        FD3D12RootSignatureLayout CallableLocalLayout        = BuildLocalLayoutFromBindingInfo(D3D12CallableShader->GetLocalBindingInfo());
        FD3D12RootSignatureRef    CallableLocalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(CallableLocalLayout));

        if (!CallableLocalRootSignature)
        {
            return false;
        }

        LocalRootSignatures.Add(D3D12CallableShader->GetIdentifier(), CallableLocalRootSignature);
        GetExportNameArray(ERayTracingShaderRecordKind::Callable).Emplace(D3D12CallableShader->GetIdentifier());

        WString CallableIdentifier = CharToWide(D3D12CallableShader->GetIdentifier());
        PipelineStream.AddLibrary(D3D12CallableShader->GetByteCode().GetD3D12Bytecode(), { CallableIdentifier });
        PipelineStream.AddRootSignatureAssociation(CallableLocalRootSignature->GetD3D12RootSignature(), { CallableIdentifier });
        PipelineStream.PayLoadExportNames.Emplace(CallableIdentifier);
    }

    PipelineStream.ShaderConfig.MaxAttributeSizeInBytes  = Desc.MaxAttributeSizeInBytes;
    PipelineStream.ShaderConfig.MaxPayloadSizeInBytes    = Desc.MaxPayloadSizeInBytes;
    PipelineStream.PipelineConfig.MaxTraceRecursionDepth = Desc.MaxRecursionDepth;

    FD3D12RootSignatureLayout GlobalLayout;
    GlobalLayout.SetType(ERootSignatureType::RayTracingGlobal);
    GlobalLayout.SetAllowInputAssembler(false);

    uint8 MaxPushConstants = 0;
    ED3D12ShaderFlags AggregatedRayTracingShaderFlags = ED3D12ShaderFlags::None;
    for (FD3D12Shader* Shader : Shaders)
    {
        CHECK(Shader != nullptr);

        const FD3D12ShaderBindingInfo& BindingInfo = Shader->GetBindingInfo();
        for (const FD3D12ShaderBindingInfo::FResourceBinding& Binding : BindingInfo.ResourceBindings)
        {
            GlobalLayout.AddRegister(EShaderVisibility::All, static_cast<EResourceType::Type>(Binding.BindingType), Binding.OriginalBindingIndex);
        }

        MaxPushConstants = Math::Max<uint8>(MaxPushConstants, static_cast<uint8>(BindingInfo.NumPushConstants));
        AggregatedRayTracingShaderFlags |= Shader->GetFlags();
    }

    GlobalLayout.SetNumPushConstants(MaxPushConstants);
    GlobalLayout.SetDirectlyIndexedResourceHeap(IsEnumFlagSet(AggregatedRayTracingShaderFlags, ED3D12ShaderFlags::RequiresResourceDescriptorHeapIndexing));
    GlobalLayout.SetDirectlyIndexedSamplerHeap(IsEnumFlagSet(AggregatedRayTracingShaderFlags, ED3D12ShaderFlags::RequiresSamplerDescriptorHeapIndexing));
    GlobalLayout.ComputeRootCBVs();

    GlobalRootSignature = MakeSharedRef<FD3D12RootSignature>(RootSignatureManager.GetOrCreateRootSignature(GlobalLayout));
    if (!GlobalRootSignature)
    {
        return false;
    }

    ComputeEffectiveDescriptorCounts(GlobalRootSignature.Get(), Shaders.Data(), Shaders.Size());
    PipelineStream.GlobalRootSignature = GlobalRootSignature->GetD3D12RootSignature();

    if (IsEnumFlagSet(Desc.Flags, ERayTracingPipelineFlags::AllowStateObjectAdditions))
    {
        PipelineStream.SetStateObjectConfig(D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS);
    }

    PipelineStream.Generate();

    D3D12_STATE_OBJECT_DESC RayTracingPipeline;
    Memory::Memzero(&RayTracingPipeline);

    RayTracingPipeline.Type          = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
    RayTracingPipeline.pSubobjects   = PipelineStream.SubObjects.Data();
    RayTracingPipeline.NumSubobjects = PipelineStream.SubObjects.Size();

    TComPtr<ID3D12StateObject> TempStateObject;
    
    HRESULT Result = E_FAIL;
    if (Desc.BasePipeline != nullptr)
    {
        FD3D12RayTracingPipelineStateRHI* BasePipeline = FD3D12DeviceRHI::ResourceCast(Desc.BasePipeline);

        ID3D12StateObject* BaseStateObject = BasePipeline ? BasePipeline->GetD3D12StateObject() : nullptr;
        if (!BaseStateObject)
        {
            D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: BasePipeline has no valid state object for AddToStateObject");
            return false;
        }

    #if D3D12_USE_ID3D12DEVICE_7
        ID3D12Device7* Device7 = GetDevice()->GetD3D12Device7();
        if (!Device7)
        {
            D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: ID3D12Device7 is required for AddToStateObject");
            return false;
        }

        Result = Device7->AddToStateObject(&RayTracingPipeline, BaseStateObject, IID_PPV_ARGS(&TempStateObject));
        if (FAILED(Result))
        {
            D3D12_ERROR("[D3D12RayTracingPipelineState]: AddToStateObject failed (hr=0x%08X)", Result);
            DEBUG_BREAK();
            return false;
        }
    #else
        D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: ID3D12Device7 is required for AddToStateObject");
        return false;
    #endif
    }
    else
    {
    #if D3D12_USE_ID3D12DEVICE_5
        Result = GetDevice()->GetD3D12Device5()->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&TempStateObject));
        if (FAILED(Result))
        {
            D3D12_ERROR("[D3D12RayTracingPipelineState]: CreateStateObject failed (hr=0x%08X)", Result);
            DEBUG_BREAK();
            return false;
        }
    #else
        D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState]: ID3D12Device5 is required for ray tracing pipeline creation");
        return false;
    #endif
    }

    TComPtr<ID3D12StateObjectProperties> TempStateObjectProperties;
    Result = TempStateObject->QueryInterface(IID_PPV_ARGS(&TempStateObjectProperties));
    if (FAILED(Result))
    {
        D3D12_ERROR_CRITICAL("[D3D12RayTracingPipelineState] Failed to retrieve ID3D12StateObjectProperties");
        return false;
    }

    StateObject           = TempStateObject;
    StateObjectProperties = TempStateObjectProperties;

    STAT_ADD(STAT_D3D12_NumRayTracingPipelineStates, 1);
    return true;
}

void* FD3D12RayTracingPipelineStateRHI::GetShaderIdentifier(const String& ExportName)
{
    if (FD3D12RayTracingShaderIdentifier* MapItem = ShaderIdentifiers.Find(ExportName))
    {
        return MapItem->ShaderIdentifier;
    }
    else
    {
        WString WideExportName = CharToWide(ExportName);

        void* Result = StateObjectProperties->GetShaderIdentifier(*WideExportName);
        if (!Result)
        {
            return nullptr;
        }

        FD3D12RayTracingShaderIdentifier Identifier;
        Memory::Memcpy(Identifier.ShaderIdentifier, Result, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

        FD3D12RayTracingShaderIdentifier& NewIdentifier = ShaderIdentifiers.Add(ExportName, Identifier);
        return NewIdentifier.ShaderIdentifier;
    }
}
