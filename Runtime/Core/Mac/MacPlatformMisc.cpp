#include "Core/Mac/MacPlatformMisc.h"
#include "Core/Mac/MacPlatformStackTrace.h"
#include "Core/Misc/CrashReporter.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Platform/PlatformAtomic.h"

#include <mach/exc.h>
#include <mach/mach.h>
#include <pthread.h>
#include <signal.h>
#include <sys/sysctl.h>
#include <Foundation/Foundation.h>

/** Distinguishes the message the signal handler sends from the ones the kernel sends */
constexpr mach_msg_id_t SIGNAL_REPORT_MESSAGE_ID = 0x44585221;

/** How long a faulting thread waits for its report before giving up and letting the process die */
constexpr int32 REPORT_TIMEOUT_SECONDS = 10;

/** Longest thread name the report carries, which is more than pthread allows anyway */
constexpr uint32 MAX_THREAD_NAME_LENGTH = 64;

#pragma pack(push, 4)

struct FMachExceptionRequest
{
    mach_msg_header_t          Header;
    mach_msg_body_t            Body;
    mach_msg_port_descriptor_t Thread;
    mach_msg_port_descriptor_t Task;
    NDR_record_t               NDR;
    exception_type_t           Exception;
    mach_msg_type_number_t     CodeCount;
    int64                      Code[2];
    mach_msg_trailer_t         Trailer;
};

struct FMachExceptionReply
{
    mach_msg_header_t  Header;
    NDR_record_t       NDR;
    kern_return_t      ReturnCode;
    mach_msg_trailer_t Trailer;
};

struct FSignalReportMessage
{
    mach_msg_header_t  Header;
    int32              Signal;
    uint32             Thread;
    uint64             FaultAddress;
    mach_msg_trailer_t Trailer;
};

#pragma pack(pop)

union FCrashMessage
{
    mach_msg_header_t     Header;
    FMachExceptionRequest Exception;
    FMachExceptionReply   Reply;
    FSignalReportMessage  Signal;
};

// The port the kernel raises exceptions on, which also receives the signal handler's messages.
static mach_port_t GExceptionPort = MACH_PORT_NULL;

// Released once the report is on disk, so the faulting thread knows when it may die.
static semaphore_t GReportCompleteSemaphore = SEMAPHORE_NULL;

// Claimed by whichever thread crashes first, so a fault raised while reporting cannot report again.
static volatile int32 GIsReporting = 0;

// Whatever was registered before this handler, kept so the exception can be passed on to a debugger.
static exception_mask_t       GPreviousMasks[EXC_TYPES_COUNT]     = {};
static mach_port_t            GPreviousPorts[EXC_TYPES_COUNT]     = {};
static exception_behavior_t   GPreviousBehaviors[EXC_TYPES_COUNT] = {};
static thread_state_flavor_t  GPreviousFlavors[EXC_TYPES_COUNT]   = {};
static mach_msg_type_number_t GPreviousPortCount                  = 0;

static const CHAR* GetMachExceptionName(exception_type_t ExceptionType)
{
    switch (ExceptionType)
    {
        case EXC_BAD_ACCESS:
            return "EXC_BAD_ACCESS";

        case EXC_BAD_INSTRUCTION:
            return "EXC_BAD_INSTRUCTION";

        case EXC_ARITHMETIC:
            return "EXC_ARITHMETIC";

        case EXC_BREAKPOINT:
            return "EXC_BREAKPOINT";

        case EXC_CRASH:
            return "EXC_CRASH";

        case EXC_GUARD:
            return "EXC_GUARD";

        default:
            return "EXC_UNKNOWN";
    }
}

static const CHAR* GetSignalName(int32 Signal)
{
    switch (Signal)
    {
        case SIGABRT:
            return "SIGABRT";

        case SIGSEGV:
            return "SIGSEGV";

        case SIGBUS:
            return "SIGBUS";

        case SIGILL:
            return "SIGILL";

        case SIGFPE:
            return "SIGFPE";

        case SIGSYS:
            return "SIGSYS";

        default:
            return "SIGNAL";
    }
}

static void ReportCrash(const CHAR* ExceptionName, uint64 ExceptionCode, uint64 FaultAddress, thread_t Thread)
{
    if (FPlatformAtomic::InterlockedCompareExchange(&GIsReporting, 1, 0) != 0)
    {
        return;
    }

    CHAR ThreadName[MAX_THREAD_NAME_LENGTH] = {};
    if (pthread_t PosixThread = ::pthread_from_mach_thread_np(Thread))
    {
        ::pthread_getname_np(PosixThread, ThreadName, sizeof(ThreadName));
    }

    FCrashContext Context;
    Context.ExceptionName            = ExceptionName;
    Context.ExceptionCode            = ExceptionCode;
    Context.FaultAddress             = FaultAddress;
    Context.ThreadName               = ThreadName;
    Context.ThreadStack.ThreadHandle = static_cast<uint64>(Thread);

    CrashReporter::Report(Context);
}

