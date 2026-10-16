#pragma once
#include "Core/Memory/Memory.h"
#include "RHI/RayTracing/RHIRayTracingPipelineState.h"
#include "D3D12RHI/D3D12Shader.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12PipelineState.h"

typedef TSharedRef<class FD3D12RayTracingPipelineStateRHI> FD3D12RayTracingPipelineStateRHIRef;

struct FD3D12RayTracingShaderIdentifier
{
    uint8 ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
};

struct FD3D12RootSignatureAssociation
{
    FD3D12RootSignatureAssociation(ID3D12RootSignature* InRootSignature, const TArray<FWideString>& InShaderExportNames)
        : ExportAssociation()
        , RootSignature(InRootSignature)
        , ShaderExportNames(InShaderExportNames)
        , ShaderExportNamesRef(InShaderExportNames.Size())
    {
        for (int32 i = 0; i < ShaderExportNames.Size(); i++)
        {
            ShaderExportNamesRef[i] = *ShaderExportNames[i];
        }
    }

    ID3D12RootSignature*                   RootSignature;
    TArray<FWideString>                    ShaderExportNames;
    TArray<LPCWSTR>                        ShaderExportNamesRef;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ExportAssociation;
};

struct FD3D12HitGroup
{
    FD3D12HitGroup(const FWideString& InHitGroupName, ERayTracingHitGroupType InHitGroupType, const FWideString& InClosestHit, const FWideString& InAnyHit, const FWideString& InIntersection)
        : Desc()
        , HitGroupName(InHitGroupName)
        , ClosestHit(InClosestHit)
        , AnyHit(InAnyHit)
        , Intersection(InIntersection)
    {
        Memory::Memzero(&Desc);

        Desc.Type = (InHitGroupType == ERayTracingHitGroupType::Procedural)
            ? D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE
            : D3D12_HIT_GROUP_TYPE_TRIANGLES;

        Desc.HitGroupExport = *HitGroupName;

        if (ClosestHit != L"")
        {
            Desc.ClosestHitShaderImport = *ClosestHit;
        }

        if (AnyHit != L"")
        {
            Desc.AnyHitShaderImport = *AnyHit;
        }

        if (Desc.Type == D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE && Intersection != L"")
        {
            Desc.IntersectionShaderImport = *Intersection;
        }
    }

    D3D12_HIT_GROUP_DESC Desc;
    FWideString          HitGroupName;
    FWideString          ClosestHit;
    FWideString          AnyHit;
    FWideString          Intersection;
};

struct FD3D12Library
{
    FD3D12Library(D3D12_SHADER_BYTECODE ByteCode, const TArray<FWideString>& InExportNames, const TArray<FWideString>& InRenamedExports)
        : ExportNames(InExportNames)
        , RenamedExports(InRenamedExports)
        , ExportDescs(InExportNames.Size())
        , Desc()
    {
        for (int32 i = 0; i < ExportDescs.Size(); i++)
        {
            D3D12_EXPORT_DESC& TempDesc = ExportDescs[i];
            TempDesc.Flags          = D3D12_EXPORT_FLAG_NONE;
            TempDesc.Name           = *ExportNames[i];
            TempDesc.ExportToRename = RenamedExports.IsValidIndex(i) ? *RenamedExports[i] : nullptr;
        }

        Desc.DXILLibrary = ByteCode;
        Desc.pExports    = ExportDescs.Data();
        Desc.NumExports  = ExportDescs.Size();
    }

    TArray<FWideString>       ExportNames;
    TArray<FWideString>       RenamedExports;
    TArray<D3D12_EXPORT_DESC> ExportDescs;
    D3D12_DXIL_LIBRARY_DESC   Desc;
};

class FD3D12RayTracingPipelineStateStream
{
public:
    FD3D12RayTracingPipelineStateStream();
    ~FD3D12RayTracingPipelineStateStream();

    void Generate();

    void AddLibrary(D3D12_SHADER_BYTECODE ByteCode, const TArray<FWideString>& ExportNames, const TArray<FWideString>& RenamedExports = TArray<FWideString>())
    {
        Libraries.Emplace(ByteCode, ExportNames, RenamedExports);
    }

