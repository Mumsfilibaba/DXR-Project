#include "MetalRHI/MetalDeviceDebug.h"
#include "MetalRHI/MetalCore.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformMisc.h"
#include "Core/Platform/PlatformThread.h"
#include "Core/Templates/CString.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "Core/Threading/Atomic/AtomicInt.h"
#include "Core/Threading/Runnable.h"

static TAutoConsoleVariable<bool> CVarCaptureNextFrame(
    "MetalRHI.CaptureNextFrame",
    "Captures the next BeginFrame/EndFrame pair with MTLCaptureManager",
    false);

static AtomicInt32 GValidationErrorCount;
static bool        GCaptureActive = false;

#if METAL_ENABLE_DEBUG_LAYER

#include <cerrno>
#include <cstdio>
#include <unistd.h>

static TAutoConsoleVariable<bool> CVarCaptureValidationOutput(
    "MetalRHI.CaptureValidationOutput",
    "Redirects the Metal debug layer's stderr output into the engine log",
    true);

static TAutoConsoleVariable<bool> CVarBreakOnValidationError(
    "MetalRHI.BreakOnValidationError",
    "Enables breakpoints when the Metal debug layer reports an error",
    false);

static AtomicBool GReportingDisabled;

static bool IsEnvFlagEnabled(const CHAR* Name)
{
    String Value;
    if (!FPlatformMisc::GetEnvironmentVariable(Name, Value) || Value.IsEmpty() || Value[0] == '0')
    {
        return false;
    }

    return CString::Stricmp(*Value, "false") != 0 && CString::Stricmp(*Value, "off") != 0;
}

static bool IsMetalDebugLayerRequested()
{
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        if (CVarEnableDebugLayer->GetBool())
        {
            return true;
        }
    }

    return IsEnvFlagEnabled("MTL_DEBUG_LAYER") || IsEnvFlagEnabled("METAL_DEVICE_WRAPPER_TYPE");
}

static bool IsMetalValidationLine(const CHAR* Text)
{
    if (!Text || Text[0] == '\0')
    {
        return false;
    }

    return CString::Strstr(Text, "[MetalRHI]") == nullptr;
}

static const CHAR* SkipNSLogPrefix(const CHAR* Text)
{
    if (Text[0] < '0' || Text[0] > '9')
    {
        return Text;
    }

    const CHAR* Separator = CString::Strstr(Text, "] ");
    const CHAR* Bracket   = CString::Strchr(Text, '[');

    if (!Separator || !Bracket || Bracket > Separator)
    {
        return Text;
    }

    return Separator + 2;
}

static bool IsFatalValidationLine(const CHAR* Text)
{
    return CString::Stristr(Text, "failed assertion") != nullptr;
}

static bool IsMetalValidationError(const CHAR* Text)
{
    return IsFatalValidationLine(Text)
        || CString::Stristr(Text, "error") != nullptr
        || CString::Stristr(Text, "page fault") != nullptr
        || CString::Stristr(Text, "faulted") != nullptr;
}

static void ReportValidationLine(const CHAR* RawText)
{
    if (GReportingDisabled.Load())
    {
        return;
    }

    const CHAR* Text = SkipNSLogPrefix(RawText);
    if (IsMetalValidationError(Text))
    {
        if (IsFatalValidationLine(Text))
        {
            GReportingDisabled.Store(true);
        }

        METAL_ERROR("[Metal Validation] %s", Text);
        GValidationErrorCount.Add(1);
        if (CVarBreakOnValidationError.GetValue())
        {
            DEBUG_BREAK();
        }
    }
    else
    {
        METAL_WARNING("[Metal Validation] %s", Text);
    }
}

class FMetalStderrCapture : public FRunnable
{
public:
    FMetalStderrCapture()
        : Thread(nullptr)
        , OriginalStderr(-1)
        , PipeRead(-1)
        , bStop(false)
        , bInstalled(false)
    {
    }

    bool Install()
    {
        if (bInstalled)
        {
            return true;
        }

        fflush(stderr);
        setvbuf(stderr, nullptr, _IONBF, 0);

        OriginalStderr = dup(STDERR_FILENO);
        if (OriginalStderr < 0)
        {
            METAL_ERROR("Failed to duplicate stderr for Metal validation capture");
            return false;
        }

        int PipeFds[2] = { -1, -1 };
        if (pipe(PipeFds) != 0)
        {
            METAL_ERROR("Failed to create a pipe for Metal validation capture");

            close(OriginalStderr);
            OriginalStderr = -1;

            return false;
        }

        PipeRead = PipeFds[0];
        if (dup2(PipeFds[1], STDERR_FILENO) < 0)
        {
            METAL_ERROR("Failed to redirect stderr for Metal validation capture");

            close(PipeFds[0]);
            close(PipeFds[1]);
            close(OriginalStderr);

            PipeRead       = -1;
            OriginalStderr = -1;

            return false;
        }

        close(PipeFds[1]);

        bStop.Store(false);

        Thread = FPlatformThread::Create(this, "MetalValidationLog");
        if (!Thread || !Thread->Start())
        {
            METAL_ERROR("Failed to start the Metal validation capture thread");
            Restore();
            return false;
        }

        bInstalled = true;
        METAL_INFO("Capturing Metal validation messages from stderr");
        return true;
    }

