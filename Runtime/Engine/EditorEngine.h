#pragma once
#include "Engine/Engine.h"

class ENGINE_API FEditorEngine : public FEngine
{
public:
    FEditorEngine();
    virtual ~FEditorEngine();

    virtual bool Init() override;
	virtual void Release() override;

private:
	TSharedPtr<class FDockspaceWidget> DockspaceWidget;
};