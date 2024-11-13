#pragma once

struct FNonCopyable
{
    FNonCopyable() = default;
    ~FNonCopyable() = default;

    FNonCopyable(const FNonCopyable&) = delete;
    FNonCopyable& operator=(const FNonCopyable&) = delete;
};

struct FNonMovable
{
    FNonMovable() = default;
    ~FNonMovable() = default;

    FNonMovable(const FNonMovable&) = delete;
    FNonMovable& operator=(const FNonMovable&) = delete;
};

struct FNonCopyAndNonMovable
{
    FNonCopyAndNonMovable() = default;
    ~FNonCopyAndNonMovable() = default;

    FNonCopyAndNonMovable(const FNonCopyAndNonMovable&) = delete;
    FNonCopyAndNonMovable& operator=(const FNonCopyAndNonMovable&) = delete;

    FNonCopyAndNonMovable(FNonCopyAndNonMovable&&) = delete;
    FNonCopyAndNonMovable& operator=(FNonCopyAndNonMovable&&) = delete;
};