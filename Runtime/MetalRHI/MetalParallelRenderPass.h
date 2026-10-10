#pragma once
#include "Core/Containers/Array.h"
#include "Core/Templates/Utility/NonCopyable.h"
#include "MetalRHI/MetalCommandContextState.h"

class FMetalCommandContext;
struct FRHIBeginRenderPassDesc;

class METALRHI_API FMetalParallelRenderPass : public FNonCopyAndNonMovable
{
public:
    FMetalParallelRenderPass(FMetalCommandContext& InParent, const FRHIBeginRenderPassDesc& Desc);
    ~FMetalParallelRenderPass();

    FMetalCommandContext* ObtainChildContext();
    void                  ReleaseChildContext(FMetalCommandContext* Child);

private:
    FMetalCommandContext&               Parent;
    id<MTLParallelRenderCommandEncoder> Encoder;
    TArray<FMetalCommandContext*>       Children;
    FMetalRenderPassInfo                RenderPassInfo;
};
