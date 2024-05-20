#pragma once

template <class T>
class non_copyable
{
public:
    non_copyable(const non_copyable&) = delete;
    T& operator = (const T&) = delete;

protected:
    non_copyable() = default;
    ~non_copyable() = default;
};

class error
{
public:
    static void fatal_error(const char*, ...);
};