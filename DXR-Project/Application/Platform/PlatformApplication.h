#pragma once
#ifdef WIN32
	#include "Windows/WindowsApplication.h"
	typedef WindowsApplication PlatformApplication;
#else
	#include "Application/Generic/GenericApplication.h"
	typedef GenericCursor PlatformCursor;
#endif