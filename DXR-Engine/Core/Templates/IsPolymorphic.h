#pragma once

/* Determine if type has any virtual functions */
template<typename T>
struct TIsPolymorphic
{
    static constexpr bool Value = __is_polymorphic( T );
};