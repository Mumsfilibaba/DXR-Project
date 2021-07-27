#pragma once

/* Determine if type has any virtual functions */
template<typename T>
struct TIsPolymorphic
{
    enum { Value = __is_polymorphic( T ) };
};