    void AddHitGroup(const FWideString& HitGroupName, ERayTracingHitGroupType HitGroupType, const FWideString& ClosestHit, const FWideString& AnyHit, const FWideString& Intersection)
    {
        HitGroups.Emplace(HitGroupName, HitGroupType, ClosestHit, AnyHit, Intersection);
    }

    void AddRootSignatureAssociation(ID3D12RootSignature* RootSignature, const TArray<FWideString>& ShaderExportNames)
    {
        RootSignatureAssociations.Emplace(RootSignature, ShaderExportNames);
    }

    void SetStateObjectConfig(D3D12_STATE_OBJECT_FLAGS Flags)
    {
        StateObjectConfig.Flags = Flags;
        bHasStateObjectConfig   = true;
    }

    TArray<FD3D12Library>                  Libraries;
    TArray<FD3D12HitGroup>                 HitGroups;
    TArray<FD3D12RootSignatureAssociation> RootSignatureAssociations;
    D3D12_RAYTRACING_PIPELINE_CONFIG1      PipelineConfig;
    D3D12_RAYTRACING_SHADER_CONFIG         ShaderConfig;
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
    TArray<FWideString>                    PayLoadExportNames;
    TArray<LPCWSTR>                        PayLoadExportNamesRef;
    ID3D12RootSignature*                   GlobalRootSignature;
    TArray<D3D12_STATE_SUBOBJECT>          SubObjects;
    D3D12_STATE_OBJECT_CONFIG              StateObjectConfig;
    bool                                   bHasStateObjectConfig;
};

class FD3D12RayTracingPipelineStateRHI : public FRHIRayTracingPipelineState, public FD3D12DeviceChild, public FD3D12EffectiveDescriptorCounts
{
public:
    FD3D12RayTracingPipelineStateRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayTracingPipelineStateRHI();

    bool Initialize(const FRHIRayTracingPipelineStateDesc& Desc);

    // FRHIPipelineState Interface
    virtual void* GetRHINativeState() const override final;

    virtual void SetDebugName(const FString& InName)       override final;
    virtual void GetDebugName(FString& OutDebugName) const override final;

    // FRHIRayTracingPipelineState Interface
    virtual void GetExportName(ERayTracingShaderRecordKind Kind, uint32 ShaderIndex, FString& OutExportName) const override final;
    virtual uint32 GetNumExportNames(ERayTracingShaderRecordKind Kind) const override final;

    void* GetShaderIdentifier(const FString& ExportName);

    FORCEINLINE ID3D12StateObject* GetD3D12StateObject() const
    {
        return StateObject.Get();
    }

    FORCEINLINE ID3D12StateObjectProperties* GetD3D12StateObjectProperties() const
    {
        return StateObjectProperties.Get();
    }

    FORCEINLINE FD3D12RootSignature* GetLocalRootSignature(const FString& ExportName) const
    {
        if (const FD3D12RootSignatureRef* Found = LocalRootSignatures.Find(ExportName))
        {
            return Found->Get();
        }

        return nullptr;
    }

    FORCEINLINE FD3D12RootSignature* GetGlobalRootSignature() const
    {
        return GlobalRootSignature.Get();
    }

private:
    TArray<FString>& GetExportNameArray(ERayTracingShaderRecordKind Kind)
    {
        return ExportNames[uint32(Kind)];
    }

    const TArray<FString>& GetExportNameArray(ERayTracingShaderRecordKind Kind) const
    {
        return ExportNames[uint32(Kind)];
    }

    TArray<FString>                                 ExportNames[4];
    FString                                         DebugName;
    TComPtr<ID3D12StateObject>                      StateObject;
    TComPtr<ID3D12StateObjectProperties>            StateObjectProperties;
    FD3D12RootSignatureRef                          GlobalRootSignature;
    TMap<FString, FD3D12RootSignatureRef>           LocalRootSignatures;
    TMap<FString, FD3D12RayTracingShaderIdentifier> ShaderIdentifiers;
};
