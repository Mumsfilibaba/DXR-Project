#pragma once
#include "EventHandler.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsApplication.h"

#include <memory>

class Application : public EventHandler
{
public:
	Application();
	~Application();

	bool Tick();

	std::shared_ptr<WindowsWindow> GetWindow();
	
	static Application* Create();
	static Application* Get();

public:
	virtual void OnWindowResize(std::shared_ptr<WindowsWindow>& Window, Uint16 Width, Uint16 Height)	override;
	virtual void OnKeyUp(EKey KeyCode)												override;
	virtual void OnKeyDown(EKey KeyCode)											override;
	virtual void OnMouseMove(Int32 X, Int32 Y)										override;

private:
	bool Initialize();

protected:
	std::shared_ptr<WindowsWindow>	Window				= nullptr;
	WindowsApplication*				PlatformApplication = nullptr;

	static std::shared_ptr<Application> ApplicationInstance;
};