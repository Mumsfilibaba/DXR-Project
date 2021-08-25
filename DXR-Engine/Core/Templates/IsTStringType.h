#pragma once

/* Determine if this type is a string-type (TFixedString, TString, or TStringView)*/
template<typename StringType>
struct TIsTStringType
{
    enum
    {
        Value = false
    };
};