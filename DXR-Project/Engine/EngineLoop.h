#pragma once
#include "Time/Timestamp.h"

/*
* EngineLoop
*/

class EngineLoop
{
public:
	// Runs before start of loop
	static Bool PreInit();
	static Bool Init();
	static Bool PostInit();
	
	// Runs every frame
	static void PreTick();
	static void Tick();
	static void PostTick();

	// Runs at exit
	static void PreRelease();
	static void Release();
	static void PostRelease();

	static void Exit();
	
	static Bool IsRunning();

	static Timestamp GetDeltaTime();
	static Timestamp GetTotalElapsedTime();
};