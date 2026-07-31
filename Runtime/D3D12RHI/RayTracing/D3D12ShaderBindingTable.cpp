#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12CommandList.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12ResourceViews.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/RayTracing/D3D12ShaderBindingTable.h"

static_assert(sizeof(FD3D12ShaderBindingTableEntry) == 64, "FD3D12ShaderBindingTableEntry must be exactly 64 bytes (32-byte identifier + 4 local entries).");

static FD3D12BufferRHI* GetViewedBuffer(FRHIResource* ViewedResource)
{
    if (!ViewedResource || ViewedResource->GetResourceType() != ERHIResourceType::Buffer)
    {
        return nullptr;
    }

    return FD3D12DeviceRHI::ResourceCast(static_cast<FRHIBuffer*>(ViewedResource));
}

FD3D12ShaderBindingTable::FD3D12ShaderBindingTable(FD3D12Device* InDevice, const FRHIShaderBindingTableDesc& InDesc)
    : FRHIShaderBindingTable(InDesc)
    , FD3D12DeviceChild(InDevice)
    , Pipeline(FD3D12DeviceRHI::ResourceCast(InDesc.Pipeline))
    , NumRayGen(Math::Max<uint32>(InDesc.NumRayGenerationShaders, 1u))
    , NumMiss(InDesc.NumMissShaders)
    , NumCallable(InDesc.NumCallableShaders)
    , NumHitGroup(InDesc.NumHitGroupRecords)
    , RecordStride(sizeof(FD3D12ShaderBindingTableEntry))
    , CpuShadow()
    , TableResourceStorage(InDevice)
{
    if (Pipeline)
    {
        Pipeline->AddRef();
    }

    const uint32 TotalRecords = NumRayGen + NumMiss + NumHitGroup + NumCallable;
    CpuShadow.Resize(int32(TotalRecords * RecordStride));
    Memory::Memzero(CpuShadow.Data(), CpuShadow.SizeInBytes());
}

FD3D12ShaderBindingTable::~FD3D12ShaderBindingTable() = default;