static kern_return_t ForwardRawException(const FMachExceptionRequest& Request, mach_port_t DestinationPort)
{
    mach_port_t ReplyPort = MACH_PORT_NULL;
    if (::mach_port_allocate(::mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &ReplyPort) != KERN_SUCCESS)
    {
        return KERN_FAILURE;
    }

    FCrashMessage Message = {};
    Message.Exception = Request;

    const mach_msg_size_t SendSize = static_cast<mach_msg_size_t>(offsetof(FMachExceptionRequest, Trailer));

    Message.Exception.Header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MAKE_SEND_ONCE) | MACH_MSGH_BITS_COMPLEX;
    Message.Exception.Header.msgh_remote_port = DestinationPort;
    Message.Exception.Header.msgh_local_port  = ReplyPort;
    Message.Exception.Header.msgh_size        = SendSize;
    Message.Exception.Thread.disposition      = MACH_MSG_TYPE_COPY_SEND;
    Message.Exception.Task.disposition        = MACH_MSG_TYPE_COPY_SEND;

    const mach_msg_return_t Result = ::mach_msg(
        &Message.Header,
        MACH_SEND_MSG | MACH_RCV_MSG | MACH_SEND_TIMEOUT | MACH_RCV_TIMEOUT,
        SendSize,
        sizeof(FCrashMessage),
        ReplyPort,
        REPORT_TIMEOUT_SECONDS * 1000,
        MACH_PORT_NULL);

    const kern_return_t ForwardResult = (Result == MACH_MSG_SUCCESS) ? Message.Reply.ReturnCode : KERN_FAILURE;
    ::mach_port_mod_refs(::mach_task_self(), ReplyPort, MACH_PORT_RIGHT_RECEIVE, -1);
    return ForwardResult;
}

static kern_return_t ForwardException(const FMachExceptionRequest& Request)
{
    const exception_mask_t ExceptionMask = 1u << Request.Exception;

    for (mach_msg_type_number_t Index = 0; Index < GPreviousPortCount; ++Index)
    {
        if (((GPreviousMasks[Index] & ExceptionMask) == 0) || (GPreviousPorts[Index] == MACH_PORT_NULL))
        {
            continue;
        }

        const exception_behavior_t Behavior = GPreviousBehaviors[Index];
        if ((Behavior & ~MACH_EXCEPTION_CODES) != EXCEPTION_DEFAULT)
        {
            break;
        }

        if ((Behavior & MACH_EXCEPTION_CODES) != 0)
        {
            return ForwardRawException(Request, GPreviousPorts[Index]);
        }

        exception_data_type_t NarrowCodes[2] = {};
        for (mach_msg_type_number_t CodeIndex = 0; (CodeIndex < Request.CodeCount) && (CodeIndex < 2); ++CodeIndex)
        {
            NarrowCodes[CodeIndex] = static_cast<exception_data_type_t>(Request.Code[CodeIndex]);
        }

        return ::exception_raise(GPreviousPorts[Index], Request.Thread.name, Request.Task.name, Request.Exception, NarrowCodes, Request.CodeCount);
    }

    return KERN_FAILURE;
}

static void SendExceptionReply(const FMachExceptionRequest& Request, kern_return_t ReturnCode)
{
    FMachExceptionReply Reply = {};
    Reply.Header.msgh_bits        = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(Request.Header.msgh_bits), 0);
    Reply.Header.msgh_remote_port = Request.Header.msgh_remote_port;
    Reply.Header.msgh_local_port  = MACH_PORT_NULL;
    Reply.Header.msgh_size        = static_cast<mach_msg_size_t>(offsetof(FMachExceptionReply, Trailer));
    Reply.Header.msgh_id          = Request.Header.msgh_id + 100;
    Reply.NDR                     = NDR_record;
    Reply.ReturnCode              = ReturnCode;

    ::mach_msg(&Reply.Header, MACH_SEND_MSG, Reply.Header.msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
}

