#pragma once
#include "CanvasImage.h"
#include "CanvasWindow.h"

#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ICanvasRenderer

class ICanvasRenderer : public FRefCounted
{
public:

    virtual ~ICanvasRenderer() = default;

    /**
     * @brief: Initialize the context
     * 
     * @param NewContext: Context to set to the renderer
     * @return: Returns true if the initialization was successful
     */
    virtual bool InitContext(InterfaceContext NewContext) = 0;

    /** @brief: Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() = 0;

    /** @brief: End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() = 0;

    /**
     * @brief: Render all the UI for this frame 
     * 
     * @param InCommandList: CommandList to record all draw-commands to
     */
    virtual void Render(class FRHICommandList& InCommandlist) = 0;

};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IInterfaceRendererModule

class IInterfaceRendererModule : public CDefaultEngineModule
{
public:

    /**
     * @brief: Creates a interface renderer
     * 
     * @return: Returns a newly created interface renderer
     */
    virtual ICanvasRenderer* CreateRenderer() = 0;
};
