#pragma once
#include "Application/Core/Events.h"
#include "Core/Containers/SharedPtr.h"

class FWidget;

// Declares a new Widget
#define NewWidget(WidgetType, ...) TWidgetFactory<WidgetType>() <<= WidgetType::FInitializer()

// Used to declare common types used for widgets
#define DECLARE_WIDGET(ThisType, SuperType) \
    using This  = ThisType;                 \
    using Super = SuperType

// Starts the declaration of a widget's FInitializer
#define FINITIALIZER_START(WidgetType)                              \
    struct FInitializer : public FWidgetInitializerBase<WidgetType> \
    {                                                               \
        using InitializerType = WidgetType::FInitializer;

// Adds a attribute and a setter function to an FInitializer
// This is how variables are added to a Widget when initializing them
#define FINITIALIZER_ATTRIBUTE(Type, Name)           \
    Type Name;                                       \
    InitializerType& Set##Name(Type In##Name)        \
    {                                                \
        Name = In##Name;                             \
        return static_cast<InitializerType&>(*this); \
    }

// Add support for multiple slots. This is added to the FInitializer of the Widget
#define FINITIALIZER_SLOT_ATTRIBUTE(SlotType, Name)                           \
    TArray<typename SlotType::FSlotInitializer> Name;                         \
    InitializerType& operator+(typename SlotType::FSlotInitializer& NewSlot)  \
    {                                                                         \
        Name.Add(::Forward<typename SlotType::FSlotInitializer>(NewSlot));   \
        return static_cast<InitializerType&>(*this);                          \
    }                                                                         \
    InitializerType& operator+(typename SlotType::FSlotInitializer&& NewSlot) \
    {                                                                         \
        Name.Add(::Forward<typename SlotType::FSlotInitializer>(NewSlot));   \
        return static_cast<InitializerType&>(*this);                          \
    }

// Ends the declaration of a FInitializer
#define FINITIALIZER_END() \
    }

// Starts the declaration of a FSlotInitializer
#define FSLOTINITIALIZER_START(SlotType) \
    struct SlotType : public TSlotBase<SlotType> \
    {

// Ends the declaration of a FSlotInitializer
#define FSLOTINITIALIZER_END() \
    }

template<typename WidgetType>
class TWidgetFactory
{
public:
    TWidgetFactory()
        : Widget(nullptr)
    {
    }

    TSharedPtr<WidgetType> operator<<=(const typename WidgetType::FInitializer& InInitializer)
    {
        Widget = MakeShared<WidgetType>();
        Widget->Initialize(InInitializer);
        return Widget;
    }

private:
    TSharedPtr<WidgetType> Widget;
};