static void* CrashHandlerThreadEntry(void*)
{
    ::pthread_setname_np("CrashHandler");

    for (;;)
    {
        FCrashMessage Message = {};

        const mach_msg_return_t Result = ::mach_msg(&Message.Header, MACH_RCV_MSG, 0, sizeof(FCrashMessage), GExceptionPort, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
        if (Result != MACH_MSG_SUCCESS)
        {
            continue;
        }

        if (Message.Header.msgh_id == SIGNAL_REPORT_MESSAGE_ID)
        {
            const thread_t SignalledThread = static_cast<thread_t>(Message.Signal.Thread);
            ::thread_suspend(SignalledThread);
            ReportCrash(GetSignalName(Message.Signal.Signal), static_cast<uint64>(Message.Signal.Signal), Message.Signal.FaultAddress, SignalledThread);
            ::thread_resume(SignalledThread);

            ::semaphore_signal(GReportCompleteSemaphore);
            continue;
        }

        const FMachExceptionRequest& Request = Message.Exception;
        const uint64 FaultAddress = (Request.CodeCount > 1) ? static_cast<uint64>(Request.Code[1]) : 0;

        ReportCrash(GetMachExceptionName(Request.Exception), static_cast<uint64>(Request.Code[0]), FaultAddress, Request.Thread.name);
        SendExceptionReply(Request, ForwardException(Request));
    }

    return nullptr;
}

static void CrashSignalHandler(int32 Signal, siginfo_t* SignalInfo, void*)
{
    if (GIsReporting == 0)
    {
        FSignalReportMessage Message = {};
        Message.Header.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
        Message.Header.msgh_size        = static_cast<mach_msg_size_t>(offsetof(FSignalReportMessage, Trailer));
        Message.Header.msgh_remote_port = GExceptionPort;
        Message.Header.msgh_local_port  = MACH_PORT_NULL;
        Message.Header.msgh_id          = SIGNAL_REPORT_MESSAGE_ID;
        Message.Signal                  = Signal;
        Message.Thread                  = static_cast<uint32>(::mach_thread_self());
        Message.FaultAddress            = SignalInfo ? reinterpret_cast<uint64>(SignalInfo->si_addr) : 0;

        const mach_msg_return_t Result = ::mach_msg(&Message.Header, MACH_SEND_MSG | MACH_SEND_TIMEOUT, 
            Message.Header.msgh_size, 0, MACH_PORT_NULL, REPORT_TIMEOUT_SECONDS * 1000, MACH_PORT_NULL);

        if (Result == MACH_MSG_SUCCESS)
        {
            mach_timespec_t Timeout = { REPORT_TIMEOUT_SECONDS, 0 };
            ::semaphore_timedwait(GReportCompleteSemaphore, Timeout);
        }
    }

    struct sigaction DefaultAction = {};
    DefaultAction.sa_handler = SIG_DFL;
    sigemptyset(&DefaultAction.sa_mask);
    ::sigaction(Signal, &DefaultAction, nullptr);
    ::raise(Signal);
}

static void HandleUncaughtObjectiveCException(NSException* Exception)
{
    const CHAR* Name   = Exception.name   ? [Exception.name UTF8String]   : "Unknown";
    const CHAR* Reason = Exception.reason ? [Exception.reason UTF8String] : "None";

    LOG_ERROR("Uncaught Objective-C exception: %s - %s", Name, Reason);
    FOutputDeviceManager::Get()->Flush();
}

bool FMacPlatformMisc::IsDebuggerPresent()
{
    // See: https://developer.apple.com/library/archive/qa/qa1361/_index.html for original implementation
    int32 Mib[4]
    {
        CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()
    };

    struct kinfo_proc Info;
    Info.kp_proc.p_flag = 0;

    size_t Size = sizeof(Info);
    const int32 Junk = sysctl(Mib, sizeof(Mib) / sizeof(*Mib), &Info, &Size, nullptr, 0);
    CHECK(Junk == 0);

    return (Info.kp_proc.p_flag & P_TRACED) != 0;
}

EAssertDialogResult FMacPlatformMisc::ShowAssertDialog(const CHAR* Title, const CHAR* Message)
{
    CFStringRef TitleRef   = CFStringCreateWithCString(nullptr, Title,   kCFStringEncodingUTF8);
    CFStringRef MessageRef = CFStringCreateWithCString(nullptr, Message, kCFStringEncodingUTF8);

    CFOptionFlags Response = 0;
    const SInt32 Error = CFUserNotificationDisplayAlert(0.0, kCFUserNotificationStopAlertLevel, nullptr, nullptr, nullptr, 
        TitleRef, MessageRef, CFSTR("Abort"), CFSTR("Debug"), CFSTR("Ignore"), &Response);
    if (TitleRef)
    {
        CFRelease(TitleRef);
    }

    if (MessageRef)
    {
        CFRelease(MessageRef);
    }

    if (Error != 0)
    {
        return EAssertDialogResult::Abort;
    }

    switch (Response & 0x3)
    {
        case kCFUserNotificationDefaultResponse:
            return EAssertDialogResult::Abort;

        case kCFUserNotificationAlternateResponse:
            return EAssertDialogResult::Debug;

        case kCFUserNotificationOtherResponse:
            return EAssertDialogResult::Ignore;

        default:
            return EAssertDialogResult::Abort;
    }
}

void FMacPlatformMisc::InstallCrashHandler()
{
    static bool bIsInstalled = false;
    if (bIsInstalled)
    {
        return;
    }

    bIsInstalled = true;

    FMacPlatformStackTrace::InitializeSymbols();

    const mach_port_t Task = ::mach_task_self(); 
    if (::mach_port_allocate(Task, MACH_PORT_RIGHT_RECEIVE, &GExceptionPort) != KERN_SUCCESS)
    {
        LOG_ERROR("Failed to allocate the crash handler's exception port");
        return;
    }

    if (::mach_port_insert_right(Task, GExceptionPort, GExceptionPort, MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS)
    {
        LOG_ERROR("Failed to create a send right for the crash handler's exception port");
        return;
    }

    if (::semaphore_create(Task, &GReportCompleteSemaphore, SYNC_POLICY_FIFO, 0) != KERN_SUCCESS)
    {
        LOG_ERROR("Failed to create the crash handler's completion semaphore");
        return;
    }

    pthread_attr_t ThreadAttributes;
    ::pthread_attr_init(&ThreadAttributes);
    ::pthread_attr_setdetachstate(&ThreadAttributes, PTHREAD_CREATE_DETACHED);

    pthread_t HandlerThread;
    const int32 ThreadResult = ::pthread_create(&HandlerThread, &ThreadAttributes, CrashHandlerThreadEntry, nullptr);
    ::pthread_attr_destroy(&ThreadAttributes);

    if (ThreadResult != 0)
    {
        LOG_ERROR("Failed to start the crash handler thread");
        return;
    }

    GPreviousPortCount = EXC_TYPES_COUNT;

    const exception_mask_t ExceptionMask = 
        EXC_MASK_BAD_ACCESS | 
        EXC_MASK_BAD_INSTRUCTION | 
        EXC_MASK_ARITHMETIC;

    const kern_return_t SwapResult = ::task_swap_exception_ports(
        Task,
        ExceptionMask,
        GExceptionPort,
        EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
        THREAD_STATE_NONE,
        GPreviousMasks,
        &GPreviousPortCount,
        GPreviousPorts,
        GPreviousBehaviors,
        GPreviousFlavors);

    if (SwapResult != KERN_SUCCESS)
    {
        LOG_ERROR("Failed to register the crash handler's exception port");
        GPreviousPortCount = 0;
    }

    struct sigaction Action = {};
    Action.sa_sigaction = CrashSignalHandler;
    Action.sa_flags     = SA_SIGINFO | SA_RESTART;

    sigemptyset(&Action.sa_mask);

    const int32 HandledSignals[] = 
    { 
        SIGABRT, 
        SIGSEGV, 
        SIGBUS, 
        SIGILL, 
        SIGFPE, 
        SIGSYS
    };

    for (int32 Signal : HandledSignals)
    {
        ::sigaction(Signal, &Action, nullptr);
    }

    NSSetUncaughtExceptionHandler(&HandleUncaughtObjectiveCException);
}

void FMacPlatformMisc::PrepareMetalDebugLayerEnvironment(bool bEnableDebugLayer)
{
    String Existing;
    if (bEnableDebugLayer)
    {
        if (!GetEnvironmentVariable("MTL_DEBUG_LAYER", Existing) || Existing.IsEmpty())
        {
            SetEnvironmentVariable("MTL_DEBUG_LAYER", "1");
        }
    }

    if (!GetEnvironmentVariable("MTL_DEBUG_LAYER_ERROR_MODE", Existing) || Existing.IsEmpty())
    {
        SetEnvironmentVariable("MTL_DEBUG_LAYER_ERROR_MODE", "nslog");
    }

    if (!GetEnvironmentVariable("MTL_DEBUG_LAYER_WARNING_MODE", Existing) || Existing.IsEmpty())
    {
        SetEnvironmentVariable("MTL_DEBUG_LAYER_WARNING_MODE", "nslog");
    }
}
