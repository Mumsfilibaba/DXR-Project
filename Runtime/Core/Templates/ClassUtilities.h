#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// NonCopyable

class NonCopyable
{
public:

    NonCopyable()  = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// NonMovable

class NonMovable
{
public:

    NonMovable()  = default;
    ~NonMovable() = default;

    NonMovable(const NonMovable&)            = delete;
    NonMovable& operator=(const NonMovable&) = delete;
};