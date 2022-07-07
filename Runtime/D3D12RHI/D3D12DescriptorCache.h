#pragma once
#include "D3D12RootSignature.h"
#include "D3D12Descriptors.h"
#include "D3D12CommandList.h"
#include "D3D12Buffer.h"
#include "D3D12ResourceViews.h"
#include "D3D12SamplerState.h"

class FD3D12CommandBatch;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12ViewCache

template <typename ViewType, D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint32 kDescriptorTableSize>
class TD3D12ViewCache
{
public:

    static CONSTEXPR D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapType()  { return HeapType; }
    static CONSTEXPR uint32                     GetDescriptorTableSize() { return kDescriptorTableSize; }

public:

    TD3D12ViewCache()
        : ResourceViews()
        , CPUDescriptorTables()
        , GPUDescriptorTables()
        , CopyDescriptors()
        , bDirty()
    { }

    void BindView(EShaderVisibility Visibility, ViewType* DescriptorView, uint32 ShaderRegister)
    {
        D3D12_ERROR_COND(DescriptorView != nullptr, "Trying to bind a ResourceView that was nullptr, check input from DescriptorCache");

        ViewType* CurrentDescriptorView = ResourceViews[Visibility][ShaderRegister];
        if (DescriptorView != CurrentDescriptorView)
        {
            ResourceViews[Visibility][ShaderRegister] = DescriptorView;
            bDirty[Visibility] = true;
        }
    }

    void Reset(ViewType* DefaultView)
    {
        FMemory::Memzero(CPUDescriptorTables, sizeof(CPUDescriptorTables));
        FMemory::Memzero(GPUDescriptorTables, sizeof(GPUDescriptorTables));
        FMemory::Memzero(CopyDescriptors    , sizeof(CopyDescriptors));

        for (uint32 Stage = 0; Stage < ShaderVisibility_Count; ++Stage)
        {
            for (uint32 Index = 0; Index < kDescriptorTableSize; ++Index)
            {
                ResourceViews[Stage][Index] = DefaultView;
            }

            bDirty[Stage] = true;
        }
    }

    FORCEINLINE uint32 GetNumDescriptorsForStage(EShaderVisibility Stage) const
    {
        return bDirty[Stage] ? kDescriptorTableSize : 0;
    }

    void PrepareDescriptorsForCopy(EShaderVisibility Stage)
    {
        Check(bDirty[Stage]);

        for (uint32 Index = 0; Index < kDescriptorTableSize; Index++)
        {
            ViewType* View = ResourceViews[Stage][Index];
            Check(View != nullptr);

            CopyDescriptors[Stage][Index] = View->GetOfflineHandle();
        }
    }

    FORCEINLINE void SetDescriptorTable(EShaderVisibility Stage, D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle)
    {
        Check(bDirty[Stage]);

        CPUDescriptorTables[Stage] = CPUHandle;
        GPUDescriptorTables[Stage] = GPUHandle;
    }

    void InvalidateAll()
    {
        for (uint32 Index = 0; Index < D3D12_CACHED_DESCRIPTORS_NUM_STAGES; Index++)
        {
            bDirty[Index] = true;
        }
    }

