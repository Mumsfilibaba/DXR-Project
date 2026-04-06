#pragma once
#include "Core/Core.h"
#include "Core/Stats/Stats.h"
#include <d3d12.h>

#ifndef D3D12_ENABLE_STATS
    #define D3D12_ENABLE_STATS (STATS_ENABLED)
#endif

#ifndef D3D12_ENABLE_PIX_MARKERS
    #define D3D12_ENABLE_PIX_MARKERS (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_CRASH_MARKERS
    #define D3D12_ENABLE_CRASH_MARKERS (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_GPU_VALIDATION
    #define D3D12_ENABLE_GPU_VALIDATION (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_MEMORY_LOGGING
    #define D3D12_ENABLE_MEMORY_LOGGING (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_LOGGING
    #define D3D12_ENABLE_LOGGING (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_DEVICE_LOST_CHECK
    #define D3D12_ENABLE_DEVICE_LOST_CHECK (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_BINDING_DEBUG_NAMES
    #define D3D12_ENABLE_BINDING_DEBUG_NAMES (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_BINDING_VALIDATION
    #define D3D12_ENABLE_BINDING_VALIDATION (0)
#endif

#ifndef D3D12_ENABLE_RESOURCE_STATE_VALIDATION
    #define D3D12_ENABLE_RESOURCE_STATE_VALIDATION (0)
#endif

#ifndef D3D12_ENABLE_RESIDENCY_LOGGING
    #define D3D12_ENABLE_RESIDENCY_LOGGING (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_PIPELINE_STATE_STREAM
    #define D3D12_ENABLE_PIPELINE_STATE_STREAM (1)
#endif

#ifndef D3D12_ENABLE_VERSIONED_ROOT_SIGNATURES
    #define D3D12_ENABLE_VERSIONED_ROOT_SIGNATURES (1)
#endif

#ifndef D3D12_ENABLE_DEBUG_MESSAGE_CALLBACK
    #define D3D12_ENABLE_DEBUG_MESSAGE_CALLBACK (!RELEASE_BUILD)
#endif

#ifndef D3D12_ENABLE_STATIC_DESCRIPTORS
    #define D3D12_ENABLE_STATIC_DESCRIPTORS (0)
#endif


// Only enable the static descriptors if we have versioned root-signatures
#if D3D12_ENABLE_STATIC_DESCRIPTORS && !D3D12_ENABLE_VERSIONED_ROOT_SIGNATURES
    #undef  D3D12_ENABLE_STATIC_DESCRIPTORS
    #define D3D12_ENABLE_STATIC_DESCRIPTORS (0)
#endif

#ifndef D3D12_BREAK_ON_HASH_COLLISION
    #if DEBUG_BUILD
        #define D3D12_BREAK_ON_HASH_COLLISION (1)
    #else
        #define D3D12_BREAK_ON_HASH_COLLISION (0)
    #endif
#endif

#if D3D12_ENABLE_VERSIONED_ROOT_SIGNATURES && defined(__ID3D12VersionedRootSignatureDeserializer_INTERFACE_DEFINED__)
    #define D3D12_USE_VERSIONED_ROOT_SIGNATURES (1)
#else
    #define D3D12_USE_VERSIONED_ROOT_SIGNATURES (0)
#endif

#if D3D12_ENABLE_DEBUG_MESSAGE_CALLBACK && defined(__ID3D12InfoQueue1_INTERFACE_DEFINED__)
    #define D3D12_USE_DEBUG_MESSAGE_CALLBACK (1)
#else
    #define D3D12_USE_DEBUG_MESSAGE_CALLBACK (0)
#endif

// -------------------------------------------
// Query heap type availability
// -------------------------------------------

#if defined(D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1) && defined(D3D12_QUERY_TYPE_PIPELINE_STATISTICS1)
    #define D3D12_SUPPORT_PIPELINE_STATISTICS1 (1)
#else
    #define D3D12_SUPPORT_PIPELINE_STATISTICS1 (0)
#endif

// -------------------------------------------
// ID3D12Device interface availability
// -------------------------------------------

#ifdef __ID3D12Device1_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_1 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_1 (0)
#endif

#ifdef __ID3D12Device2_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_2 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_2 (0)
#endif

#ifdef __ID3D12Device3_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_3 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_3 (0)
#endif

#ifdef __ID3D12Device4_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_4 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_4 (0)
#endif

#ifdef __ID3D12Device5_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_5 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_5 (0)
#endif

#ifdef __ID3D12Device6_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_6 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_6 (0)
#endif

#ifdef __ID3D12Device7_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_7 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_7 (0)
#endif

#ifdef __ID3D12Device8_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_8 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_8 (0)
#endif

#ifdef __ID3D12Device9_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_9 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_9 (0)
#endif

#ifdef __ID3D12Device10_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_10 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_10 (0)
#endif

#ifdef __ID3D12Device11_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_11 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_11 (0)
#endif

#ifdef __ID3D12Device12_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_12 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_12 (0)
#endif

#ifdef __ID3D12Device13_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_13 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_13 (0)
#endif

#ifdef __ID3D12Device14_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12DEVICE_14 (1)
#else
    #define D3D12_USE_ID3D12DEVICE_14 (0)
#endif

// -------------------------------------------
// ID3D12CommandList interface availability
// -------------------------------------------

#ifdef __ID3D12GraphicsCommandList1_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_1 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_1 (0)
#endif

#ifdef __ID3D12GraphicsCommandList2_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_2 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_2 (0)
#endif

#ifdef __ID3D12GraphicsCommandList3_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_3 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_3 (0)
#endif

#ifdef __ID3D12GraphicsCommandList4_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_4 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_4 (0)
#endif

#ifdef __ID3D12GraphicsCommandList5_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_5 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_5 (0)
#endif

#ifdef __ID3D12GraphicsCommandList6_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_6 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_6 (0)
#endif

#ifdef __ID3D12GraphicsCommandList7_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_7 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_7 (0)
#endif

#ifdef __ID3D12GraphicsCommandList8_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_8 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_8 (0)
#endif

#ifdef __ID3D12GraphicsCommandList9_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_9 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_9 (0)
#endif

#ifdef __ID3D12GraphicsCommandList10_INTERFACE_DEFINED__
    #define D3D12_USE_ID3D12COMMANDLIST_10 (1)
#else
    #define D3D12_USE_ID3D12COMMANDLIST_10 (0)
#endif
