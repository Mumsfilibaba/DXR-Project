#include "ApplicationInterface.h"
#include "StandardApplication.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

TSharedPtr<FApplicationInterface> FApplicationInterface::GInstance;

bool FApplicationInterface::Create()
{
    TSharedPtr<FGenericApplication> Application = MakeSharedPtr(FPlatformApplicationMisc::CreateApplication());
    if (!Application)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    FStandardApplication* NewApplication = dbg_new FStandardApplication(Application);
    GInstance = MakeSharedPtr(NewApplication);
    if (!NewApplication->CreateContext())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create UI Context");
        return false;
    }

    Application->SetMessageListener(GInstance);
    return true;
}

void FApplicationInterface::Release()
{
    if (GInstance)
    {
        GInstance->SetPlatformApplication(nullptr);
        GInstance.Reset();
    }
}