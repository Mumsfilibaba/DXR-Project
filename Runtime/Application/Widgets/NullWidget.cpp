#include "NullWidget.h"
#include "Application/Widget.h"
#include "Application/WidgetChildren.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

struct FNullWidgetImpl : public FWidget
{
    FINITIALIZER_START(FNullWidgetImpl)
    FINITIALIZER_END();

    FNullWidgetImpl()
        : FWidget()
        , Children(this)
    {
    }

    void Initialize(const FInitializer& Initializer)
    {
    }

    virtual void Paint(const FRectangle& AssignedBounds) override final
    {
    }

    virtual FWidgetChildren* GetChildren() override final
    {
        return &Children;
    }

    static TSharedPtr<FNullWidgetImpl> Construct()
    {
        static TSharedPtr<FNullWidgetImpl> NullWidget = NewWidget(FNullWidgetImpl);
        return NullWidget;
    }

    FEmptyWidgetChildren Children;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING

TSharedPtr<FWidget> FNullWidget::NullWidget = FNullWidgetImpl::Construct();
