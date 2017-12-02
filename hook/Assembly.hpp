// Inline assembly injection
// Author(s):       iFarbod <ifarbod@outlook.com>
//                  LINK/2012
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "client/hook/Hook.hpp"

namespace hook
{

class RegPack
{
public:
    // The ordering is very important, don't change
    // The first field is the last to be pushed and first to be poped
    enum RegName
    {
        kEdi,
        kEsi,
        kEbp,
        kEsp,
        kEbx,
        kEdx,
        kEcx,
        kEax
    };

    enum EfFlag
    {
        kCarryFlag = 0,
        kParityFlag = 2,
        kAdjustFlag = 4,
        kZeroFlag = 6,
        kSignFlag = 7,
        kDirectionFlag = 10,
        kOverflowFlag = 11
    };

    uintptr_t edi() { return m_registers[kEdi]; }

    uintptr_t esi() { return m_registers[kEsi]; }

    uintptr_t ebp() { return m_registers[kEbp]; }

    uintptr_t esp() { return m_registers[kEsp]; }

    uintptr_t ebx() { return m_registers[kEbx]; }

    uintptr_t edx() { return m_registers[kEdx]; }

    uintptr_t ecx() { return m_registers[kEcx]; }

    uintptr_t eax() { return m_registers[kEax]; }

    uintptr_t& operator[](size_t i) { return m_registers[i]; }
    const uintptr_t& operator[](size_t i) const { return m_registers[i]; }

    template <uint32_t bit>  // bit starts from 0, use EfFlag enum
    bool Flag()
    {
        return (m_eflags & (1 << bit)) != 0;
    }

    bool jnb() { return Flag<kCarryFlag>() == false; }

private:
    // PUSHFD / POPFD
    uint32_t m_eflags;

    // PUSHAD/POPAD -- must be the lastest fields (because of esp)
    uintptr_t m_registers[8];
};

// Lowest level stuff (actual assembly) goes on the following namespace
namespace hook_asm
{

// Wrapper functor, so the assembly can use some templating
template <typename T>
struct Wrapper
{
    static void Call(RegPack* regs)
    {
        T fun;
        fun(*regs);
    }
};

// Constructs a RegPack and calls the wrapper functor
template <typename W>  // where W is of type wrapper
inline void __declspec(naked) MakeRegPackAndCall()
{
    __asm
    {
        // Construct the RegPack structure on the stack
        // Pushes general purposes registers to RegPack
        pushad
            // Add 4 to RegPack::esp because of our return pointer, let it be as before this func is called
        add[esp + 12], 4
        // Pushes EFLAGS to RegPack
        pushfd

            // Call wrapper sending RegPack as parameter
        push esp
        Call W::Call
        add esp, 4

        // Destructs the RegPack from the stack
        // Fix RegPack::esp before popping it (doesn't make a difference though) (+4 because eflags)
        sub[esp + 12 + 4], 4

        // Warning: Do not use any instruction that changes EFLAGS after this (-> sub affects EF!! <-)
        popfd

        popad

        // Back to normal flow
        ret
    }
}

}  // namespace hook_asm

// Makes inline assembly (but not assembly, an actual functor of type FuncT) at address
template <typename FuncT>
void MakeInline(MemoryPointer at)
{
    using functor = hook_asm::Wrapper<FuncT>;
    if (false)
        functor::Call(nullptr);  // To instantiate the template, if not done __asm will fail
    MakeCall(at, hook_asm::MakeRegPackAndCall<functor>);
}

// Same as above, but it NOPs everything between at and end (exclusive), then performs MakeInline
template <typename FuncT>
void MakeInline(MemoryPointer at, MemoryPointer end)
{
    MakeRangedNop(at, end);
    MakeInline<FuncT>(at);
}

// Same as above, but (at,end) are template parameters.
// On this case the functor can be passed as argument since there will be one func instance for each at,end not just for
// each FuncT
template <uintptr_t at, uintptr_t end, typename FuncT>
void MakeInline(FuncT func)
{
    // Stores the func object
    // TODO: Use smart pointers
    // static std::unique_ptr<FuncT> static_func;
    // static_func.reset(new FuncT(std::move(func)));
    static FuncT static_func = func;
    static_func = func;

    // Encapsulates the call to static_func
    struct Caps
    {
        void operator()(RegPack& regs) { static_func(regs); }
    };

    // Does the actual MakeInline
    return MakeInline<Caps>(LazyPointer<at>::Get(), LazyPointer<end>::Get());
}

//  MakeInline
//  Same as above, but (end) is calculated by the length of a call instruction
template <uintptr_t at, typename FuncT>
void MakeInline(FuncT func)
{
    return MakeInline<at, at + 5, FuncT>(func);
}

}  // namespace hook
