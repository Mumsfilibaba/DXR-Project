#pragma once

/** Determine if this type is a string-type(TStaticString, TString, or TStringView) */

template<typename T>
struct TIsTStringType
{
    enum { Value = false };
};