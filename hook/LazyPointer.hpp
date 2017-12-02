// Lazy pointers
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>

#include "client/hook/MemoryPointer.hpp"

namespace hook
{

template <uintptr_t addr>
struct LazyPointer
{
public:
    // Returns the final raw pointer
    static MemoryPointer Get()
    {
        return xGet().Get();
    }

    template <typename T>
    static T* Get()
    {
        return Get().Get<T>();
    }

private:
    // Returns the final pointer
    static MemoryPointer xGet()
    {
        static void* ptr = nullptr;

        if (!ptr)
        {
            ptr = MemoryPointer(addr).Get();
        }

        return MemoryPointer(ptr);
    }
};

template <uintptr_t addr>
inline MemoryPointer LazyPtr()
{
    return LazyPointer<addr>::Get();
}

}
