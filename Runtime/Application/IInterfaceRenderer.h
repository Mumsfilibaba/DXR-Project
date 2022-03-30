#pragma once
#include "InterfaceImage.h"
#include "IWindow.h"

#include "Core/Modules/ModuleManager.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IInterfaceRenderer

class IInterfaceRenderer : public CRefCounted
{
public:

    virtual ~IInterfaceRenderer() = default;

    /**
     * @brief: Init the context
     * 
     * @param NewContext: Context to set to the renderer
     * @return: Returns true if the initialization was successful
     */
    virtual bool InitContext(InterfaceContext NewContext) = 0;

    /** Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() = 0;

    /** End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() = 0;

    /**
     * @brief: Render all the UI for this frame 
     * 
     * @param InCommandList: CommandList to record all draw-commands to
     */
    virtual void Render(class CRHICommandList& InCommandlist) = 0;

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
    virtual IInterfaceRenderer* CreateRenderer() = 0;
};