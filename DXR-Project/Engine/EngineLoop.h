#pragma once
#include "Time/Timestamp.h"

/*
* EngineLoop
*/

class EngineLoop
{
public:
	// Runs before start of loop
	static bool PreInitialize();
	static bool Initialize();
	static bool PostInitialize();
	
	// Runs every frame
	static void PreTick();
	static void Tick();
	static void PostTick();

	// Runs at exit
	static void PreRelease();
	static void Release();
	static void PostRelease();

	static void Exit();
	
	static bool IsRunning();

	static Timestamp GetDeltaTime();
	static Timestamp GetTotalElapsedTime();
};