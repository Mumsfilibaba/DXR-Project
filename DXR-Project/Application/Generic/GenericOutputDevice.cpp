#include "GenericOutputDevice.h"

#include "Application/Platform/PlatformOutputDevice.h"

/*
* GlobalOutputDevices
*/
GenericOutputDevice* GlobalOutputDevices::Console = nullptr;

bool GlobalOutputDevices::Initialize()
{
	Console = PlatformOutputDevice::Make();
	return false;
}

bool GlobalOutputDevices::Release()
{
	SAFEDELETE(Console);
	return false;
}
