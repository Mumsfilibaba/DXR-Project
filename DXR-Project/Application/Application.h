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
	virtual void OnWindowResize(WindowsWindow* Window, Uint16 Width, Uint16 Height)	override;
	virtual void OnKeyDown(Uint32 KeyCode)											override;
	virtual void OnMouseMove(Int32 x, Int32 y)										override;

private:
	bool Initialize();

protected:
	std::shared_ptr<WindowsWindow>	Window				= nullptr;
	WindowsApplication*				PlatformApplication = nullptr;

	static std::shared_ptr<Application> ApplicationInstance;
};