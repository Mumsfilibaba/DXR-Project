#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsBaseOf

template<typename BaseType, typename DerivedType>
struct TIsBaseOf
{
    enum
    {
        Value = __is_base_of(BaseType, DerivedType)
    };
};