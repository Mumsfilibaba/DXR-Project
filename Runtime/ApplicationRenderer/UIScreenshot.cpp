#include "UIScreenshot.h"
#include "Application/Application.h"
#include "Application/IApplicationRenderer.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Elements/Window.h"
#include "Core/Containers/Array.h"
#include "Core/Math/Math.h"
#include "Core/Misc/CommandLine.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Image/PngWriter.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIFence.h"
#include "RHI/RHITexture.h"
#include "RHI/RHIBuffer.h"

constexpr int32 BYTES_PER_PIXEL = 4;

constexpr int32 DEFAULT_SETTLE_FRAMES = 30;

constexpr uint64 ROW_PITCH_ALIGNMENT   = 256ull;
constexpr uint64 ROW_OFFSET_ALIGNMENT  = 512ull;

static FAutoConsoleCommand CCmdUIScreenshot(
    "UI.Screenshot",
    "Writes a PNG of the main window's UI. Takes the path to write, defaulting to Screenshots/Current/UI.png",
    FConsoleCommandDelegate::CreateLambda([](StringView Arguments)
    {
        const String Filename = Arguments.IsEmpty() ? String("Screenshots/Current/UI.png") : String(Arguments);
        UIScreenshot::CapturePrimaryWindow(Filename);
    }));

static FAutoConsoleCommand CCmdUIScreenshotAtFrame(
    "UI.ScreenshotAtFrame",
    "Writes a PNG of the main window's UI once the given number of frames have passed, then exits. "
    "Takes the frame count followed by the path, for example '30 Screenshots/Current/Shell.png'",
    FConsoleCommandDelegate::CreateLambda([](StringView Arguments)
    {
        const String Text = String(Arguments).Trim();

        int32 Split = 0;
        while (Split < Text.Length() && Text[Split] != ' ')
        {
            ++Split;
        }

        const int32 Frames = (Split > 0) ? CString::Atoi(*Text.SubString(0, Split)) : DEFAULT_SETTLE_FRAMES;

        String Filename;
        if (Split + 1 < Text.Length())
        {
            Filename = Text.SubString(Split + 1, Text.Length() - (Split + 1)).Trim();
        }

        if (Filename.IsEmpty())
        {
            Filename = String("Screenshots/Current/Shell.png");
        }

        UIScreenshot::RequestAtFrame(Frames, Filename, true);
    }));

struct FPendingScreenshotRequest
{
    String Filename;
    int32  FramesRemaining = 0;
    bool   bIsPending      = false;
    bool   bExitAfter      = false;
};

static FPendingScreenshotRequest GPendingRequest;

static bool GHasReadCommandLine = false;

static void ReadCommandLineRequest()
{
    StringView Path;
    if (!CommandLine::FindOption("-UIScreenshot", Path) || Path.IsEmpty())
    {
        return;
    }

    int32 Frames = DEFAULT_SETTLE_FRAMES;

    StringView FrameText;
    if (CommandLine::FindOption("-UIScreenshotFrame", FrameText) && !FrameText.IsEmpty())
    {
        Frames = CString::Atoi(*String(FrameText));
    }

    UIScreenshot::RequestAtFrame(Frames, String(Path), true);
}

static bool WriteTextureToPng(const FRHITextureRef& Texture, const String& Filename)
{
    const FRHITextureDesc& Desc   = Texture->GetDesc();
    const uint32           Width  = Desc.Extent.X;
    const uint32           Height = Desc.Extent.Y;

    if (Width == 0 || Height == 0)
    {
        LOG_ERROR("[UIScreenshot]: Cannot write '%s' from a %u x %u texture", *Filename, Width, Height);
        return false;
    }

    const uint64 RowStride    = Math::AlignUp<uint64>(
        Math::AlignUp<uint64>(uint64(Width) * BYTES_PER_PIXEL, ROW_PITCH_ALIGNMENT), ROW_OFFSET_ALIGNMENT);
    const uint64 ReadbackSize = RowStride * uint64(Height);

    FRHIFenceRef  Fence          = RHI::CreateFence();
    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(
        FRHIBufferDesc::CreateReadbackBuffer(ReadbackSize, BYTES_PER_PIXEL),
        ERHIResourceState::CopyDest,
        nullptr);

    if (!(Fence && ReadbackBuffer))
    {
        LOG_ERROR("[UIScreenshot]: Failed to create the %llu byte readback for '%s'", ReadbackSize, *Filename);
        return false;
    }

    ReadbackBuffer->SetDebugName("UIScreenshot Readback");
    Fence->SetDebugName("UIScreenshot Fence");

    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
        Texture.Get(), ERHIResourceState::PixelShaderResource, ERHIResourceState::CopySource));

    for (uint32 Row = 0; Row < Height; ++Row)
    {
        CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), RowStride * uint64(Row),
            Texture.Get(), FTextureRegion2D(Width, 1, 0, Row), 0);
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(
        Texture.Get(), ERHIResourceState::CopySource, ERHIResourceState::PixelShaderResource));

    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    if (!Fence->Wait())
    {
        LOG_ERROR("[UIScreenshot]: Timed out waiting for the readback of '%s'", *Filename);
        return false;
    }

    void* Mapped = ReadbackBuffer->Map(0, ReadbackSize);
    if (!Mapped)
    {
        LOG_ERROR("[UIScreenshot]: Failed to map the readback for '%s'", *Filename);
        return false;
    }

    const FImageView Image(reinterpret_cast<const uint8*>(Mapped),
        static_cast<int32>(Width), static_cast<int32>(Height),
        EImageFormat::R8G8B8A8, static_cast<int32>(RowStride));

    FPngWriter Writer;
    const bool bResult = Writer.WriteToFile(Filename, Image);

    ReadbackBuffer->Unmap(0, ReadbackSize);
    return bResult;
}

