#include "Core/Windows/Windows.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/String.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "D3D12RHI/D3D12DeviceDebug.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Core.h"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

#if D3D12_ENABLE_GPU_VALIDATION
static TAutoConsoleVariable<bool> CVarEnableGPUValidation(
    "D3D12RHI.EnableGPUValidation",
    "Enables GPU Based Validation if true",
    false);
#endif

#if D3D12_ENABLE_CRASH_MARKERS
static TAutoConsoleVariable<bool> CVarEnableDRED(
    "D3D12RHI.EnableDRED",
    "Enables Device Removed Extended Data (DRED) if the Device gets removed",
    false);
#endif

static TAutoConsoleVariable<String> CVarDeviceRemovedDumpFilePath(
    "D3D12RHI.DeviceRemovedDumpFilePath",
    "File path for DRED device removed dump output",
    "D3D12DeviceRemovedDump.txt");

static FAutoConsoleCommand CCmdForceDeviceRemoved(
    "D3D12RHI.ForceDeviceRemoved",
    "Forces a device removal via ID3D12Device5::RemoveDevice() to verify the detection -> dump pipeline",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
    #if D3D12_USE_ID3D12DEVICE_5
        FD3D12Device* Device = FD3D12DeviceRHI::Get()->GetDevice();
        if (Device && Device->GetD3D12Device5())
        {
            D3D12_WARNING("[D3D12] Forcing device removal via RemoveDevice()");
            Device->GetD3D12Device5()->RemoveDevice();
        }
        else
        {
            D3D12_ERROR("[D3D12] Cannot force device removal - device unavailable");
        }
    #else
        D3D12_ERROR("[D3D12] ID3D12Device5 unavailable; cannot force device removal");
    #endif
    }));

// -------------------------------------------------------------------------------------------
// DRED enum to string helpers
// -------------------------------------------------------------------------------------------

static const CHAR* ToString(D3D12_AUTO_BREADCRUMB_OP BreadCrumbOp)
{
    switch (BreadCrumbOp)
    {
    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:                                        return "D3D12_AUTO_BREADCRUMB_OP_SETMARKER";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:                                       return "D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:                                         return "D3D12_AUTO_BREADCRUMB_OP_ENDEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:                                    return "D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:                             return "D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:                                  return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:                                         return "D3D12_AUTO_BREADCRUMB_OP_DISPATCH";
    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:                                 return "D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:                                return "D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:                                     return "D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:                                        return "D3D12_AUTO_BREADCRUMB_OP_COPYTILES";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:                               return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:                            return "D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:                         return "D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:                            return "D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:                                  return "D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:                                    return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE";
    case D3D12_AUTO_BREADCRUMB_OP_PRESENT:                                          return "D3D12_AUTO_BREADCRUMB_OP_PRESENT";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:                                 return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:                                  return "D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:                                    return "D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:                                      return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:                                    return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:                             return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:                           return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:                         return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION";
    case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:                             return "D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:                                     return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1";
    case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:                      return "D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:                                     return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:                                   return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1";
    case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:             return "D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO: return "D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:              return "D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:                                     return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:                            return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:                               return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:                                   return "D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:                          return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP";
    case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:                                return "D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:                       return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:                          return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:                                     return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH";
    default:                                                                        return "UNKNOWN";
    }
}

static const CHAR* ToString(D3D12_DRED_ALLOCATION_TYPE AllocationType)
{
    switch (AllocationType)
    {
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_QUEUE:               return "COMMAND_QUEUE";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_ALLOCATOR:           return "COMMAND_ALLOCATOR";
    case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_STATE:              return "PIPELINE_STATE";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_LIST:                return "COMMAND_LIST";
    case D3D12_DRED_ALLOCATION_TYPE_FENCE:                       return "FENCE";
    case D3D12_DRED_ALLOCATION_TYPE_DESCRIPTOR_HEAP:             return "DESCRIPTOR_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_HEAP:                        return "HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_QUERY_HEAP:                  return "QUERY_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_SIGNATURE:           return "COMMAND_SIGNATURE";
    case D3D12_DRED_ALLOCATION_TYPE_PIPELINE_LIBRARY:            return "PIPELINE_LIBRARY";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER:               return "VIDEO_DECODER";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_PROCESSOR:             return "VIDEO_PROCESSOR";
    case D3D12_DRED_ALLOCATION_TYPE_RESOURCE:                    return "RESOURCE";
    case D3D12_DRED_ALLOCATION_TYPE_PASS:                        return "PASS";
    case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSION:               return "CRYPTOSESSION";
    case D3D12_DRED_ALLOCATION_TYPE_CRYPTOSESSIONPOLICY:         return "CRYPTOSESSIONPOLICY";
    case D3D12_DRED_ALLOCATION_TYPE_PROTECTEDRESOURCESESSION:    return "PROTECTEDRESOURCESESSION";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_DECODER_HEAP:          return "VIDEO_DECODER_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_POOL:                return "COMMAND_POOL";
    case D3D12_DRED_ALLOCATION_TYPE_COMMAND_RECORDER:            return "COMMAND_RECORDER";
    case D3D12_DRED_ALLOCATION_TYPE_STATE_OBJECT:                return "STATE_OBJECT";
    case D3D12_DRED_ALLOCATION_TYPE_METACOMMAND:                 return "METACOMMAND";
    case D3D12_DRED_ALLOCATION_TYPE_SCHEDULINGGROUP:             return "SCHEDULINGGROUP";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_ESTIMATOR:      return "VIDEO_MOTION_ESTIMATOR";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_MOTION_VECTOR_HEAP:    return "VIDEO_MOTION_VECTOR_HEAP";
    case D3D12_DRED_ALLOCATION_TYPE_VIDEO_EXTENSION_COMMAND:     return "VIDEO_EXTENSION_COMMAND";
    default:                                                     return "UNKNOWN";
    }
}

