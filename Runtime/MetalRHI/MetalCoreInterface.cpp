#include "MetalCoreInterface.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalCoreInterface

CMetalCoreInterface::CMetalCoreInterface()
	: CRHICoreInterface(ERHIInstanceType::Metal)
	, CommandContext(CMetalCommandContext::CreateMetalContext())
{ }

CMetalCoreInterface::~CMetalCoreInterface()
{
	SafeDelete(CommandContext);
	SafeDelete(Device);
}

CMetalCoreInterface* CMetalCoreInterface::CreateMetalCoreInterface()
{
	return dbg_new CMetalCoreInterface();
}

bool CMetalCoreInterface::Initialize(bool bEnableDebug)
{
	UNREFERENCED_VARIABLE(bEnableDebug);
	
	Device = CMetalDevice::CreateMetalDevice();
	if (!Device)
	{
		LOG_ERROR("[MetalRHI]: Failed to Create Device");
		return false;
	}
	
	LOG_INFO("[MetalRHI]: Created Device");
	return true;
}
