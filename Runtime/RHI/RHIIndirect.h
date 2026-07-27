#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"
#include "RHI/RayTracing/RHIShaderBindingTable.h"

struct FRHIDrawIndirectParameters
{
    uint32 VertexCountPerInstance = 0;
    uint32 InstanceCount          = 0;
    uint32 StartVertexLocation    = 0;
    uint32 StartInstanceLocation  = 0;
};

struct FRHIDrawIndexedIndirectParameters
{
    uint32 IndexCountPerInstance = 0;
    uint32 InstanceCount         = 0;
    uint32 StartIndexLocation    = 0;
    int32  BaseVertexLocation    = 0;
    uint32 StartInstanceLocation = 0;
};

struct FRHIDispatchIndirectParameters
{
    uint32 ThreadGroupCountX = 0;
    uint32 ThreadGroupCountY = 0;
    uint32 ThreadGroupCountZ = 0;
};

struct FRHIDispatchMeshIndirectParameters
{
    uint32 ThreadGroupCountX = 0;
    uint32 ThreadGroupCountY = 0;
    uint32 ThreadGroupCountZ = 0;
};

struct FRHIShaderRecordAddressRange
{
    uint64 StartAddress = 0;
    uint64 SizeInBytes  = 0;
};

struct FRHIShaderTableAddressRange
{
    uint64 StartAddress  = 0;
    uint64 SizeInBytes   = 0;
    uint64 StrideInBytes = 0;
};

struct alignas(8) FRHIDispatchRaysIndirectParameters
{
    static FORCEINLINE FRHIDispatchRaysIndirectParameters FromShaderBindingTable(const FRHIShaderBindingTableAddressInfo& AddressInfo, uint32 InWidth, uint32 InHeight, uint32 InDepth)
    {
        FRHIDispatchRaysIndirectParameters Parameters = {};
        Parameters.RayGenerationShaderRecord.StartAddress = AddressInfo.RayGeneration.StartAddress;
        Parameters.RayGenerationShaderRecord.SizeInBytes  = AddressInfo.RayGeneration.SizeInBytes;

        Parameters.MissShaderTable.StartAddress  = AddressInfo.Miss.StartAddress;
        Parameters.MissShaderTable.SizeInBytes   = AddressInfo.Miss.SizeInBytes;
        Parameters.MissShaderTable.StrideInBytes = AddressInfo.Miss.StrideInBytes;

        Parameters.HitGroupTable.StartAddress  = AddressInfo.HitGroup.StartAddress;
        Parameters.HitGroupTable.SizeInBytes   = AddressInfo.HitGroup.SizeInBytes;
        Parameters.HitGroupTable.StrideInBytes = AddressInfo.HitGroup.StrideInBytes;

        Parameters.CallableShaderTable.StartAddress  = AddressInfo.Callable.StartAddress;
        Parameters.CallableShaderTable.SizeInBytes   = AddressInfo.Callable.SizeInBytes;
        Parameters.CallableShaderTable.StrideInBytes = AddressInfo.Callable.StrideInBytes;

        Parameters.Width  = InWidth;
        Parameters.Height = InHeight;
        Parameters.Depth  = InDepth;
        return Parameters;
    }

    FRHIShaderRecordAddressRange RayGenerationShaderRecord;
    FRHIShaderTableAddressRange  MissShaderTable;
    FRHIShaderTableAddressRange  HitGroupTable;
    FRHIShaderTableAddressRange  CallableShaderTable;
    uint32                       Width   = 0;
    uint32                       Height  = 0;
    uint32                       Depth   = 0;
    uint32                       Padding = 0;
};

struct FRHIIndirectLayout
{
    static constexpr uint32 DrawRecordSize         = 16;
    static constexpr uint32 DrawIndexedRecordSize  = 20;
    static constexpr uint32 DispatchRecordSize     = 12;
    static constexpr uint32 DispatchMeshRecordSize = 12;

    static constexpr uint32 RayGenerationOffset = 0;
    static constexpr uint32 MissOffset          = 16;
    static constexpr uint32 HitGroupOffset      = 40;
    static constexpr uint32 CallableOffset      = 64;
    static constexpr uint32 WidthOffset         = 88;
    static constexpr uint32 HeightOffset        = 92;
    static constexpr uint32 DepthOffset         = 96;
    static constexpr uint32 PaddingOffset       = 100;
    static constexpr uint32 RecordSize          = 104;
};

static_assert(sizeof(FRHIDrawIndirectParameters) == FRHIIndirectLayout::DrawRecordSize);
static_assert(sizeof(FRHIDrawIndexedIndirectParameters) == FRHIIndirectLayout::DrawIndexedRecordSize);
static_assert(sizeof(FRHIDispatchIndirectParameters) == FRHIIndirectLayout::DispatchRecordSize);
static_assert(sizeof(FRHIDispatchMeshIndirectParameters) == FRHIIndirectLayout::DispatchMeshRecordSize);
static_assert(sizeof(FRHIDispatchRaysIndirectParameters) == FRHIIndirectLayout::RecordSize);
static_assert(alignof(FRHIDispatchRaysIndirectParameters) == 8);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, RayGenerationShaderRecord) == FRHIIndirectLayout::RayGenerationOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, MissShaderTable) == FRHIIndirectLayout::MissOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, HitGroupTable) == FRHIIndirectLayout::HitGroupOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, CallableShaderTable) == FRHIIndirectLayout::CallableOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Width) == FRHIIndirectLayout::WidthOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Height) == FRHIIndirectLayout::HeightOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Depth) == FRHIIndirectLayout::DepthOffset);
static_assert(OFFSETOF(FRHIDispatchRaysIndirectParameters, Padding) == FRHIIndirectLayout::PaddingOffset);