void* FD3D12ShaderBindingTable::GetRHINativeResource() const
{
    FD3D12Resource* Resource = TableResourceStorage.GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

bool FD3D12ShaderBindingTable::Initialize()
{
    const uint64 RequiredSize = uint64(CpuShadow.SizeInBytes());
    if (RequiredSize == 0)
    {
        return false;
    }

    FD3D12BufferAllocator* Allocator = GetDevice()->GetBufferAllocator();
    if (!Allocator)
    {
        return false;
    }

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_NONE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.Width              = RequiredSize;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.MipLevels          = 1;
    Desc.SampleDesc.Count   = 1;

    if (!Allocator->TryAllocate(
            D3D12_HEAP_TYPE_DEFAULT,
            Desc,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            ED3D12ResourceStateMode::MultipleStates,
            D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
            TableResourceStorage))
    {
        return false;
    }

    return TableResourceStorage.GetResource() != nullptr;
}

FRHIShaderBindingTableAddressInfo FD3D12ShaderBindingTable::GetAddressInfo() const
{
    FRHIShaderBindingTableAddressInfo AddressInfo = {};
    if (!TableResourceStorage.GetResource())
    {
        return AddressInfo;
    }

    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE RayGen = GetRayGenRecord();
    AddressInfo.RayGeneration = { RayGen.StartAddress, RayGen.SizeInBytes, RayGen.SizeInBytes };

    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Miss = GetMissTable();
    AddressInfo.Miss = { Miss.StartAddress, Miss.SizeInBytes, Miss.StrideInBytes };

    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE HitGroup = GetHitGroupTable();
    AddressInfo.HitGroup = { HitGroup.StartAddress, HitGroup.SizeInBytes, HitGroup.StrideInBytes };

    const D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE Callable = GetCallableTable();
    AddressInfo.Callable = { Callable.StartAddress, Callable.SizeInBytes, Callable.StrideInBytes };
    return AddressInfo;
}

uint32 FD3D12ShaderBindingTable::GetSubTableBaseRecord(ERayTracingShaderRecordKind RecordKind) const
{
    switch (RecordKind)
    {
        case ERayTracingShaderRecordKind::RayGeneration:
            return 0;

        case ERayTracingShaderRecordKind::Miss:
            return NumRayGen;
        
        case ERayTracingShaderRecordKind::HitGroup:
            return NumRayGen + NumMiss;

        case ERayTracingShaderRecordKind::Callable:
            return NumRayGen + NumMiss + NumHitGroup;
        
        default:
            return 0;
    }
}

void FD3D12ShaderBindingTable::PopulateRecord(FD3D12RootSignature* LocalRootSignature, uint8* OutRecord, uint64 RecordByteOffset, ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const String& ExportName, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    UNREFERENCED_VARIABLE(RecordKind);
    UNREFERENCED_VARIABLE(RecordIndex);

    void* Identifier = (Pipeline && !ExportName.IsEmpty()) ? Pipeline->GetShaderIdentifier(ExportName) : nullptr;

    bool bIdentifierIsZero = true;
    if (Identifier)
    {
        const uint8* IdentifierBytes = reinterpret_cast<const uint8*>(Identifier);
        for (uint32 ByteIndex = 0; ByteIndex < D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; ++ByteIndex)
        {
            if (IdentifierBytes[ByteIndex] != 0)
            {
                bIdentifierIsZero = false;
                break;
            }
        }
    }

    if (Identifier && !bIdentifierIsZero)
    {
        Memory::Memcpy(OutRecord, Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
    else
    {
        D3D12_ERROR("[FD3D12ShaderBindingTable]: Failed to resolve shader identifier for record kind '%s' index %u (export name '%s')."
            "The record would be written with a zeroed identifier and its shader would never execute. verify the pipeline registered an export for this shader.",
            ToString(RecordKind), RecordIndex, ExportName.IsEmpty() ? "<empty>" : *ExportName);
    }

    if (!LocalRootSignature || !Bindings || NumBindings == 0)
    {
        return;
    }

    D3D12_GPU_VIRTUAL_ADDRESS* RootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(OutRecord + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

    const FD3D12ShaderStage& Stage                  = LocalRootSignature->GetShaderStage(EShaderVisibility::All);
    const int8               SamplerTableParamIndex = Stage.GetRootParameterIndex(EResourceType::Sampler);
    const int8               SRVTableParamIndex     = Stage.GetRootParameterIndex(EResourceType::SRV);
    const int8               UAVTableParamIndex     = Stage.GetRootParameterIndex(EResourceType::UAV);

    FD3D12PendingLocalTable PendingShaderResourceViewTable;
    PendingShaderResourceViewTable.RecordByteOffset = RecordByteOffset;
    PendingShaderResourceViewTable.TableParamSlot   = (SRVTableParamIndex >= 0) ? static_cast<uint32>(SRVTableParamIndex) : 0;
    PendingShaderResourceViewTable.DescriptorType   = ED3D12LocalTableDescriptorType::ShaderResourceView;

    FD3D12PendingLocalTable PendingUnorderedAccessViewTable;
    PendingUnorderedAccessViewTable.RecordByteOffset = RecordByteOffset;
    PendingUnorderedAccessViewTable.TableParamSlot   = (UAVTableParamIndex >= 0) ? static_cast<uint32>(UAVTableParamIndex) : 0;
    PendingUnorderedAccessViewTable.DescriptorType   = ED3D12LocalTableDescriptorType::UnorderedAccessView;

    FD3D12PendingLocalTable PendingSamplerTable;
    PendingSamplerTable.RecordByteOffset = RecordByteOffset;
    PendingSamplerTable.TableParamSlot   = (SamplerTableParamIndex >= 0) ? static_cast<uint32>(SamplerTableParamIndex) : 0;
    PendingSamplerTable.DescriptorType   = ED3D12LocalTableDescriptorType::Sampler;

    bool bHasShaderResourceViewTable  = false;
    bool bHasUnorderedAccessViewTable = false;
    bool bHasSamplerTable             = false;
    bool bReportedBudgetOverflow      = false;

    const auto ReportBudgetOverflowIfNeeded = [&](int8 ParamIndex)
    {
        if (ParamIndex >= D3D12_MAX_LOCAL_RECORD_ENTRIES && !bReportedBudgetOverflow)
        {
            D3D12_ERROR("[FD3D12ShaderBindingTable]: Local root signature for '%s' exceeds the %d-entry hit-record budget. local binding at parameter %d dropped.", ExportName.IsEmpty() ? "<empty>" : *ExportName, int32(D3D12_MAX_LOCAL_RECORD_ENTRIES), int32(ParamIndex));
            bReportedBudgetOverflow = true;
        }
    };

    bool bReportedTableOverflow = false;
    const auto ReportTableOverflowIfNeeded = [&](int8 Slot)
    {
        if (Slot >= int8(FD3D12PendingLocalTable::MaxDescriptors) && !bReportedTableOverflow)
        {
            D3D12_ERROR("[FD3D12ShaderBindingTable]: Local descriptor table for '%s' exceeds the %u-descriptor capacity. local binding at table slot %d dropped.", ExportName.IsEmpty() ? "<empty>" : *ExportName, FD3D12PendingLocalTable::MaxDescriptors, int32(Slot));
            bReportedTableOverflow = true;
        }
    };

    for (uint32 i = 0; i < NumBindings; ++i)
    {
        const FRHIHitGroupLocalShaderBinding& Binding  = Bindings[i];
        const uint16                          Register = static_cast<uint16>(Binding.RegisterIndex);
        switch (Binding.Type)
        {
            case ERayTracingLocalBindingType::ConstantBuffer:
            {
                const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(EResourceType::CBV, Register);
                if (ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_RECORD_ENTRIES)
                {
                    FD3D12BufferRHI* Buffer = FD3D12DeviceRHI::ResourceCast(Binding.Buffer);
                    RootDescriptors[ParamIndex] = Buffer ? Buffer->GetGPUVirtualAddress() : 0;
                }
                else
                {
                    ReportBudgetOverflowIfNeeded(ParamIndex);
                }

                break;
            }

            case ERayTracingLocalBindingType::ShaderResourceView:
            {
                FD3D12ShaderResourceViewRHI* ShaderResourceView = FD3D12DeviceRHI::ResourceCast(Binding.ShaderResourceView);
                FD3D12BufferRHI*             ViewedBuffer       = (ShaderResourceView && ShaderResourceView->GetDesc().IsBufferSRV()) ? GetViewedBuffer(ShaderResourceView->GetResource()) : nullptr;
                const bool                   bIsBuffer          = ViewedBuffer != nullptr;

                const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(EResourceType::SRV, Register);
                if (bIsBuffer && ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_RECORD_ENTRIES)
                {
                    const auto&  BufferViewDesc = ShaderResourceView->GetDesc().Buffer;
                    const uint64 ElementSize    = GetBufferViewElementSize(ViewedBuffer->GetDesc(), BufferViewDesc);
                    if (ElementSize == 0)
                    {
                        D3D12_ERROR("[FD3D12ShaderBindingTable]: Root SRV at register %u has a zero element size and cannot be addressed.", uint32(Register));
                        break;
                    }

                    RootDescriptors[ParamIndex] = ViewedBuffer->GetGPUVirtualAddress() + uint64(BufferViewDesc.FirstElement) * ElementSize;
                }
                else if (bIsBuffer && ParamIndex >= D3D12_MAX_LOCAL_RECORD_ENTRIES)
                {
                    ReportBudgetOverflowIfNeeded(ParamIndex);
                }
                else if (SRVTableParamIndex >= 0)
                {
                    const int8 Slot = LocalRootSignature->GetSlotForRegister(EShaderVisibility::All, EResourceType::SRV, Register);
                    if (Slot >= 0 && static_cast<uint32>(Slot) < FD3D12PendingLocalTable::MaxDescriptors)
                    {
                        const uint32 SlotCount = static_cast<uint32>(Slot) + 1;
                        PendingShaderResourceViewTable.ShaderResourceViews[Slot] = ShaderResourceView;
                        PendingShaderResourceViewTable.NumDescriptors = Math::Max(SlotCount, PendingShaderResourceViewTable.NumDescriptors);
                        bHasShaderResourceViewTable = true;
                    }
                    else
                    {
                        ReportTableOverflowIfNeeded(Slot);
                    }
                }

                break;
            }

            case ERayTracingLocalBindingType::UnorderedAccessView:
            {
                FD3D12UnorderedAccessViewRHI* UnorderedAccessView = FD3D12DeviceRHI::ResourceCast(Binding.UnorderedAccessView);
                FD3D12BufferRHI*              ViewedBuffer        = (UnorderedAccessView && UnorderedAccessView->GetDesc().IsBufferUAV()) ? GetViewedBuffer(UnorderedAccessView->GetResource()) : nullptr;
                const bool                    bIsBuffer           = ViewedBuffer != nullptr;

                const int8 ParamIndex = Stage.GetRootDescriptorParameterIndex(EResourceType::UAV, Register);
                if (bIsBuffer && ParamIndex >= 0 && ParamIndex < D3D12_MAX_LOCAL_RECORD_ENTRIES)
                {
                    const auto&  BufferViewDesc = UnorderedAccessView->GetDesc().Buffer;
                    const uint64 ElementSize    = GetBufferViewElementSize(ViewedBuffer->GetDesc(), BufferViewDesc);
                    if (ElementSize == 0)
                    {
                        D3D12_ERROR("[FD3D12ShaderBindingTable]: Root UAV at register %u has a zero element size and cannot be addressed.", uint32(Register));
                        break;
                    }

                    RootDescriptors[ParamIndex] = ViewedBuffer->GetGPUVirtualAddress() + uint64(BufferViewDesc.FirstElement) * ElementSize;
                }
                else if (bIsBuffer && ParamIndex >= D3D12_MAX_LOCAL_RECORD_ENTRIES)
                {
                    ReportBudgetOverflowIfNeeded(ParamIndex);
                }
                else if (UAVTableParamIndex >= 0)
                {
                    const int8 Slot = LocalRootSignature->GetSlotForRegister(EShaderVisibility::All, EResourceType::UAV, Register);
                    if (Slot >= 0 && static_cast<uint32>(Slot) < FD3D12PendingLocalTable::MaxDescriptors)
                    {
                        PendingUnorderedAccessViewTable.UnorderedAccessViews[Slot] = UnorderedAccessView;
                        const uint32 SlotCount = static_cast<uint32>(Slot) + 1;
                        PendingUnorderedAccessViewTable.NumDescriptors = Math::Max(SlotCount, PendingUnorderedAccessViewTable.NumDescriptors);
                        bHasUnorderedAccessViewTable = true;
                    }
                    else
                    {
                        ReportTableOverflowIfNeeded(Slot);
                    }
                }

                break;
            }

            case ERayTracingLocalBindingType::SamplerState:
            {
                FD3D12SamplerStateRHI* SamplerState = FD3D12DeviceRHI::ResourceCast(Binding.Sampler);
                if (SamplerTableParamIndex >= 0 && SamplerState)
                {
                    const int8 Slot = LocalRootSignature->GetSlotForRegister(EShaderVisibility::All, EResourceType::Sampler, Register);
                    if (Slot >= 0 && static_cast<uint32>(Slot) < FD3D12PendingLocalTable::MaxDescriptors)
                    {
                        const uint32 SlotCount = static_cast<uint32>(Slot) + 1;
                        PendingSamplerTable.Samplers[Slot] = SamplerState;
                        PendingSamplerTable.NumDescriptors = Math::Max(SlotCount, PendingSamplerTable.NumDescriptors);
                        bHasSamplerTable = true;
                    }
                    else
                    {
                        ReportTableOverflowIfNeeded(Slot);
                    }
                }

                break;
            }

            default:
            {
                break;
            }
        }
    }

    if (bHasShaderResourceViewTable)
    {
        PendingLocalTables.Emplace(PendingShaderResourceViewTable);
    }

    if (bHasUnorderedAccessViewTable)
    {
        PendingLocalTables.Emplace(PendingUnorderedAccessViewTable);
    }

    if (bHasSamplerTable)
    {
        PendingLocalTables.Emplace(PendingSamplerTable);
    }
}

void FD3D12ShaderBindingTable::SetBindings(ERayTracingShaderRecordKind RecordKind, uint32 RecordIndex, const FRHIHitGroupLocalShaderBinding* Bindings, uint32 NumBindings)
{
    const uint32 SubTableBaseRecord  = GetSubTableBaseRecord(RecordKind);
    const uint32 GlobalRecordIndex   = SubTableBaseRecord + RecordIndex;
    const uint64 ByteOffset          = uint64(GlobalRecordIndex) * RecordStride;

    RemovePendingTablesInRange(ByteOffset, ByteOffset + RecordStride);

    if (ByteOffset + RecordStride > uint64(CpuShadow.SizeInBytes()))
    {
        return;
    }

    if (Pipeline)
    {
        String ExportName;
        Pipeline->GetExportName(RecordKind, RecordIndex, ExportName);

        FD3D12RootSignature* LocalRootSignature = Pipeline->GetLocalRootSignature(ExportName);
        PopulateRecord(LocalRootSignature, CpuShadow.Data() + ByteOffset, ByteOffset, RecordKind, RecordIndex, ExportName, Bindings, NumBindings);
    }
    else
    {
        PopulateRecord(nullptr, CpuShadow.Data() + ByteOffset, ByteOffset, RecordKind, RecordIndex, String(), Bindings, NumBindings);
    }
}

void FD3D12ShaderBindingTable::RemovePendingTablesInRange(uint64 BeginByteOffset, uint64 EndByteOffset)
{
    for (int32 i = PendingLocalTables.Size() - 1; i >= 0; --i)
    {
        const uint64 Offset = PendingLocalTables[i].RecordByteOffset;
        if (Offset >= BeginByteOffset && Offset < EndByteOffset)
        {
            PendingLocalTables.RemoveAt(i);
        }
    }
}

uint32 FD3D12ShaderBindingTable::GetNumPendingLocalTableDescriptors() const
{
    uint32 Total = 0;
    for (const FD3D12PendingLocalTable& Pending : PendingLocalTables)
    {
        if (IsResourceDescriptorHeap(Pending.DescriptorType))
        {
            Total += Pending.NumDescriptors;
        }
    }

    return Total;
}

uint32 FD3D12ShaderBindingTable::GetNumPendingLocalSamplerDescriptors() const
{
    uint32 Total = 0;
    for (const FD3D12PendingLocalTable& Pending : PendingLocalTables)
    {
        if (IsSamplerDescriptorHeap(Pending.DescriptorType))
        {
            Total += Pending.NumDescriptors;
        }
    }

    return Total;
}

void FD3D12ShaderBindingTable::ClearTableRecords()
{
    Memory::Memzero(CpuShadow.Data(), CpuShadow.SizeInBytes());
    PendingLocalTables.Clear();
}

void FD3D12ShaderBindingTable::UploadCpuShadow(FD3D12CommandContext& CmdContext)
{
    const uint64 RequiredSize = uint64(CpuShadow.SizeInBytes());
    if (RequiredSize == 0)
    {
        return;
    }

    CHECK(TableResourceStorage.GetResource() != nullptr);
    CHECK(TableResourceStorage.GetSize() >= RequiredSize);

    CmdContext.GetBarrierBatcher().AddTransitionBarrier(TableResourceStorage.GetResource(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    
    const uint64 UploadOffset = TableResourceStorage.GetResourceOffset();
    CmdContext.UpdateBuffer(TableResourceStorage.GetResource(), FBufferRegion(UploadOffset, RequiredSize), CpuShadow.Data());
    
    CmdContext.GetBarrierBatcher().AddTransitionBarrier(TableResourceStorage.GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void FD3D12ShaderBindingTable::Build(FD3D12CommandContext& CmdContext)
{
    UploadCpuShadow(CmdContext);
}

void FD3D12ShaderBindingTable::ResolveLocalDescriptorTables(FD3D12CommandContext& CmdContext, FD3D12LocalDescriptorHeap& ResourceHeap, FD3D12LocalDescriptorHeap& SamplerHeap)
{
    if (PendingLocalTables.IsEmpty())
    {
        return;
    }

    ID3D12Device* D3DDevice = GetDevice()->GetD3D12Device();

    const D3D12_CPU_DESCRIPTOR_HANDLE DefaultSRVHandle     = GetDevice()->GetDefaultDescriptors().DefaultSRV->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE DefaultUAVHandle     = GetDevice()->GetDefaultDescriptors().DefaultUAV->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE DefaultSamplerHandle = GetDevice()->GetDefaultDescriptors().DefaultSampler->GetOfflineHandle();

    struct FResolvedTable
    {
        uint32                         NumDescriptors;
        ED3D12LocalTableDescriptorType DescriptorType;
        SIZE_T                         SourceHandles[FD3D12PendingLocalTable::MaxDescriptors];
        uint32                         BaseHandle;
    };

    TArray<FResolvedTable> ResolvedTables;
    for (const FD3D12PendingLocalTable& Pending : PendingLocalTables)
    {
        if (Pending.NumDescriptors == 0)
        {
            continue;
        }

        const bool                       bIsSampler = IsSamplerDescriptorHeap(Pending.DescriptorType);
        FD3D12LocalDescriptorHeap&       TargetHeap = bIsSampler ? SamplerHeap : ResourceHeap;
        const D3D12_DESCRIPTOR_HEAP_TYPE HeapType   = bIsSampler ? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER : D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        SIZE_T SourceHandles[FD3D12PendingLocalTable::MaxDescriptors] = {};
        for (uint32 SlotIndex = 0; SlotIndex < Pending.NumDescriptors; ++SlotIndex)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE Src = DefaultSRVHandle;
            switch (Pending.DescriptorType)
            {
                case ED3D12LocalTableDescriptorType::ShaderResourceView:
                {
                    FD3D12ShaderResourceViewRHI* ShaderResourceView = Pending.ShaderResourceViews[SlotIndex];
                    Src = ShaderResourceView ? ShaderResourceView->GetOfflineHandle() : DefaultSRVHandle;
                    break;
                }

                case ED3D12LocalTableDescriptorType::UnorderedAccessView:
                {
                    FD3D12UnorderedAccessViewRHI* UnorderedAccessView = Pending.UnorderedAccessViews[SlotIndex];
                    Src = UnorderedAccessView ? UnorderedAccessView->GetOfflineHandle() : DefaultUAVHandle;
                    break;
                }

                case ED3D12LocalTableDescriptorType::Sampler:
                {
                    FD3D12SamplerStateRHI* SamplerState = Pending.Samplers[SlotIndex];
                    Src = SamplerState ? SamplerState->GetOfflineHandle() : DefaultSamplerHandle;
                    break;
                }
            }

            SourceHandles[SlotIndex] = Src.ptr;
        }

        uint32 BaseHandle = 0;
        
        bool bFoundInCache = false;
        for (const FResolvedTable& Resolved : ResolvedTables)
        {
            if (Resolved.NumDescriptors != Pending.NumDescriptors || Resolved.DescriptorType != Pending.DescriptorType)
            {
                continue;
            }

            if (Memory::Memcmp(Resolved.SourceHandles, SourceHandles, sizeof(SIZE_T) * Pending.NumDescriptors) == 0)
            {
                BaseHandle    = Resolved.BaseHandle;
                bFoundInCache = true;
                break;
            }
        }

        if (!bFoundInCache)
        {
            BaseHandle = TargetHeap.AllocateHandles(Pending.NumDescriptors);
            for (uint32 SlotIndex = 0; SlotIndex < Pending.NumDescriptors; ++SlotIndex)
            {
                const D3D12_CPU_DESCRIPTOR_HANDLE Src = { SourceHandles[SlotIndex] };
                const D3D12_CPU_DESCRIPTOR_HANDLE Dst = TargetHeap.GetCPUHandle(int32(BaseHandle + SlotIndex));
                D3DDevice->CopyDescriptorsSimple(1, Dst, Src, HeapType);
            }

            FResolvedTable Resolved;
            Resolved.NumDescriptors = Pending.NumDescriptors;
            Resolved.DescriptorType = Pending.DescriptorType;
            Resolved.BaseHandle     = BaseHandle;
            
            Memory::Memcpy(Resolved.SourceHandles, SourceHandles, sizeof(SourceHandles));
            ResolvedTables.Emplace(Resolved);
        }

        const D3D12_GPU_DESCRIPTOR_HANDLE TableGpuHandle = TargetHeap.GetGPUHandle(int32(BaseHandle));
        if (Pending.RecordByteOffset + RecordStride <= uint64(CpuShadow.SizeInBytes()))
        {
            D3D12_GPU_VIRTUAL_ADDRESS* RootDescriptors = reinterpret_cast<D3D12_GPU_VIRTUAL_ADDRESS*>(CpuShadow.Data() + Pending.RecordByteOffset + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
            if (Pending.TableParamSlot < D3D12_MAX_LOCAL_RECORD_ENTRIES)
            {
                RootDescriptors[Pending.TableParamSlot] = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(TableGpuHandle.ptr);
            }
        }
    }

    UploadCpuShadow(CmdContext);
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE FD3D12ShaderBindingTable::GetRayGenRecord() const
{
    CHECK(TableResourceStorage.GetResource() != nullptr);
    const uint64 BaseAddress = TableResourceStorage.GetGPUVirtualAddress();
    return { BaseAddress, RecordStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE FD3D12ShaderBindingTable::GetMissTable() const
{
    CHECK(TableResourceStorage.GetResource() != nullptr);
    if (NumMiss == 0)
    {
        return { 0, 0, 0 };
    }

    const uint64 BaseAddress = TableResourceStorage.GetGPUVirtualAddress() + uint64(GetSubTableBaseRecord(ERayTracingShaderRecordKind::Miss)) * RecordStride;
    return { BaseAddress, uint64(NumMiss) * RecordStride, RecordStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE FD3D12ShaderBindingTable::GetHitGroupTable() const
{
    CHECK(TableResourceStorage.GetResource() != nullptr);
    if (NumHitGroup == 0)
    {
        return { 0, 0, 0 };
    }

    const uint64 BaseAddress = TableResourceStorage.GetGPUVirtualAddress() + uint64(GetSubTableBaseRecord(ERayTracingShaderRecordKind::HitGroup)) * RecordStride;
    return { BaseAddress, uint64(NumHitGroup) * RecordStride, RecordStride };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE FD3D12ShaderBindingTable::GetCallableTable() const
{
    CHECK(TableResourceStorage.GetResource() != nullptr);

    if (NumCallable == 0)
    {
        return { 0, 0, 0 };
    }

    const uint64 BaseAddress = TableResourceStorage.GetGPUVirtualAddress() + uint64(GetSubTableBaseRecord(ERayTracingShaderRecordKind::Callable)) * RecordStride;
    return { BaseAddress, uint64(NumCallable) * RecordStride, RecordStride };
}
