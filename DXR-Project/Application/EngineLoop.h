#pragma once
#include "Timestamp.h"

class EngineLoop
{
public:
	static bool CoreInitialize();
	
	static bool Initialize();

	static void Tick();

	static void Release();

	static void CoreRelease();

	static Timestamp GetDeltaTime();
	static Timestamp GetTotalElapsedTime();

	static bool IsRunning();

	static void Exit();
};