static const CHAR* GetDeviceRemovedDumpFilePath()
{
    return *CVarDeviceRemovedDumpFilePath.GetValue();
}

// -------------------------------------------------------------------------------------------
// Device Removed Handling
// -------------------------------------------------------------------------------------------

static VOID CALLBACK OnDeviceRemovedEvent(PVOID Context, BOOLEAN /*bTimerOrWaitFired*/)
{
    D3D12RHIDeviceRemovedHandler(reinterpret_cast<FD3D12Device*>(Context), "RemovedEvent");
}

void D3D12RHIDeviceRemovedHandler(FD3D12Device* Device, const char* Source)
{
    CHECK(Device != nullptr);

    ID3D12Device* D3DDevice = Device->GetD3D12Device();

    const HRESULT Reason = D3DDevice->GetDeviceRemovedReason();
    if (Reason == S_OK)
    {
        return;
    }

    static AtomicBool Handled;
    if (Handled.Exchange(true))
    {
        return;
    }

    D3D12_ERROR("[D3D12] Device Removed (Source=%s, Reason=0x%08X)", Source ? Source : "Unknown", static_cast<uint32>(Reason));

    TComPtr<ID3D12DeviceRemovedExtendedData1> DREDInterface;
    if (FAILED(D3DDevice->QueryInterface(IID_PPV_ARGS(&DREDInterface))))
    {
        D3D12_ERROR("[D3D12] DRED interface unavailable - was DRED armed before device creation?");
        return;
    }

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 DREDAutoBreadcrumbsOutput;
    if (FAILED(DREDInterface->GetAutoBreadcrumbsOutput1(&DREDAutoBreadcrumbsOutput)))
    {
        D3D12_ERROR("[D3D12] GetAutoBreadcrumbsOutput1 failed");
        return;
    }

    D3D12_DRED_PAGE_FAULT_OUTPUT1 DREDPageFaultOutput;
    if (FAILED(DREDInterface->GetPageFaultAllocationOutput1(&DREDPageFaultOutput)))
    {
        D3D12_ERROR("[D3D12] GetPageFaultAllocationOutput1 failed");
        return;
    }

    TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(GetDeviceRemovedDumpFilePath());

    const auto WriteLine = [&File](const String& Line)
    {
        D3D12_ERROR("%s", *Line);

        if (File)
        {
            String Out = Line;
            Out += '\n';

            File->Write((const uint8*)*Out, Out.Size());
        }
    };

    WriteLine(String::CreateFormatted("[D3D12] Device Removed (Source=%s, Reason=0x%08X)", Source ? Source : "Unknown", static_cast<uint32>(Reason)));

    TComPtr<ID3D12DeviceRemovedExtendedData2> DREDInterface2;

    const bool bHasDREDInterface2 = SUCCEEDED(D3DDevice->QueryInterface(IID_PPV_ARGS(&DREDInterface2)));
    if (bHasDREDInterface2)
    {
        const D3D12_DRED_DEVICE_STATE State = DREDInterface2->GetDeviceState();
        WriteLine(String::CreateFormatted("DRED DeviceState: %d", static_cast<int32>(State)));
    }

    const D3D12_AUTO_BREADCRUMB_NODE1* CurrentNode = DREDAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
    while (CurrentNode)
    {
        const uint32 LastCompleted = CurrentNode->pLastBreadcrumbValue ? *CurrentNode->pLastBreadcrumbValue : CurrentNode->BreadcrumbCount;

        WriteLine("BreadCrumbs:");

        for (uint32 i = 0; i < CurrentNode->BreadcrumbCount; i++)
        {
            const CHAR* Marker = "";
            if (i + 1 == LastCompleted)
            {
                Marker = "  <-- LAST COMPLETED";
            }
            else if (i == LastCompleted)
            {
                Marker = "  <-- FAULTING (did not complete)";
            }

            String Line = String("    ") + ToString(CurrentNode->pCommandHistory[i]);
            for (uint32 c = 0; c < CurrentNode->BreadcrumbContextsCount; ++c)
            {
                if (CurrentNode->pBreadcrumbContexts[c].BreadcrumbIndex == i)
                {
                    const String Context = String::CreateFormatted("%ls", CurrentNode->pBreadcrumbContexts[c].pContextString);
                    Line = Line + " [" + *Context + "]";
                    break;
                }
            }

            WriteLine(Line + Marker);
        }

        CurrentNode = CurrentNode->pNext;
    }

    const auto WriteAllocationNodes = [&WriteLine](const D3D12_DRED_ALLOCATION_NODE1* Node, const CHAR* ListName)
    {
        if (!Node)
        {
            WriteLine(String("  ") + ListName + ": <none>");
            return;
        }

        WriteLine(String("  ") + ListName + ":");

        while (Node)
        {
            String Name;
            if (Node->ObjectNameA)
            {
                Name = Node->ObjectNameA;
            }
            else if (Node->ObjectNameW)
            {
                Name = String(String::CreateFormatted("%ls", Node->ObjectNameW));
            }
            else
            {
                Name = "<unnamed>";
            }

            WriteLine(String::CreateFormatted("    '%s' (%s)", *Name, ToString(Node->AllocationType)));
            Node = Node->pNext;
        }
    };

    WriteLine(String::CreateFormatted("PageFault VA: 0x%llx", static_cast<unsigned long long>(DREDPageFaultOutput.PageFaultVA)));

    if (bHasDREDInterface2)
    {
        D3D12_DRED_PAGE_FAULT_OUTPUT2 PageFault2;
        if (SUCCEEDED(DREDInterface2->GetPageFaultAllocationOutput2(&PageFault2)))
        {
            WriteLine(String::CreateFormatted("PageFault Flags: 0x%x", static_cast<uint32>(PageFault2.PageFaultFlags)));
        }
    }

    WriteAllocationNodes(DREDPageFaultOutput.pHeadExistingAllocationNode,    "ExistingAllocations");
    WriteAllocationNodes(DREDPageFaultOutput.pHeadRecentFreedAllocationNode, "RecentFreedAllocations");

    // Signal other systems that the device is removed
    CoreDelegates::DeviceRemovedDelegate.Broadcast();

    FPlatformApplicationMisc::MessageBox("Error", " [D3D12] Device Removed");
}