    ViewType* ResourceViews[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    D3D12_CPU_DESCRIPTOR_HANDLE CopyDescriptors[D3D12_CACHED_DESCRIPTORS_NUM_STAGES][kDescriptorTableSize];

    D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorTables[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorTables[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
    bool bDirty[D3D12_CACHED_DESCRIPTORS_NUM_STAGES];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

using FD3D12ConstantBufferViewCache  = TD3D12ViewCache<FD3D12ConstantBufferView , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_CONSTANT_BUFFER_COUNT>;
using FD3D12ShaderResourceViewCache  = TD3D12ViewCache<FD3D12ShaderResourceView , D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_COUNT>;
using FD3D12UnorderedAccessViewCache = TD3D12ViewCache<FD3D12UnorderedAccessView, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_COUNT>;
using FD3D12SamplerStateCache        = TD3D12ViewCache<FD3D12SamplerState       , D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER    , D3D12_DEFAULT_SAMPLER_STATE_COUNT>;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12PipelineStageMask

struct FD3D12PipelineStageMask
{
    static CONSTEXPR FD3D12PipelineStageMask ComputeMask()
    {
        return FD3D12PipelineStageMask(1, 0, 0, 0, 0, 0);
    }

    static CONSTEXPR FD3D12PipelineStageMask BasicGraphicsMask()
    {
        return FD3D12PipelineStageMask(0, 1, 0, 0, 0, 1);
    }

    static CONSTEXPR FD3D12PipelineStageMask FullGraphicsMask()
    {
        return FD3D12PipelineStageMask(0, 1, 1, 1, 1, 1);
    }

    CONSTEXPR FD3D12PipelineStageMask()
        : ComputeStage(0)
        , VertexStage(0)
        , HullStage(0)
        , DomainStage(0)
        , GeometryStage(0)
        , PixelStage(0)
    { }

    CONSTEXPR FD3D12PipelineStageMask( uint8 InComputeStage
                                     , uint8 InVertexStage
                                     , uint8 InHullStage
                                     , uint8 InDomainStage
                                     , uint8 InGeometryStage
                                     , uint8 InPixelStage)
        : ComputeStage(InComputeStage)
        , VertexStage(InVertexStage)
        , HullStage(InHullStage)
        , DomainStage(InDomainStage)
        , GeometryStage(InGeometryStage)
        , PixelStage(InPixelStage)
    { }

    CONSTEXPR bool CheckShaderVisibility(EShaderVisibility ShaderVisibility) const
    {
        const uint8 Flag = (1 << ShaderVisibility);
        return (*GetClassAsData<const uint8>(this)) & Flag;
    }

    CONSTEXPR bool operator==(FD3D12PipelineStageMask RHS) const
    {
        return (ComputeStage  == RHS.ComputeStage)
            && (VertexStage   == RHS.VertexStage)
            && (HullStage     == RHS.HullStage)
            && (DomainStage   == RHS.DomainStage)
            && (GeometryStage == RHS.GeometryStage)
            && (PixelStage    == RHS.PixelStage);
    }

    CONSTEXPR bool operator!=(FD3D12PipelineStageMask RHS) const
    {
        return !(*this == RHS);
    }

    uint8 ComputeStage  : 1;
    uint8 VertexStage   : 1;
    uint8 HullStage     : 1;
    uint8 DomainStage   : 1;
    uint8 GeometryStage : 1;
    uint8 PixelStage    : 1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12VertexBufferCache

struct FD3D12VertexBufferCache
{
    FD3D12VertexBufferCache()
        : NumVertexBuffers(0)
        , VBViews()
        , VBResources()
    {
        Clear();
    }

    void Clear()
    {
        FMemory::Memzero(VBViews    , sizeof(VBViews));
        FMemory::Memzero(VBResources, sizeof(VBResources));
        NumVertexBuffers = 0;
    }

    uint32                   NumVertexBuffers;
    D3D12_VERTEX_BUFFER_VIEW VBViews[D3D12_MAX_VERTEX_BUFFER_SLOTS];
    FD3D12Resource*          VBResources[D3D12_MAX_VERTEX_BUFFER_SLOTS];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12IndexBufferCache

struct FD3D12IndexBufferCache
{
    FD3D12IndexBufferCache()
        : IBView()
        , IBResource(nullptr)
    {
        Clear();
    }

    void Clear()
    {
        IBView.Format         = DXGI_FORMAT_R32_UINT;
        IBView.BufferLocation = 0;
        IBView.SizeInBytes    = 0;

        IBResource = nullptr;
    }

    D3D12_INDEX_BUFFER_VIEW IBView;
    FD3D12Resource*         IBResource;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12RenderTargetViewCache

struct FD3D12RenderTargetViewCache
{
    FD3D12RenderTargetViewCache()
        : RenderTargetViews()
        , NumRenderTargets(0)
    {
        Clear();
    }

    void SetRenderTarget(FD3D12RenderTargetView* View, uint32 Slot)
    {
        Check(Slot < ArrayCount(RenderTargetViews));
        RenderTargetViews[Slot] = View;
    }

    void Clear()
    {
        FMemory::Memzero(RenderTargetViews, sizeof(RenderTargetViews));
        NumRenderTargets = 0;
    }

    FD3D12RenderTargetView* RenderTargetViews[D3D12_MAX_RENDER_TARGET_COUNT];
    uint32                  NumRenderTargets;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ConstantBufferCache

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TD3D12ResourceViewCache

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12SamplerStateCache

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12DescriptorCache

class FD3D12DescriptorCache : public FD3D12DeviceChild
{
public:

    FD3D12DescriptorCache(FD3D12Device* Device);
    
    ~FD3D12DescriptorCache()
    {
        SafeDelete(NullCBV);
    }

    bool Initialize();

    void PrepareGraphicsDescriptors(FD3D12CommandBatch* CommandBatch, FD3D12RootSignature* RootSignature, FD3D12PipelineStageMask PipelineMask);
    void PrepareComputeDescriptors(FD3D12CommandBatch* CommandBatch, FD3D12RootSignature* RootSignature);
    
    void SetCurrentCommandList(FD3D12CommandList* InCommandList)
    {
        CurrentCommandList = InCommandList;
    }

    void SetRenderTargets(FD3D12RenderTargetViewCache& RenderTargets, FD3D12DepthStencilView* DepthStencil);

    void SetVertexBuffers(FD3D12VertexBufferCache& VertexBuffers);
    void SetIndexBuffer(FD3D12IndexBufferCache& IndexBuffer);

    void Clear();

    FORCEINLINE void SetConstantBufferView(EShaderVisibility Visibility, FD3D12ConstantBufferView* Descriptor, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullCBV;
        }

        ConstantBufferViewCache.BindView(Visibility, Descriptor, ShaderRegister);
    }

    FORCEINLINE void SetShaderResourceView(EShaderVisibility Visibility, FD3D12ShaderResourceView* Descriptor, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSRV.Get();
        }

        ShaderResourceViewCache.BindView(Visibility, Descriptor, ShaderRegister);
    }

    FORCEINLINE void SetUnorderedAccessView(EShaderVisibility Visibility, FD3D12UnorderedAccessView* Descriptor, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullUAV.Get();
        }

        UnorderedAccessViewCache.BindView(Visibility, Descriptor, ShaderRegister);
    }

    FORCEINLINE void SetSamplerState(EShaderVisibility Visibility, FD3D12SamplerState* Descriptor, uint32 ShaderRegister)
    {
        if (!Descriptor)
        {
            Descriptor = NullSampler.Get();
        }

        SamplerStateCache.BindView(Visibility, Descriptor, ShaderRegister);
    }

private:
    void AllocateDescriptorsAndSetHeaps( ID3D12GraphicsCommandList* CommandList
                                       , FD3D12OnlineDescriptorManager* ResourceDescriptors
                                       , FD3D12OnlineDescriptorManager* SamplerDescriptors
                                       , FD3D12PipelineStageMask PipelineMask);

    template<typename TResourveViewCache>
    void CopyAndBindComputeDescriptors( ID3D12Device* DxDevice
                                      , ID3D12GraphicsCommandList* CommandList
                                      , TResourveViewCache& ResourceViewCache
                                      , int32 ParameterIndex)
    {
        if (ParameterIndex >= 0 && ResourceViewCache.bDirty[ShaderVisibility_All])
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges  = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcCPUStartHandles = ResourceViewCache.CopyDescriptors[ShaderVisibility_All];

            D3D12_CPU_DESCRIPTOR_HANDLE DstCPUTableHandle = ResourceViewCache.CPUDescriptorTables[ShaderVisibility_All];
            DxDevice->CopyDescriptors(1, &DstCPUTableHandle, &DestRangeSize, NumSrcRanges, SrcCPUStartHandles, RangeSizes, TResourveViewCache::GetDescriptorHeapType());

            D3D12_GPU_DESCRIPTOR_HANDLE GPUTableHandle = ResourceViewCache.GPUDescriptorTables[ShaderVisibility_All];
            CommandList->SetComputeRootDescriptorTable(ParameterIndex, GPUTableHandle);

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.bDirty[ShaderVisibility_All] = false;
        }
    }

    template<typename TResourveViewCache>
    void CopyAndBindGraphicsDescriptors( ID3D12Device* DxDevice
                                       , ID3D12GraphicsCommandList* CommandList
                                       , TResourveViewCache& ResourceViewCache
                                       , int32 ParameterIndex
                                       , EShaderVisibility ShaderVisibility)
    {
        if (ParameterIndex >= 0 && ResourceViewCache.bDirty[ShaderVisibility])
        {
            const UINT DestRangeSize = TResourveViewCache::GetDescriptorTableSize();
            const UINT NumSrcRanges  = TResourveViewCache::GetDescriptorTableSize();

            const D3D12_CPU_DESCRIPTOR_HANDLE* SrcCPUStartHandles = ResourceViewCache.CopyDescriptors[ShaderVisibility];

            D3D12_CPU_DESCRIPTOR_HANDLE DstCPUTableHandle = ResourceViewCache.CPUDescriptorTables[ShaderVisibility];
            DxDevice->CopyDescriptors(1, &DstCPUTableHandle, &DestRangeSize, NumSrcRanges, SrcCPUStartHandles, RangeSizes, TResourveViewCache::GetDescriptorHeapType());

            D3D12_GPU_DESCRIPTOR_HANDLE GPUTableHandle = ResourceViewCache.GPUDescriptorTables[ShaderVisibility];
            CommandList->SetGraphicsRootDescriptorTable(ParameterIndex, GPUTableHandle);

            // When the descriptors are copied and bound, then the descriptors are not dirty anymore
            ResourceViewCache.bDirty[ShaderVisibility] = false;
        }
    }

    FD3D12CommandList*             CurrentCommandList;

    ID3D12DescriptorHeap*          PreviousDescriptorHeaps[2] = { nullptr, nullptr };

    FD3D12ConstantBufferView*      NullCBV;
    FD3D12ShaderResourceViewRef    NullSRV;
    FD3D12UnorderedAccessViewRef   NullUAV;
    FD3D12RenderTargetViewRef      NullRTV;
    FD3D12SamplerStateRef          NullSampler;

    FD3D12ShaderResourceViewCache  ShaderResourceViewCache;
    FD3D12UnorderedAccessViewCache UnorderedAccessViewCache;
    FD3D12ConstantBufferViewCache  ConstantBufferViewCache;
    FD3D12SamplerStateCache        SamplerStateCache;

    UINT RangeSizes[D3D12_CACHED_DESCRIPTORS_COUNT];
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12ShaderConstantsCache

class FD3D12ShaderConstantsCache
{
public:

    FD3D12ShaderConstantsCache()
        : Constants()
        , NumConstants()
        , bIsDirty(false)
    {
        Reset();
    }

    FORCEINLINE void Set32BitShaderConstants(const uint32* InConstants, uint32 InNumConstants)
    {
        D3D12_ERROR_COND( InNumConstants <= D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT
                        , "Trying to set a number of shader-constants (NumConstants=%u) higher than the maximum (MaxShaderConstants=%u)"
                        , InNumConstants
                        , D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT);

        FMemory::Memcpy(Constants, InConstants, sizeof(uint32) * InNumConstants);
        NumConstants = InNumConstants;

        bIsDirty = true;
    }

    FORCEINLINE void CommitGraphics(FD3D12CommandList& CmdList, FD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ((RootIndex >= 0) && bIsDirty)
        {
            DxCmdList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
            bIsDirty = false;
        }
    }

    FORCEINLINE void CommitCompute(FD3D12CommandList& CmdList, FD3D12RootSignature* RootSignature)
    {
        ID3D12GraphicsCommandList* DxCmdList = CmdList.GetGraphicsCommandList();

        int32 RootIndex = RootSignature->Get32BitConstantsIndex();
        if ((RootIndex >= 0) && bIsDirty)
        {
            DxCmdList->SetComputeRoot32BitConstants(RootIndex, NumConstants, Constants, 0);
            bIsDirty = false;
        }
    }

    FORCEINLINE void Reset()
    {
        FMemory::Memzero(Constants, sizeof(Constants));
        NumConstants = 0;

        bIsDirty = true;
    }

private:
    uint32 Constants[D3D12_MAX_32BIT_SHADER_CONSTANTS_COUNT];
    uint32 NumConstants;

    bool   bIsDirty;
};