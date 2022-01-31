#pragma once

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNonCopyable

class CNonCopyable
{
public:

    CNonCopyable()  = default;
    ~CNonCopyable() = default;

    CNonCopyable(const CNonCopyable&)            = delete;
    CNonCopyable& operator=(const CNonCopyable&) = delete;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CNonMovable

class CNonMovable
{
public:

    CNonMovable()  = default;
    ~CNonMovable() = default;

    CNonMovable(const CNonMovable&)            = delete;
    CNonMovable& operator=(const CNonMovable&) = delete;
};