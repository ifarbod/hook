// Reusable memory pointer representation
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>

namespace hook
{

union MemoryPointer
{
public:
    // Default constructor.
    MemoryPointer() : m_ptr(nullptr) {}
    // Construct from nullptr.
    MemoryPointer(std::nullptr_t) : m_ptr(nullptr) {}
    // Copy constructor.
    MemoryPointer(const MemoryPointer& x) = default;
    // Construct from a pointer.
    MemoryPointer(void* x) : m_ptr(x) {}
    // Construct from an integral pointer.
    MemoryPointer(uintptr_t x) : m_a(x) {}
    // Construct from a pointer with a specified type.
    template <typename T>
    MemoryPointer(T* x) : m_ptr(reinterpret_cast<void*>(x))
    {
    }

    // Returns true if the underlying pointer is a nullptr.
    bool IsNull() const { return m_ptr != nullptr; }
    // Return the underlying pointer as a uint32_t.
    uintptr_t AsInt() const { return m_a; }

    explicit operator bool() const { return IsNull(); }
    explicit operator uintptr_t() const { return m_a; }

    MemoryPointer Get() const { return *this; }
    template <typename T>
    T* Get() const
    {
        return Get();
    }
    template <typename T>
    T* GetRaw()
    {
        return Get();
    }

    template <typename T>
    operator T*() const
    {
        return reinterpret_cast<T*>(m_ptr);
    }

    // Comparison
    bool operator==(const MemoryPointer& rhs) const { return m_a == rhs.m_a; }
    bool operator!=(const MemoryPointer& rhs) const { return m_a != rhs.m_a; }
    bool operator<(const MemoryPointer& rhs) const { return m_a < rhs.m_a; }
    bool operator<=(const MemoryPointer& rhs) const { return m_a <= rhs.m_a; }
    bool operator>(const MemoryPointer& rhs) const { return m_a > rhs.m_a; }
    bool operator>=(const MemoryPointer& rhs) const { return m_a >= rhs.m_a; }

    // Operators
    MemoryPointer operator+(const MemoryPointer& rhs) const { return MemoryPointer(m_a + rhs.m_a); }
    MemoryPointer operator-(const MemoryPointer& rhs) const { return MemoryPointer(m_a - rhs.m_a); }
    MemoryPointer operator*(const MemoryPointer& rhs) const { return MemoryPointer(m_a * rhs.m_a); }
    MemoryPointer operator/(const MemoryPointer& rhs) const { return MemoryPointer(m_a / rhs.m_a); }

    MemoryPointer operator+=(const MemoryPointer& rhs) const { return MemoryPointer(m_a + rhs.m_a); }
    MemoryPointer operator-=(const MemoryPointer& rhs) const { return MemoryPointer(m_a - rhs.m_a); }
    MemoryPointer operator*=(const MemoryPointer& rhs) const { return MemoryPointer(m_a * rhs.m_a); }
    MemoryPointer operator/=(const MemoryPointer& rhs) const { return MemoryPointer(m_a / rhs.m_a); }

private:
    // Pointer.
    void* m_ptr;
    // Unsigned integral pointer.
    uintptr_t m_a;
};

}  // namespace hook
