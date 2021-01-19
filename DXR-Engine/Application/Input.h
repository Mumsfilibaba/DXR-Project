#pragma once
#include "InputCodes.h"

/*
* Input
*/

class Input
{
public:
	static EKey		ConvertFromScanCode(UInt32 ScanCode);
	static UInt32	ConvertToScanCode(EKey Key);

	static void RegisterKeyUp(EKey Key);
	static void RegisterKeyDown(EKey Key);

	static bool IsKeyUp(EKey KeyCode);
	static bool IsKeyDown(EKey KeyCode);
};