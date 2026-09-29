#pragma once
#include "Core/Memory/Memory.h"
#include "MetalRHI/MetalCore.h"

template<EShaderVisibility::Type Stage>
struct TMetalStageEncoder;

#define METAL_DECLARE_STAGE_ENCODER(Stage, EncoderProtocol, Prefix)                                                                                                                                             \
    template<>                                                                                                                                                                                                  \
    struct TMetalStageEncoder<Stage>                                                                                                                                                                            \
    {                                                                                                                                                                                                           \
        using EncoderType = id<EncoderProtocol>;                                                                                                                                                                \
                                                                                                                                                                                                                \
        static FORCEINLINE void SetBufferOffset(EncoderType Encoder, NSUInteger Offset, NSUInteger Slot)                                  { [Encoder set##Prefix##BufferOffset:Offset atIndex:Slot]; }           \
        static FORCEINLINE void SetBytes(EncoderType Encoder, const void* Bytes, NSUInteger Size, NSUInteger Slot)                        { [Encoder set##Prefix##Bytes:Bytes length:Size atIndex:Slot]; }        \
        static FORCEINLINE void SetBuffers(EncoderType Encoder, const id<MTLBuffer>* Buffers, const NSUInteger* Offsets, NSRange Range)   { [Encoder set##Prefix##Buffers:Buffers offsets:Offsets withRange:Range]; } \
        static FORCEINLINE void SetTextures(EncoderType Encoder, const id<MTLTexture>* Textures, NSRange Range)                           { [Encoder set##Prefix##Textures:Textures withRange:Range]; }          \
        static FORCEINLINE void SetSamplerStates(EncoderType Encoder, const id<MTLSamplerState>* Samplers, NSRange Range)                 { [Encoder set##Prefix##SamplerStates:Samplers withRange:Range]; }     \
    };

METAL_DECLARE_STAGE_ENCODER(EShaderVisibility::Vertex,        MTLRenderCommandEncoder,  Vertex)
METAL_DECLARE_STAGE_ENCODER(EShaderVisibility::Pixel,         MTLRenderCommandEncoder,  Fragment)
METAL_DECLARE_STAGE_ENCODER(EShaderVisibility::Mesh,          MTLRenderCommandEncoder,  Mesh)
METAL_DECLARE_STAGE_ENCODER(EShaderVisibility::Amplification, MTLRenderCommandEncoder,  Object)
METAL_DECLARE_STAGE_ENCODER(EShaderVisibility::Compute,       MTLComputeCommandEncoder, )

#undef METAL_DECLARE_STAGE_ENCODER

class FMetalEncoderBindingCache
{
public:
    static constexpr uint32 MaxBuffers  = 31;
    static constexpr uint32 MaxTextures = 128;
    static constexpr uint32 MaxSamplers = 16;

    FMetalEncoderBindingCache()
    {
        Memory::Memzero(Stages, sizeof(Stages));
    }

    void ResetRenderStages()
    {
        Memory::Memzero(&Stages[EShaderVisibility::Vertex], sizeof(FStageSlots) * (EShaderVisibility::Amplification - EShaderVisibility::Vertex + 1));
    }

    void ResetComputeStage()
    {
        Memory::Memzero(&Stages[EShaderVisibility::Compute], sizeof(FStageSlots));
    }

    template<EShaderVisibility::Type Stage>
    FORCEINLINE void SetBuffer(typename TMetalStageEncoder<Stage>::EncoderType Encoder, id<MTLBuffer> Buffer, NSUInteger Offset, uint8 Slot)
    {
        CHECK(Slot < MaxBuffers);

        FStageSlots& Slots = Stages[Stage];
        const uint64 Bit   = 1ull << Slot;

        if (Slots.Buffers[Slot] == Buffer)
        {
            if (Slots.Offsets[Slot] != Offset)
            {
                Slots.Offsets[Slot] = Offset;

                if (!(Slots.PendingBuffers & Bit))
                {
                    TMetalStageEncoder<Stage>::SetBufferOffset(Encoder, Offset, Slot);
                }
            }

            return;
        }

        Slots.Buffers[Slot]  = Buffer;
        Slots.Offsets[Slot]  = Offset;
        Slots.PendingBuffers |= Bit;
    }

    template<EShaderVisibility::Type Stage>
    FORCEINLINE void SetBytes(typename TMetalStageEncoder<Stage>::EncoderType Encoder, const void* Bytes, NSUInteger Size, uint8 Slot)
    {
        CHECK(Slot < MaxBuffers);

        FStageSlots& Slots = Stages[Stage];
        Slots.Buffers[Slot]  = nil;
        Slots.PendingBuffers &= ~(1ull << Slot);
        TMetalStageEncoder<Stage>::SetBytes(Encoder, Bytes, Size, Slot);
    }

    template<EShaderVisibility::Type Stage>
    FORCEINLINE void SetTexture(id<MTLTexture> Texture, uint8 Slot)
    {
        CHECK(Slot < MaxTextures);

        FStageSlots& Slots = Stages[Stage];

        if (Slots.Textures[Slot] != Texture)
        {
            Slots.Textures[Slot] = Texture;
            Slots.PendingTextures[Slot / 64] |= 1ull << (Slot % 64);
        }
    }

    template<EShaderVisibility::Type Stage>
    FORCEINLINE void SetSamplerState(id<MTLSamplerState> Sampler, uint8 Slot)
    {
        CHECK(Slot < MaxSamplers);

        FStageSlots& Slots = Stages[Stage];

        if (Slots.Samplers[Slot] != Sampler)
        {
            Slots.Samplers[Slot] = Sampler;
            Slots.PendingSamplers |= 1ull << Slot;
        }
    }

    template<EShaderVisibility::Type Stage>
    FORCEINLINE void Commit(typename TMetalStageEncoder<Stage>::EncoderType Encoder)
    {
        FStageSlots& Slots = Stages[Stage];

        ForEachRun(Slots.PendingBuffers, 0, [&](NSRange Range)
        {
            TMetalStageEncoder<Stage>::SetBuffers(Encoder, Slots.Buffers + Range.location, Slots.Offsets + Range.location, Range);
        });

        for (uint32 Word = 0; Word < MaxTextures / 64; ++Word)
        {
            ForEachRun(Slots.PendingTextures[Word], Word * 64, [&](NSRange Range)
            {
                TMetalStageEncoder<Stage>::SetTextures(Encoder, Slots.Textures + Range.location, Range);
            });

            Slots.PendingTextures[Word] = 0;
        }

        ForEachRun(Slots.PendingSamplers, 0, [&](NSRange Range)
        {
            TMetalStageEncoder<Stage>::SetSamplerStates(Encoder, Slots.Samplers + Range.location, Range);
        });

        Slots.PendingBuffers  = 0;
        Slots.PendingSamplers = 0;
    }

private:
    struct FStageSlots
    {
        id<MTLBuffer>       Buffers[MaxBuffers];
        NSUInteger          Offsets[MaxBuffers];
        id<MTLTexture>      Textures[MaxTextures];
        id<MTLSamplerState> Samplers[MaxSamplers];
        uint64              PendingBuffers;
        uint64              PendingTextures[MaxTextures / 64];
        uint64              PendingSamplers;
    };

    template<typename FunctionType>
    static FORCEINLINE void ForEachRun(uint64 Mask, uint32 BaseSlot, FunctionType&& Function)
    {
        while (Mask != 0)
        {
            const uint32 First   = MetalRHI::FirstSetBit(Mask);
            const uint64 Shifted = Mask >> First;
            const uint32 Count   = (~Shifted == 0) ? (64 - First) : MetalRHI::FirstSetBit(~Shifted);

            Function(NSMakeRange(BaseSlot + First, Count));

            if (First + Count >= 64)
            {
                break;
            }

            Mask &= ~0ull << (First + Count);
        }
    }

    FStageSlots Stages[EShaderVisibility::Count];
};