bool D3D12RHICheckDeviceRemoved(FD3D12Device* Device, HRESULT Result, const char* Source)
{
    if (SUCCEEDED(Result))
    {
        return false;
    }

    if (Device->GetD3D12Device()->GetDeviceRemovedReason() == S_OK)
    {
        return false;
    }

    D3D12RHIDeviceRemovedHandler(Device, Source);
    return true;
}

void FD3D12Device::RegisterDeviceRemovedEvent()
{
    TComPtr<ID3D12Fence> Fence;
    if (FAILED(GetD3D12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence))))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to create device-removed fence");
        return;
    }

    HANDLE Event = ::CreateEventA(nullptr, TRUE, FALSE, nullptr);
    if (!Event)
    {
        D3D12_WARNING("[FD3D12Device]: Failed to create device-removed event");
        return;
    }

    if (FAILED(Fence->SetEventOnCompletion(UINT64_MAX, Event)))
    {
        D3D12_WARNING("[FD3D12Device]: SetEventOnCompletion for device-removed fence failed");
        ::CloseHandle(Event);
        return;
    }

    HANDLE Wait = nullptr;
    if (!::RegisterWaitForSingleObject(&Wait, Event, &OnDeviceRemovedEvent, this, INFINITE, WT_EXECUTEONLYONCE))
    {
        D3D12_WARNING("[FD3D12Device]: RegisterWaitForSingleObject failed");
        ::CloseHandle(Event);
        return;
    }

    DeviceRemovedEvent = Event;
    DeviceRemovedWait  = Wait;
    DeviceRemovedFence = Fence;

    D3D12_INFO("[FD3D12Device]: Registered automated device-removed notification");
}

// -------------------------------------------------------------------------------------------
// D3D12 debug message callback
// -------------------------------------------------------------------------------------------

