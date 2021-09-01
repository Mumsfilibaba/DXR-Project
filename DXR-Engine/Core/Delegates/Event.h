#pragma once
#include "MulticastDelegate.h"

/* Macros for delcaring event types */

#define DECLARE_EVENT( NewEvent, OwnerType, ... )       \
    class NewEvent : public TEvent<__VA_ARGS__>         \
    {                                                   \
        friend class OwnerType;                         \
                                                        \
        NewEvent(const NewEvent&) = default;            \
        NewEvent(NewEvent&&) = default;                 \
                                                        \
        NewEvent& operator=(const NewEvent&) = default; \
        NewEvent& operator=(NewEvent&&) = default;      \
                                                        \
    public:                                             \
        NewEvent() = default;                           \
    };

/* Event */
template<typename... ArgTypes>
class TEvent : public TMulticastDelegate<ArgTypes...>
{
    using Super = TMulticastDelegate<ArgTypes...>;

public:
    using Super::Unbind;
    using Super::UnbindIfBound;
    using Super::IsBound;
    using Super::IsObjectBound;
    using Super::GetCount;
    using Super::AddStatic;
    using Super::AddRaw;
    using Super::AddLambda;
    using Super::Add;

protected:
    using Super::UnbindAll;
    using Super::Swap;
    using Super::Broadcast;
};