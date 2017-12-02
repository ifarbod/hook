// Code injecting utils
// Author(s):       iFarbod <ifarbod@outlook.com>
//
// Copyright (c) 2013-2017 CTNorth Team
//
// Distributed under the MIT license (See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT)

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

#include <type_traits>

#include "client/hook/MemoryPointer.hpp"

namespace hook
{

// Memory protection

inline bool ProtectMemory(MemoryPointer addr, size_t size, DWORD protection)
{
    return !!VirtualProtect(addr.Get(), size, protection, &protection);
}

inline bool UnprotectMemory(MemoryPointer addr, size_t size, DWORD& out_oldprotect)
{
    return !!VirtualProtect(addr.Get(), size, PAGE_EXECUTE_READWRITE, &out_oldprotect);
}

class ScopedUnprotect
{
public:
    ScopedUnprotect(MemoryPointer addr, size_t size)
    {
        if (size == 0)
            m_unprotected = false;
        else
            m_unprotected = UnprotectMemory(m_addr = addr.Get<void>(), m_size = size, m_oldProtect);
    }

    ~ScopedUnprotect()
    {
        if (m_unprotected)
            ProtectMemory(m_addr.Get(), m_size, m_oldProtect);
    }

private:
    MemoryPointer m_addr;
    size_t m_size;
    DWORD m_oldProtect;
    bool m_unprotected;
};

// Methods for reading/writing memory

// Gets contents from a memory address
template <typename T>
inline T Read(MemoryPointer addr)
{
    return *addr.Get<T>();
}

inline void Fill(MemoryPointer addr, int32_t value, size_t size)
{
    ScopedUnprotect xprotect(addr, size);
    memset(addr.Get<void>(), value, size);
}

inline void MemCpy(MemoryPointer addr, const void* src, size_t size)
{
    ScopedUnprotect xprotect(addr, size);
    memcpy(addr.Get<void>(), src, size);
}

template <typename T>
inline void Write(MemoryPointer addr, T value)
{
    ScopedUnprotect xprotect(addr, sizeof(T));

    if (Read<T>(addr) != value)
    {
        *addr.Get<T>() = value;
    }
}

// Searches in the range [|addr|, |addr| + |maxSearch|] for a pointer in the range [|defaultBase|, |defaultEnd|] and
// replaces it with the proper offset in the pointer |replacementBase|. does memory unprotection if |vp| is true.
inline MemoryPointer AdjustPointer(MemoryPointer addr, MemoryPointer replacementBase, MemoryPointer defaultBase,
    MemoryPointer defaultEnd, size_t maxSearch = 8)
{
    ScopedUnprotect xprotect(addr, maxSearch + sizeof(void*));
    for (size_t i = 0; i < maxSearch; ++i)
    {
        MemoryPointer ptr = Read<void*>(addr + i);
        if (ptr >= defaultBase.Get() && ptr <= defaultEnd.Get())
        {
            auto result = replacementBase + (ptr - defaultBase.Get());
            Write<void*>(addr + i, result.Get());
            return result;
        }
    }
    return nullptr;
}

inline void CopyStr(MemoryPointer addr, char const* value)
{
    ScopedUnprotect xprotect(addr, 1);
    strcpy(addr, value);
}

inline void CopyStrEx(MemoryPointer addr, char const* value, size_t count)
{
    ScopedUnprotect xprotect(addr, count);
    strncpy(addr, value, count);
}

inline void ZeroMem(MemoryPointer at, size_t count = 1)
{
    Fill(at, 0, count);
}

inline void MakeNop(MemoryPointer at, size_t count = 1)
{
    Fill(at, 0x90, count);
}

inline void MakeRangedNop(MemoryPointer at, MemoryPointer until)
{
    return MakeNop(at, size_t(until.GetRaw<int8_t>() - at.GetRaw<int8_t>()));
}

// C3 RET
inline void MakeRet(MemoryPointer at)
{
    Write<uint8_t>(at, 0xC3);
}

// C2 RET (2Bytes)
inline void MakeRet(MemoryPointer at, uint16_t pop)
{
    Write<uint8_t>(at, 0xC2);
    Write<uint16_t>(at + 1, pop);
}

inline void MakeRETEx(MemoryPointer at, uint8_t ret = 1)
{
    Write<uint8_t>(at, 0xB0);  // mov al, @ret
    Write<uint8_t>(at + 1, ret);
    MakeRet(at + 2, 4);
}

// for making functions return 0
inline void MakeRet0(MemoryPointer at)
{
    Write<uint8_t>(at, 0x33);  // xor eax, eax
    Write<uint8_t>(at + 1, 0xC0);
    MakeRet(at + 2);
}

inline MemoryPointer GetAbsoluteOffset(size_t relValue, MemoryPointer endOfInstruction)
{
    return endOfInstruction.Get<int8_t>() + relValue;
}

inline uint32_t GetRelativeOffset(MemoryPointer absValue, MemoryPointer endOfInstruction)
{
    return static_cast<uint32_t>(absValue.Get<int8_t>() - endOfInstruction.Get<int8_t>());
}

inline MemoryPointer ReadRelativeOffset(MemoryPointer at, size_t sizeofAddr = 4)
{
    switch (sizeofAddr)
    {
        case 1:
            return (GetAbsoluteOffset(Read<int8_t>(at), at + sizeofAddr));
        case 2:
            return (GetAbsoluteOffset(Read<int16_t>(at), at + sizeofAddr));
        case 4:
            return (GetAbsoluteOffset(Read<int32_t>(at), at + sizeofAddr));
        default:
            return nullptr;
    }
}

inline void MakeRelativeOffset(MemoryPointer at, MemoryPointer dest, size_t sizeofAddr = 4)
{
    switch (sizeofAddr)
    {
        case 1:
            Write<int8_t>(at, static_cast<int8_t>(GetRelativeOffset(dest, at + sizeofAddr)));
            break;
        case 2:
            Write<int16_t>(at, static_cast<int16_t>(GetRelativeOffset(dest, at + sizeofAddr)));
            break;
        case 4:
            Write<int32_t>(at, static_cast<int32_t>(GetRelativeOffset(dest, at + sizeofAddr)));
            break;
    }
}

inline MemoryPointer GetBranchDestination(MemoryPointer at)
{
    switch (Read<uint8_t>(at))
    {
        // We need to handle other instructions (and prefixes) later...
        case 0xE8:  // call rel
        case 0xE9:  // jmp rel
            return ReadRelativeOffset(at + 1, 4);
        case 0xEB:
            return ReadRelativeOffset(at + 1, 1);
        case 0xFF:
        {
            switch (Read<uint8_t>(at + 1))
            {
                case 0x15:  // call dword ptr [addr]
                case 0x25:  // jmp dword ptr [addr]
                    return *(Read<uint32_t*>(at + 2));
            }
            break;
        }
    }
    return nullptr;
}

// Jump Near
inline MemoryPointer MakeJmp(MemoryPointer at, MemoryPointer dest = nullptr)
{
    auto p = GetBranchDestination(at);
    Write<uint8_t>(at, 0xE9);

    if (!!dest.AsInt())
    {
        MakeRelativeOffset(at + 1, dest, 4);
    }

    return p;
}

inline MemoryPointer MakeCall(MemoryPointer at, MemoryPointer dest = nullptr)
{
    auto p = GetBranchDestination(at);
    Write<uint8_t>(at, 0xE8);

    if (!!dest.AsInt())
    {
        MakeRelativeOffset(at + 1, dest, 4);
    }

    return p;
}

inline MemoryPointer MakeShortJmp(MemoryPointer at, MemoryPointer dest = nullptr)
{
    auto p = GetBranchDestination(at);
    Write<uint8_t>(at, 0xEB);

    if (!!dest.AsInt())
    {
        MakeRelativeOffset(at + 1, dest, 1);
    }

    return p;
}

}  // namespace hook