#if D3D12_USE_DEBUG_MESSAGE_CALLBACK
static void __stdcall D3D12DebugMessageCallback(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID, LPCSTR pDescription, void*)
{
    switch (Severity)
    {
        case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        case D3D12_MESSAGE_SEVERITY_ERROR:
        {
            D3D12_ERROR("[D3D12 Debug Layer] %s", pDescription);
            break;
        }

        case D3D12_MESSAGE_SEVERITY_WARNING:
        {
            D3D12_WARNING("[D3D12 Debug Layer] %s", pDescription);
            break;
        }

        case D3D12_MESSAGE_SEVERITY_INFO:
        case D3D12_MESSAGE_SEVERITY_MESSAGE:
        default:
        {
            D3D12_INFO("[D3D12 Debug Layer] %s", pDescription);
            break;
        }
    }
}
#endif

void FD3D12Device::RegisterDebugMessageCallback()
{
#if D3D12_USE_DEBUG_MESSAGE_CALLBACK
    TComPtr<ID3D12InfoQueue1> InfoQueue;
    if (SUCCEEDED(GetD3D12Device()->QueryInterface(IID_PPV_ARGS(&InfoQueue))))
    {
        DWORD Cookie = 0;
        const HRESULT CallbackResult = InfoQueue->RegisterMessageCallback(
            D3D12DebugMessageCallback,
            D3D12_MESSAGE_CALLBACK_FLAG_NONE,
            nullptr,
            &Cookie);

        if (SUCCEEDED(CallbackResult))
        {
            DebugInfoQueue             = InfoQueue;
            DebugMessageCallbackCookie = Cookie;
            D3D12_INFO("[FD3D12Device] Registered D3D12 debug message callback");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] Failed to register D3D12 debug message callback (hr=0x%08X)", CallbackResult);
        }
    }
#endif
}

void FD3D12Device::UnregisterDebugMessageCallback()
{
#if D3D12_USE_DEBUG_MESSAGE_CALLBACK
    if (DebugInfoQueue)
    {
        if (DebugMessageCallbackCookie != 0)
        {
            const HRESULT Result = DebugInfoQueue->UnregisterMessageCallback(DebugMessageCallbackCookie);
            if (FAILED(Result))
            {
                D3D12_WARNING("[FD3D12Device] Failed to unregister D3D12 debug message callback (hr=0x%08X)", Result);
            }
            else
            {
                D3D12_INFO("[FD3D12Device] Unregistered D3D12 debug message callback");
            }
        }

        DebugInfoQueue.Reset();
        DebugMessageCallbackCookie = 0;
    }
#endif
}

// -------------------------------------------------------------------------------------------
// Debug layer / GPU validation / DRED arming
// -------------------------------------------------------------------------------------------

void D3D12RHIEnableDRED()
{
#if D3D12_ENABLE_CRASH_MARKERS
    if (!CVarEnableDRED.GetValue())
    {
        return;
    }

    TComPtr<ID3D12DeviceRemovedExtendedDataSettings1> DREDSettings;
    if (SUCCEEDED(D3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DREDSettings))))
    {
        DREDSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        DREDSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        DREDSettings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

        D3D12_INFO("[FD3D12Adapter]: DRED enabled (Auto-breadcrumbs + Page-fault + Contexts)");
    }
    else
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DRED");
    }
#endif
}

void D3D12RHISetupDebugInterfaces(bool bEnableDebugLayer)
{
    if (!bEnableDebugLayer)
    {
        return;
    }

    TComPtr<ID3D12Debug> DebugInterface;
    if (FAILED(D3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DebugLayer");
        return;
    }

    DebugInterface->EnableDebugLayer();

#if D3D12_ENABLE_GPU_VALIDATION
    if (CVarEnableGPUValidation.GetValue())
    {
        TComPtr<ID3D12Debug1> DebugInterface1;
        if (SUCCEEDED(DebugInterface.GetAs(&DebugInterface1)))
        {
            DebugInterface1->SetEnableGPUBasedValidation(true);
        }
        else
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to enable GPU-Validation");
        }
    }
#endif

#if WIN10_BUILD_20348
    {
        TComPtr<ID3D12Debug5> DebugInterface5;
        if (SUCCEEDED(DebugInterface.GetAs(&DebugInterface5)))
        {
            DebugInterface5->SetEnableAutoName(true);
        }
        else
        {
            D3D12_WARNING("[FD3D12Adapter]: FAILED to enable auto-naming of objects");
        }
    }
#endif

    TComPtr<IDXGIInfoQueue> InfoQueue;
    if (SUCCEEDED(D3D12Functions::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
    {
        bool bBreakOnError = true;
        if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("D3D12RHI.BreakOnError"))
        {
            bBreakOnError = CVar->GetBool();
        }

        InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, bBreakOnError);

        bool bBreakOnWarning = false;
        if (IConsoleVariable* CVar = FConsoleManager::Get().FindConsoleVariable("D3D12RHI.BreakOnWarning"))
        {
            bBreakOnWarning = CVar->GetBool();
        }

        InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
    }
    else
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve InfoQueue");
    }
}
