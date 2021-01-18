#pragma once
#include "Time/Clock.h"

#include <Containers/TArrayView.h>

/*
* EngineMain
*/

Int32 EngineMain(const TArrayView<const Char*> Args);

/*
* EngineLoop
*/

class EngineLoop
{
public:
	EngineLoop() 	= default;
	~EngineLoop() 	= default;

	// Runs before start of loop
	Bool PreInit();
	Bool Init();
	Bool PostInit();
	
	// Runs every frame
	void PreTick();
	void Tick();
	void PostTick();

	// Runs at exit
	Bool PreRelease();
	Bool Release();
	Bool PostRelease();

	void Exit();
	
	Bool IsRunning() const;

	Timestamp GetDeltaTime()		const;
	Timestamp GetTotalElapsedTime() const;

private:
	Bool  ShouldRun = false;
	Clock EngineClock;
};