    void Restore()
    {
        if (OriginalStderr >= 0)
        {
            dup2(OriginalStderr, STDERR_FILENO);
        }

        bStop.Store(true);

        if (Thread)
        {
            Thread->WaitForCompletion();
            delete Thread;
            Thread = nullptr;
        }

        if (PipeRead >= 0)
        {
            close(PipeRead);
            PipeRead = -1;
        }

        if (OriginalStderr >= 0)
        {
            close(OriginalStderr);
            OriginalStderr = -1;
        }

        bInstalled = false;
    }

    virtual int32 Run() override
    {
        constexpr int32 MaxValidationLineLength = 4096;

        CHAR   Chunk[512];
        String Line;

        while (!bStop.Load())
        {
            const ssize_t BytesRead = read(PipeRead, Chunk, sizeof(Chunk));
            if (BytesRead < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                break;
            }

            if (BytesRead == 0)
            {
                break;
            }

            if (OriginalStderr >= 0)
            {
                ssize_t Written = 0;
                while (Written < BytesRead)
                {
                    const ssize_t Result = write(OriginalStderr, Chunk + Written, static_cast<size_t>(BytesRead - Written));
                    if (Result < 0)
                    {
                        if (errno == EINTR)
                        {
                            continue;
                        }

                        break;
                    }

                    Written += Result;
                }
            }

            for (ssize_t Index = 0; Index < BytesRead; ++Index)
            {
                const CHAR Character = Chunk[Index];
                if (Character == '\n' || Character == '\r')
                {
                    if (!Line.IsEmpty())
                    {
                        if (IsMetalValidationLine(*Line))
                        {
                            ReportValidationLine(*Line);
                        }

                        Line.Clear();
                    }
                }
                else if (Line.Size() < MaxValidationLineLength)
                {
                    Line += Character;
                }
            }
        }

        if (!Line.IsEmpty() && IsMetalValidationLine(*Line))
        {
            ReportValidationLine(*Line);
        }

        return 0;
    }

private:
    IPlatformThread* Thread;
    int              OriginalStderr;
    int              PipeRead;
    AtomicBool       bStop;
    bool             bInstalled;
};

static FMetalStderrCapture GStderrCapture;

void MetalEnableDebugLayer()
{
    if (!IsMetalDebugLayerRequested())
    {
        return;
    }

    if (!IsEnvFlagEnabled("MTL_DEBUG_LAYER"))
    {
        FPlatformMisc::SetEnvironmentVariable("MTL_DEBUG_LAYER", "1");
    }

    METAL_INFO("Metal debug layer enabled");
}

void MetalStartValidationCapture()
{
    if (!IsMetalDebugLayerRequested() || !CVarCaptureValidationOutput.GetValue())
    {
        return;
    }

    GStderrCapture.Install();
}

void MetalStopValidationCapture()
{
    GStderrCapture.Restore();
}

#endif

void MetalResetValidationErrors()
{
    GValidationErrorCount.Store(0);
}

bool MetalHasValidationErrors()
{
    return GValidationErrorCount.Load() != 0;
}

void MetalBeginFrameCapture(id<MTLDevice> Device)
{
    if (!Device || GCaptureActive || !CVarCaptureNextFrame.GetValue())
    {
        return;
    }

    CVarCaptureNextFrame.SetVariable(false);

    MTLCaptureManager* Manager = [MTLCaptureManager sharedCaptureManager];
    if (!Manager)
    {
        return;
    }

    MTLCaptureDescriptor* Descriptor = [[MTLCaptureDescriptor new] autorelease];
    Descriptor.captureObject = Device;
    if ([Manager supportsDestination:MTLCaptureDestinationDeveloperTools])
    {
        Descriptor.destination = MTLCaptureDestinationDeveloperTools;
    }
    else if ([Manager supportsDestination:MTLCaptureDestinationGPUTraceDocument])
    {
        Descriptor.destination = MTLCaptureDestinationGPUTraceDocument;
        Descriptor.outputURL   = [NSURL fileURLWithPath:@"/tmp/DXR-MetalCapture.gputrace"];
    }
    else
    {
        METAL_WARNING("MTLCaptureManager has no supported capture destination");
        return;
    }

    NSError* Error = nil;
    if (![Manager startCaptureWithDescriptor:Descriptor error:&Error])
    {
        const String ErrorString(Error ? [Error localizedDescription] : @"unknown error");
        METAL_ERROR("Failed to start a Metal GPU capture: %s", *ErrorString);
        return;
    }

    GCaptureActive = true;
    METAL_INFO("Metal GPU capture started");
}

void MetalEndFrameCapture()
{
    if (!GCaptureActive)
    {
        return;
    }

    [[MTLCaptureManager sharedCaptureManager] stopCapture];
    GCaptureActive = false;
    METAL_INFO("Metal GPU capture stopped");
}
