// Memory function calling utilities
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>

#include <type_traits>

#include "client/hook/LazyPointer.hpp"

namespace hook
{

template <typename Ret = void, typename... Args>
inline Ret Call(MemoryPointer p, Args... a)
{
    return reinterpret_cast<Ret(__cdecl*)(Args...)>(p.Get<void>())(std::forward<Args>(a)...);
}

template <uintptr_t addr, typename Ret = void, typename... Args>
inline Ret Call(Args... a)
{
    return Call<Ret>(LazyPtr<addr>(), std::forward<Args>(a)...);
}

template <typename Ret = void, typename... Args>
inline Ret StdCall(MemoryPointer p, Args... a)
{
    return reinterpret_cast<Ret(__stdcall*)(Args...)>(p.Get<void>())(std::forward<Args>(a)...);
}

template <uintptr_t addr, typename Ret = void, typename... Args>
inline Ret StdCall(Args... a)
{
    return StdCall<Ret>(LazyPtr<addr>(), std::forward<Args>(a)...);
}

template <typename Ret = void, typename... Args>
inline Ret ThisCall(MemoryPointer p, Args... a)
{
    return reinterpret_cast<Ret(__thiscall*)(Args...)>(p.Get<void>())(std::forward<Args>(a)...);
}

template <uintptr_t addr, typename Ret = void, typename... Args>
inline Ret ThisCall(Args... a)
{
    return ThisCall<Ret>(LazyPtr<addr>(), std::forward<Args>(a)...);
}

template <size_t index>
struct Vtbl
{
    template <typename Ret, typename... Args>
    static Ret Call(Args... a)
    {
        auto obj = MemoryPointer(std::get<0>(std::forward_as_tuple(a...)));
        auto p = MemoryPointer((*obj.template Get<void**>())[i]);
        return ThisCall<Ret>(p, std::forward<Args>(a)...);
    }
};

}  // namespace hook