bool UIScreenshot::CaptureElement(const TSharedPtr<FVisualElement>& Element, const IntVector2& Size, float DPIScale, const String& Filename)
{
    if (!Element || Size.X <= 0 || Size.Y <= 0)
    {
        LOG_ERROR("[UIScreenshot]: Cannot capture '%s' from a %d x %d element", *Filename, Size.X, Size.Y);
        return false;
    }

    if (!FApplication::IsInitialized())
    {
        return false;
    }

    TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer();
    if (!Renderer)
    {
        LOG_ERROR("[UIScreenshot]: Cannot capture '%s' with no renderer registered", *Filename);
        return false;
    }

    FRHITextureRef Texture = Renderer->RenderElementToTexture(Element, Size, DPIScale);
    if (!Texture)
    {
        LOG_ERROR("[UIScreenshot]: Failed to render '%s' into a texture", *Filename);
        return false;
    }

    const bool bResult = WriteTextureToPng(Texture, Filename);

    Renderer->RetireTexture(Texture);

    if (bResult)
    {
        LOG_INFO("[UIScreenshot]: Wrote %d x %d to '%s'", Size.X, Size.Y, *Filename);
    }

    return bResult;
}

bool UIScreenshot::CaptureWindow(const TSharedPtr<FWindow>& Window, const String& Filename)
{
    if (!Window)
    {
        return false;
    }

    return CaptureElement(Window->GetContent(), Window->GetSize(), Window->GetWindowDPIScale(), Filename);
}

bool UIScreenshot::CapturePrimaryWindow(const String& Filename)
{
    if (!FApplication::IsInitialized())
    {
        return false;
    }

    const TArray<TSharedPtr<FWindow>>& Windows = FApplication::Get().GetWindows();
    if (Windows.IsEmpty())
    {
        LOG_ERROR("[UIScreenshot]: Cannot capture '%s' with no windows open", *Filename);
        return false;
    }

    return CaptureWindow(Windows[0], Filename);
}

void UIScreenshot::RequestAtFrame(int32 FramesFromNow, const String& Filename, bool bExitAfter)
{
    GPendingRequest.Filename        = Filename;
    GPendingRequest.FramesRemaining = Math::Max(0, FramesFromNow);
    GPendingRequest.bIsPending      = true;
    GPendingRequest.bExitAfter      = bExitAfter;

    LOG_INFO("[UIScreenshot]: '%s' is queued for %d frame(s) from now", *Filename, GPendingRequest.FramesRemaining);
}

void UIScreenshot::Tick()
{
    if (!GHasReadCommandLine)
    {
        GHasReadCommandLine = true;
        ReadCommandLineRequest();
    }

    if (!GPendingRequest.bIsPending)
    {
        return;
    }

    if (GPendingRequest.FramesRemaining > 0)
    {
        --GPendingRequest.FramesRemaining;
        return;
    }

    const String Filename   = GPendingRequest.Filename;
    const bool   bExitAfter = GPendingRequest.bExitAfter;

    GPendingRequest.bIsPending = false;
    GPendingRequest.Filename.Clear();

    CapturePrimaryWindow(Filename);

    if (bExitAfter)
    {
        FConsoleManager::Get().ExecuteCommand(*FOutputDeviceLogger::Get(), String("Engine.Exit"));
    }
}
