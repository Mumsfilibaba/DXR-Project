#pragma once

/* Check weather the type is the base of another type */
template<typename BaseType, typename DerivedType>
struct TIsBaseOf
{
    enum
    {
        Value = __is_base_of( BaseType, DerivedType )
    };
};