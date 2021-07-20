#pragma once

/* Determines if the type is Plain Old Data (POD) */
template<typename T>
struct TIsPlainOldData
{
    static constexpr bool Value = __is_pod( T );
};