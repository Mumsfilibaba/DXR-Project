#pragma once
#include "EventHandler.h"

#include "Windows/WindowsWindow.h"
#include "Windows/WindowsApplication.h"

#include <memory>

class Application : public EventHandler
{
public:
	~Application();

	bool Tick();

	void SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow>& InCapture);

	std::shared_ptr<WindowsWindow> GetWindow()			const;
	std::shared_ptr<WindowsWindow> GetActiveWindow()	const;
	std::shared_ptr<WindowsWindow> GetCapture()			const;
	
	static Application* Create();
	static Application* Get();

public:
	// EventHandler Interface
	virtual void OnWindowResize(std::shared_ptr<WindowsWindow>& InWindow, Uint16 InWidth, Uint16 InHeight)		override;
	virtual void OnKeyUp(EKey InKeyCode)																		override;
	virtual void OnKeyDown(EKey InKeyCode)																		override;
	virtual void OnMouseMove(Int32 InX, Int32 InY)																override;
	virtual void OnMouseButtonReleased(EMouseButton InButton)													override;
	virtual void OnMouseButtonPressed(EMouseButton InButton)													override;

private:
	Application();

	bool Initialize();

protected:
	std::shared_ptr<WindowsWindow>	Window				= nullptr;
	WindowsApplication*				PlatformApplication = nullptr;

	static std::shared_ptr<Application> Instance;
};