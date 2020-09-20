#pragma once
#ifdef WIN32
	#include "Windows/WindowsConsoleOutput.h"
	typedef WindowsConsoleOutput PlatformOutputDevice;
#else
	#include "Application/Generic/GenericOutputDevice.h"
	typedef GenericOutputDevice PlatformOutputDevice;
#endif