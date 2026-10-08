#include "MetalRHI/MetalParallelRenderPass.h"
#include "MetalRHI/MetalCommandContext.h"
#include "MetalRHI/MetalEncoderManager.h"
#include "MetalRHI/MetalQueue.h"

FMetalParallelRenderPass::FMetalParallelRenderPass(FMetalCommandContext& InParent, const FRHIBeginRenderPassDesc& Desc)
    : Parent(InParent)
    , Encoder(nil)
    , Children()
    , RenderPassInfo()
{
    SCOPED_AUTORELEASE_POOL();

    MTLRenderPassDescriptor* Descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    Parent.FillRenderPassDescriptor(Descriptor, Desc);

    Encoder        = [Parent.GetEncoders().BeginParallelRenderEncoder(Descriptor, "ParallelRender") retain];
    RenderPassInfo = FMetalCommandContext::GetRenderPassInfo(Descriptor, Desc.NumRenderTargets);
}

FMetalParallelRenderPass::~FMetalParallelRenderPass()
{
    CHECK(Children.IsEmpty());

    if (Encoder)
    {
        Parent.GetEncoders().EndParallelRenderEncoder(Encoder);
        [Encoder release];
    }
}

FMetalCommandContext* FMetalParallelRenderPass::ObtainChildContext()
{
    CHECK(Encoder != nil);

    FMetalCommandContext* Child = Parent.GetQueue().ObtainCommandContext();
    CHECK(Child != nullptr);

    Child->BeginParallelChild(Parent.GetEncoders().GetCommands(), [Encoder renderCommandEncoder]);
    Child->GetContextState().SetRenderPassInfo(RenderPassInfo);
    Children.Add(Child);
    return Child;
}

void FMetalParallelRenderPass::ReleaseChildContext(FMetalCommandContext* Child)
{
    CHECK(Child != nullptr);

    Child->EndParallelChild();
    Children.Remove(Child);
    Parent.GetQueue().ReleaseCommandContext(Child);
}
