#pragma once

/** Determine if this type is a string-type(TStaticString, TString, or TStringView) */

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TIsTStringType

template<typename T>
struct TIsTStringType
{
    enum { Value = false };
};