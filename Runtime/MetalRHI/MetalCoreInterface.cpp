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
}

CMetalCoreInterface* CMetalCoreInterface::CreateMetalCoreInterface()
{
	return dbg_new CMetalCoreInterface();
}

bool CMetalCoreInterface::Initialize(bool bEnableDebug)
{
	UNREFERENCED_VARIABLE(bEnableDebug);
	
	return true;
}
